#include <stdio.h>       
#include <stdlib.h>      
#include <stdbool.h>     // bool, true, false
#include <unistd.h>      // sleep(), close()
#include <signal.h>      // signal(), kill()
#include <sys/socket.h>  // socket(), send(), recv()
#include <netinet/in.h>  // struct sockaddr_in, htons()
#include <arpa/inet.h>   // inet_addr()
#include <string.h>      // strcmp(), memset()
#include <fcntl.h>       // open(), fcntl()
#include <pthread.h>     // pthread_create()
#include <sys/ipc.h>     // ftok()
#include <sys/shm.h>     // shmget(), shmat(), shmdt(), shmctl()
#include <sys/sem.h>     // semget(), semop(), semctl()
#include <errno.h>       // errno


#define diglett_num 5
#define MAX_PLAYER 3
#define GAMETIME 20
#define BUFFER_SIZE 256
#define PORT 8045
#define PATH "/dev/button_driver"           // 設備文件路徑


void sem_lock(int sem_id);
void sem_unlock(int sem_id);


struct game_info {
    int diglett[diglett_num];
    int point[MAX_PLAYER];
    int time;
    int player_num;
    bool nowPlaying;
};

struct shm_struct {
    int diglett[diglett_num];
    int point[MAX_PLAYER];
    int game_time;
    int playerID;
};


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./client <ip> <port>\n");
        exit(EXIT_FAILURE);
    }
    
    char buffer[BUFFER_SIZE] = {0};
    char welcome_buf[BUFFER_SIZE] = {0};
    char read_button_buf[BUFFER_SIZE] = {0};


    int shm_id, sem_id;
    key_t shm_key = 1234;
    key_t sem_key = 8888;
    struct shm_struct *shm_ptr;
    struct game_info game;
    int etx_fd, myID = -1, choice = 100;
    ssize_t bytes, recv_bytes;


    /* Set server address info */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);            // client can link to server by IP address
    server_addr.sin_port = htons(atoi(argv[2]));                 // server's port number

    /* create socket */
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server at %s:%d\n", argv[1], atoi(argv[2]));


    /* create share memory */
    // Create the segment
    if ((shm_id = shmget(shm_key, sizeof(struct shm_struct), IPC_CREAT | 0666)) < 0) {
        perror("shmget failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    // attach the segment to data space
    shm_ptr = (struct shm_struct *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        shmctl(shm_id, IPC_RMID, NULL);
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    memset(shm_ptr, 0, sizeof(struct shm_struct));              // 初始化共享記憶體資料
    

    /* create semaphore */
    sem_id = semget(sem_key, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("semget failed");
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    if (semctl(sem_id, 0, SETVAL, 1) < 0) {                 // initialize semaphore to 1
        perror("Semaphore initialization failed");
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        close(client_sock);
        exit(EXIT_FAILURE);
    }


    // 打開設備文件
    etx_fd = open(PATH, O_RDWR | O_NONBLOCK);               // 直接在打開時設置非阻塞
    if (etx_fd < 0) {
        perror("Failed to open button_device");
        // 清理資源
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_id, 0, IPC_RMID);
        close(client_sock);
        return EXIT_FAILURE;
    }


    // Receive my player ID
    recv_bytes = recv(client_sock, &game, sizeof(struct game_info), 0);
    if (recv_bytes <= 0) {
        perror("Server disconnected or receive error");
    }
    myID = game.player_num;
    printf("My id: %d\n", myID);

    // 寫入共享記憶體 (playerID)
    
    sem_lock(sem_id);
    shm_ptr->playerID = myID;
    shm_ptr->game_time = GAMETIME;
    sem_unlock(sem_id);
    
    // Using this sleep make sure the pygame display initialize correctly (Before ready barrier, so that all the thread will pass simulately after ready barrier)
    sleep(5);

    // server確認client已經ready
    recv_bytes = recv(client_sock, welcome_buf, BUFFER_SIZE, 0);
    if (recv_bytes <= 0) {
        perror("Server disconnected or receive error");
    }
    printf("%s", welcome_buf);
    snprintf(buffer, sizeof(buffer), "ready");
    send(client_sock, buffer, BUFFER_SIZE, 0);              // 傳送ready給server

    printf("read_button_buf check: %s\n\n", read_button_buf);

    while (1) {
        // 讀取更新的 game_info
        recv_bytes = recv(client_sock, &game, sizeof(struct game_info), MSG_DONTWAIT);
        if (recv_bytes == 0) {
            printf("Client closed the connection\n");
            break;
        }
        else if (recv_bytes < 0) {
            // recv_bytes = -1 代表暫時沒有新資料
            // printf("no info update\n");
        }
        else {
            printf("Time left: %d\n", game.time);
            printf("Diglett: ");
            
            for (int i=0; i<diglett_num; i++) {
                printf("%d ", game.diglett[i]);
            }
            printf("\nMy score : %d\n\n", game.point[myID - 1]);

            // 將資料寫入共享記憶體
            sem_lock(sem_id);
            shm_ptr->game_time = game.time;
            for (int i=0; i<diglett_num; i++) {
                shm_ptr->diglett[i] = game.diglett[i];
            }
            for (int i=0; i<MAX_PLAYER; i++) {
                shm_ptr->point[i] = game.point[i];
            }

            if (game.time == 0) {
                printf("Game over received from server.\n");
                break;
            }
            sem_unlock(sem_id);
        }


        memset(read_button_buf, 0, BUFFER_SIZE);
        bytes = read(etx_fd, read_button_buf, sizeof(read_button_buf));     // 讀取按鈕狀態
        

        // If read sth(player action)
        if (bytes > 0) {
            printf("read_button_buf: %s\n", read_button_buf);
            
            char *button_name = strtok(read_button_buf, "\n");              // 從讀到的字串中取出按鈕名稱
            if (button_name != NULL) {
                printf("Button press %s\n", button_name);
                printf("-------Info Update-------\n");
                
                choice = 100;
                if (strcmp(button_name, "BUTTON_1") == 0) {
                    choice = 0;
                }
                else if (strcmp(button_name, "BUTTON_2") == 0) {
                    choice = 1;
                }
                else if (strcmp(button_name, "BUTTON_3") == 0) {
                    choice = 2;
                }
                else if (strcmp(button_name, "BUTTON_4") == 0) {
                    choice = 3;
                }
                else if (strcmp(button_name, "BUTTON_5") == 0) {
                    choice = 4;
                }

                // 傳送玩家選擇給 server
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, sizeof(buffer), "%d", choice);
                if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
                    perror("Failed to send button choice");
                    break;
                }
            }
            else {
                // 沒有取得有效的按鈕名稱
                fprintf(stderr, "No valid button name read.\n");
            }

        }
        else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // EAGAIN, EWOULDBLOCK 表示"目前暫時沒有資料可處理"
            perror("Failed to read button_state");
            break;
        }

        // 若 bytes == 0，表示沒有按下任何按鈕，可持續等待下一次迴圈
    }

    memset(read_button_buf, 0, sizeof(read_button_buf));
    memset(buffer, 0, sizeof(buffer));

    sleep(1); // Using sleep make sure the shm was not closed.
    if ((write(etx_fd, "clear", strlen("clear"))) == -1) {
        perror("Failed to write to device");
    }
    printf("Send 'clear' to driver\n");

    close(etx_fd);
    close(client_sock);

    // detach shm
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
    }

    // remove shm, semaphore
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}


// P operation
void sem_lock(int semid) {
    struct sembuf p = {0, -1, 0}; // 對 semaphore 編號 0, 做 -1
    if (semop(semid, &p, 1) < 0) {
        perror("semop P failed");
    }
}


// V operation
void sem_unlock(int semid) {
    struct sembuf v = {0, 1, 0};  // 對 semaphore 編號 0, 做 +1
    if (semop(semid, &v, 1) < 0) {
        perror("semop V failed");
    }
}
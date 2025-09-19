#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>             // signal()
#include <sys/socket.h>         // socket(), connect()
#include <netinet/in.h>         // struct sockaddr_in
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/time.h>

/* Global variables */
#define diglett_num 5
#define MAX_PLAYER 3
#define GAMETIME 20
#define BUFFER_SIZE 256
#define PORT 8045

struct thread_info {
    int socket;
    int id;
};

struct game_info {
    int diglett[diglett_num];
    int point[MAX_PLAYER];
    int time;
    int player_num;
    bool nowPlaying;
};

int server_sock, ready = 0;
int socket_list[MAX_PLAYER] = {0};                  // 存threads的socket_num
pthread_t thread_list[MAX_PLAYER] = {0};            // 存threads的PID
struct game_info game;
pthread_barrier_t barrier, ready_barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void initialize_game_info();
void *client_handler(void *arg);
void *timer_thread(void *arg);
void timer_handler(int signum);
void info_sync(int signo, siginfo_t *info, void *context);
void close_socket(int signum);


int main(int argc, char *argv[]) {

    /* Server socket setting */
    struct sockaddr_in server_addr, client_addr;
    pthread_t game_timer;
    int client_sock;
    int addr_len = sizeof(client_addr);
    int yes = 1;
    
    // Set server address info
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);                        // Server can accept request from any IP with port=PORT
    server_addr.sin_port = htons(PORT);                                     // server's port number
    
    // New server socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {              // AF_INET = IPv4, SOCK_STREAM = tcp
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, close_socket);                                           // enable Ctrl+C to close server

    // enable socket to use the same IP & PORT
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));     

    // bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind socket failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // listen to request
    if (listen(server_sock, MAX_PLAYER) < 0) {                                       // set the server max connection pending = 5
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", PORT);

    initialize_game_info();

    // 要初始化的 barrier 物件
    pthread_barrier_init(&barrier, NULL, MAX_PLAYER);            // 先寫死, 之後再看要不要改
    pthread_barrier_init(&ready_barrier, NULL, MAX_PLAYER);

    // 底下新增一個timer thread, 處理間隔一秒的signal
    pthread_create(&game_timer, NULL, timer_thread, NULL);
    pthread_detach(game_timer);


    while(1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // 動態分配 memory 給 thread_info
        struct thread_info *thread = malloc(sizeof(struct thread_info));
        if (thread == NULL) {
            perror("Failed to allocate memory for thread_info");
            close(client_sock);
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (game.player_num >= MAX_PLAYER) {
            // 超過最大玩家數，拒絕連接
            char *msg = "Server full. Try again later.\n";
            send(client_sock, msg, strlen(msg), 0);
            close(client_sock);
            free(thread);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        thread->id = game.player_num;
        thread->socket = client_sock;
        socket_list[thread->id] = client_sock;
        game.player_num += 1;
        pthread_mutex_unlock(&mutex);

        // 發送遊戲資訊給客戶端
        if (send(client_sock, &game, sizeof(struct game_info), 0) < 0){
            printf("Game info. error updated w/ thread %d.\n", client_sock);
        }

        // 創建處理客戶端的執行緒，並傳遞獨立的 thread_info

        // 創建 thread 的時候，會把 thread 的 ID 存入 thread_list[thread->id] 裡面
        // 而 thread_list[thread->id] 裡面的 thread->id 會是 0、1、2
        // 這三支 thread 會分別跳到各自的 client_handler 等待執行
        if (pthread_create(&thread_list[thread->id], NULL, client_handler, (void *)thread) != 0) {
            perror("Failed to create thread");
            close(client_sock);
            free(thread);
            continue;
        }

        printf("Player: %d, socket: %d\n", game.player_num, client_sock);
        pthread_detach(thread_list[thread->id]);
    }

    pthread_mutex_destroy(&mutex);
    close(server_sock);
    return 0;
}


void initialize_game_info() {
    memset(game.diglett, 0, sizeof(game.diglett));
    memset(game.point, 0, sizeof(game.point));
    game.time = GAMETIME;
    game.player_num = 0;
    game.nowPlaying = false;
}


void *client_handler(void *arg) {               
    struct thread_info *thread = (struct thread_info *)arg;
    struct sigaction send_info;
    int client_sock = thread->socket;
    int thread_id = thread->id;
    char buffer[BUFFER_SIZE] = {0};
    int choice;

    snprintf(buffer, BUFFER_SIZE, "Welcome! Are you ready ~\n");
    send(client_sock, buffer, strlen(buffer), 0);

    // 等待玩家ready
    memset(buffer, 0, BUFFER_SIZE);
    int n = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if (n <= 0 || strncmp(buffer, "ready", 5) != 0) {
        close(client_sock);
        free(thread);  // 釋放資源
        return NULL;
    }

    // 收到ready後等待其他玩家都ready後才繼續
    // pthread_barrier_wait() 可以知道目前有幾個 thread 到達
    // 並且它會去檢查 ready_barrier 的等待值是多少
    pthread_barrier_wait(&ready_barrier);
    pthread_mutex_lock(&mutex);
    if (game.player_num == MAX_PLAYER && !game.nowPlaying) {
        game.nowPlaying = true;
    }
    pthread_mutex_unlock(&mutex);

    /* catch signal SIGUSR1 */
    send_info.sa_sigaction = info_sync;
    send_info.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &send_info, NULL) < 0) {
        perror("sigaction failed");
        exit(1);
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(client_sock, buffer, BUFFER_SIZE, MSG_DONTWAIT);
        if (n == 0) {
            printf("Client closed the connection\n");
            break;
        }
        else if (n < 0) {
            // printf("Player no action\n");
            continue;
        }

        if (game.time < GAMETIME && game.time > 0) {
            printf("choice: %c\n", buffer[0]);
            // 讀取 buffer
            if (sscanf(buffer, "%d", &choice) == 1) {
                // 玩家有操作
                if (choice != 100) {
                    // 有打到地鼠或炸彈
                    pthread_mutex_lock(&mutex);
                    if (choice >= 0 && choice < diglett_num && game.diglett[choice] != 0) {
                        game.point[thread_id] += game.diglett[choice];
                        game.diglett[choice] = 0;
                    }
                    pthread_mutex_unlock(&mutex);
                    // 通知所有client更新畫面
                    for (int i = 0; i < game.player_num; i++) {
                        if (thread_list[i] != pthread_self()) {
                            pthread_kill(thread_list[i], SIGUSR1);
                        }
                    }
                    pthread_kill(pthread_self(), SIGUSR1);
                }
            }
        }
        else if (game.time == 0) {
            // 通知所有client更新畫面
            for (int i = 0; i < game.player_num; i++) {
                if (thread_list[i] != pthread_self()) {
                    pthread_kill(thread_list[i], SIGUSR1);
                }
            }
            pthread_kill(pthread_self(), SIGUSR1);
            printf("Game over, ");
            printf("time left : %d\n", game.time);
            break;
        }
    }

    close(client_sock);
    free(thread);  // 釋放資源
    pthread_exit(NULL);
}

// 這個 timer thread 每秒就會呼叫一次 timer_handler
void *timer_thread(void *arg) {

    /* Timer setting */
    struct sigaction sa;
    struct itimerval timer;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);

    while (1) {
        if (game.time == 0) {
            break;
        }
    }

    pthread_exit(NULL);
}


void timer_handler(int signum) {
    int hole_diglett, hole_bomb, num_diglett, rand_inital_pos;

    if (game.nowPlaying) {
        pthread_mutex_lock(&mutex);
        srand(time(NULL));
        game.time -= 1;
        memset(game.diglett, 0, sizeof(game.diglett));

        // 單隻地鼠
        // 出現地鼠機率 = 80%、超級地鼠機率 = 20%
        hole_diglett = rand() % diglett_num;            // random number 0~4
        if ((rand() % 5) == 1) {
            game.diglett[hole_diglett] = 2;             // Ex diglett
        }
        else {
            game.diglett[hole_diglett] = 1;
        }
        
        // 炸彈
        // 出現炸彈機率 = 33%
        if ((rand() % 3) == 1) {
            hole_bomb = hole_diglett + 1;
            if (hole_bomb >= diglett_num) {
                hole_bomb = hole_diglett - 1;
            }
            game.diglett[hole_bomb] = -1;
        }

        // 特殊機制
        if (game.time == 0) {
            printf("\nGame over!\n");
            memset(game.diglett, 0, sizeof(game.diglett));
        }
        else if (game.time == 1) {
            memset(game.diglett, 0, sizeof(game.diglett));
            num_diglett = rand() % 4 + 2;                       // 2~5 digletts
            rand_inital_pos = rand() % 4;                       // the rand initial position @ final round
            for(int i=0; i<num_diglett; i++) {
                hole_diglett = i + rand_inital_pos;         
                if (hole_diglett > 4) {
                    hole_diglett = hole_diglett - 4;
                }
                game.diglett[hole_diglett] = 1;
            }
        }
        pthread_mutex_unlock(&mutex);

        // 通知client thread處理
        for (int i=0; i<game.player_num; i++) {
            if (pthread_kill(thread_list[i], SIGUSR1) < 0) {
                perror("Failed to send SIGUSR1 to client");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void info_sync(int signo, siginfo_t *info, void *context) {
    // Find the current thread id
    int i;
    for (i=0; i<MAX_PLAYER; i++) {
        if (pthread_equal(thread_list[i], pthread_self()))
            break;
    }

    if (i >= MAX_PLAYER) {
        printf("Error: Thread not found in thread_list.\n");
        return;
    }

    // If the game is in progress, wait other threads till all thread arrived the barrier
    /*    
    if (game.nowPlaying) {
        pthread_barrier_wait(&barrier);
    }
    */

    printf("\nTime left: %d\n", game.time);
    // Send the update Game info to "specific" client id, not broadcast to all client, since each client will enter this function
    if (send(socket_list[i], &game, sizeof(struct game_info), 0) < 0) {
        printf("Game info. error updated w/ thread %d.\n", socket_list[i]);
    }
    printf("Game info. update success, sent to thread %d\n", socket_list[i]);
}


void close_socket(int signum) {
    close(server_sock);
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    pthread_barrier_destroy(&ready_barrier);

    printf("\nServer is shutting down...\n");
    exit(0);
}
#include <errno.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define SEM_MODE 0666
#define SEM_KEY 12345

// 帳戶餘額
int account = 0;

// Semaphore ID for OS
int sem;

int P(int s)
{ 
    struct sembuf sop;
    sop.sem_num =  0;
    sop.sem_op  = -1;
    sop.sem_flg =  0;
    
    // 如果 semaphore counter 等於 0，semop() 會 blocking，直到 counter 被 release
    if(semop(s, &sop, 1) < 0)
    {
        fprintf(stderr,"P(): semop failed: %s\n",strerror(errno)); 
        return -1;
    }
    else
    { 
        return 0; 
    } 
}

int V(int s)
{ 
    struct sembuf sop;
    sop.sem_num =  0;
    sop.sem_op  =  1;
    sop.sem_flg =  0;
    
    if(semop(s, &sop, 1) < 0)
    {  
        fprintf(stderr,"V(): semop failed: %s\n",strerror(errno)); 
        return -1; 
    }
    else
    { 
        return 0; 
    }
} 

void handle_signal(int sig)
{
    semctl(sem, 0, IPC_RMID, 0);
    exit(0);
}

void* handle_client(void* arg)
{
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if(bytes_read <= 0) break; // 連線中斷

        // 處理請求 (格式: 操作類型 空格 金額)
        char operation[16];
        int amount = 0;
        sscanf(buffer, "%s %d", operation, &amount);

        // critical section begin
        P(sem);
        if(strcmp(operation, "deposit") == 0)
        {
            if(account >= 0)
            {
            account = account + amount;
            fprintf(stderr, "After deposit: %d\n", account);
            }
        }
        
        if(strcmp(operation, "withdraw") == 0)
        {
            if(account >= 0)
            {
                account = account - amount;
                if(account < 0)
                {
                    break;
                }
                fprintf(stderr, "After withdraw: %d\n", account);
            }
        }
        V(sem);
        // critical section end

        // 回傳結果給 client
        snprintf(buffer, BUFFER_SIZE, "Current balance: %d\n", account);
        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
    return NULL;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_signal);  // Ctrl + C = remove semaphore

    int port = atoi(argv[1]);
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket, 10) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    // create semaphore
    sem = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | SEM_MODE); 
        
    // initial binary semaphore
    semctl(sem, 0, SETVAL, 1);

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int* client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if(*client_socket < 0)
        {
            perror("Accept failed");
            free(client_socket);
            continue;
        }
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_socket);
        pthread_detach(thread);
    }

    close(server_socket);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

typedef struct
{
    int guess;
    char result[8];
} data;

data *shmaddr;
int shmid;
int answer;

void handle_signal(int sig)
{
    shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

void sigusr1_handler(int signo)
{
    if(shmaddr -> guess == answer)
    {
        strcpy(shmaddr -> result, "bingo");  // 把 bingo 存在 result 裡面
        printf("[game] Guess %d, bingo\n", shmaddr -> guess);
    }

    if(shmaddr -> guess > answer)
    {
        strcpy(shmaddr -> result, "smaller");  // 告訴 guess.c 猜的值要小一點
        printf("[game] Guess %d, smaller\n", shmaddr -> guess);
    }

    if(shmaddr -> guess < answer)
    {
        strcpy(shmaddr -> result, "bigger");  // 告訴 guess.c 猜的值要大一點
        printf("[game] Guess %d, bigger\n", shmaddr -> guess);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_signal);  // Ctrl + C = remove share memory

    printf("[game] Game PID: %d\n", getpid());

    key_t key = atoi(argv[1]);  // 十進位 1234 = 十六進位 4D2
    answer = atoi(argv[2]);

    // create shared memory
    shmid = shmget(key, sizeof(data), IPC_CREAT | 0666);
    if(shmid == -1)
    {
        perror("shmget");
        return 0;
    }

    shmaddr = shmat(shmid, NULL, 0);  // share memory 的記憶體位址
    if(shmaddr == (void *)-1)
    {
        perror("shmat");
        return 0;
    }

    memset(shmaddr, 0, sizeof(data));
    strcpy(shmaddr->result, "first guess");
    
    // Signal handlers
    struct sigaction sa_usr1 = {0};
    sa_usr1.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    while(1)
    {
        pause();
    }

    return 0;
}

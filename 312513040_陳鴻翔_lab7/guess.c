#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int guess;
    char result[8];
} data;

data *shared;
int left = 1;
int right = 0;
int mid = 0;
int initial_guess = 0;
int end_flag = 0;
pid_t pid;

void timer_handler(int signo)
{
    if(strcmp(shared -> result, "bingo") == 0)
    {
        end_flag = 1;
        return;
    }

    // 更新上下界
    if(strcmp(shared -> result, "smaller") == 0)
    {
        right = mid - 1;
    }

    if(strcmp(shared -> result, "bigger") == 0)
    {
        left = mid + 1;
    }

    mid = (left + right) / 2;
    shared -> guess = mid;
    printf("[game] Guess: %d\n", mid);
    kill(pid, SIGUSR1);
}

int main(int argc, char *argv[])
{
    key_t key = atoi(argv[1]);
    right = atoi(argv[2]);
    pid = atoi(argv[3]);

    // access shared memory
    int shmid = shmget(key, sizeof(data), IPC_CREAT | 0666);

    // get the share memory pointer
    shared = (data *)shmat(shmid, NULL, 0);

    mid = (left + right) / 2;
    shared -> guess = mid;  // initial guess value
    strcpy(shared -> result, " ");  // 確保 result 為空
    printf("[game] Guess: %d\n", mid);
    kill(pid, SIGUSR1);

    // Set up timer
    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    if(setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
    signal(SIGALRM, timer_handler);

    // Wait for guesses to complete
    while(!end_flag)
    {
        pause();
    }

    return 0;
}

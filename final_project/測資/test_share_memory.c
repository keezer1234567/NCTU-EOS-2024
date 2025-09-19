#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#define DIGLETT_NUM 5
#define MAX_PLAYER 2

struct shm_struct {
    int diglett[DIGLETT_NUM];
    int point[MAX_PLAYER];
    int game_time;
    int playerID;
};

int main()
{
    int shmid = shmget(1234, sizeof(struct shm_struct), 0666 | IPC_CREAT);

    if(shmid < 0)
    {
        perror("shmget failed");
        exit(1);
    }

    struct shm_struct *shm_data = (struct shm_struct *)shmat(shmid, NULL, 0);

    if(shm_data == (void *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    srand(time(NULL));

    shm_data->game_time = 15; 
    shm_data->playerID = 1;
    memset(shm_data->point, 0, sizeof(shm_data->point));

    while(1)
    {
        for(int i = 0; i < DIGLETT_NUM; i++)
        {
            shm_data->diglett[i] = (rand() % 4) - 1; // 隨機生成 -1 到 2 的數字
        }
        shm_data->point[0] += rand() % 3;
        shm_data->point[1] += rand() % 3;

        if(shm_data->game_time > 0)
        {
            shm_data->game_time--;
        }
        printf("diglett: [%d, %d, %d, %d, %d]\n",
               shm_data->diglett[0], shm_data->diglett[1], shm_data->diglett[2],
               shm_data->diglett[3], shm_data->diglett[4]);
        printf("point: [%d, %d], game_time: %d\n",
               shm_data->point[0], shm_data->point[1], shm_data->game_time);
        sleep(1);
    }
    shmdt(shm_data);
    return 0;
}

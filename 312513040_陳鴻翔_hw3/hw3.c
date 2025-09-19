#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENTS 2

// 餐廳列表
char *shop_list = "Dessert shop:3km\n"
                  "- cookie:$60|cake:$80\n"
                  "Beverage shop:5km\n"
                  "- tea:$40|boba:$70\n"
                  "Diner:8km\n"
                  "- fried-rice:$120|Egg-drop-soup:$50\n";

// 用陣列存放外送員的等待時間
int delivery_time[2] = {0, 0};
pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;

// 選擇餐廳的旗標、各種餐點的數量
typedef struct
{
    int X;
    int Y;
    int client_sock;
    int shop_flag;
    int cookie_amount;
    int cake_amount;
    int tea_amount;
    int boba_amount;
    int fried_amount;
    int egg_amount;
    int waiting_time;
    int confirm_flag;
} all_data;

// 選擇餐廳
enum select_shop
{
    unset,
    select_dessert,
    select_beverage,
    select_diner
};

void confirm(all_data *order, char *buffer)
{
    char response_confirm[256];
    int total_cost = 0;
    int delivery_distance = 0;
    int driver_flag = 0;  // 0 代表給第一位外送員送餐，1 代表給第二位外送員送餐

    // 避免有人點餐後，連續送三次 confirm
    if(order->confirm_flag != 1)
    {
        return;
    }
    if(order->shop_flag == select_dessert)
    {
        total_cost = order->cookie_amount * 60 + order->cake_amount * 80;
        delivery_distance = 3;
    }
    if(order->shop_flag == select_beverage)
    {
        total_cost = order->tea_amount * 40 + order->boba_amount * 70;
        delivery_distance = 5;
    }
    if(order->shop_flag == select_diner)
    {
        total_cost = order->fried_amount * 120 + order->egg_amount * 50;
        delivery_distance = 8;
    }

    if(order->shop_flag == unset)
    {
        send(order->client_sock, "Please order some meals\n", 256, 0);
        return;
    }

    // 計算外送等待時間
    pthread_mutex_lock(&delivery_mutex);
    if(delivery_time[0] <= delivery_time[1])
    {
        driver_flag = 0;
    }
    else
    {
        driver_flag = 1;
    }
    delivery_time[driver_flag] += delivery_distance; // 初次更新外送員等待時間
    pthread_mutex_unlock(&delivery_mutex);

    // 如果等待時間超過 30 秒
    if(delivery_time[driver_flag] > 30)
    {
        send(order->client_sock, "Your delivery will take a long time, do you want to wait?\n", 256, 0);
        
        // 接收客戶回應
        ssize_t received = recv(order->client_sock, buffer, sizeof(buffer), 0);
        if(strncmp(buffer, "No", 2) == 0)
        {
            pthread_mutex_lock(&delivery_mutex);
            delivery_time[driver_flag] -= delivery_distance; // 把剛剛紀錄的等待時間從陣列裡扣掉
            pthread_mutex_unlock(&delivery_mutex);
            sleep(1);  // 不打這個 Part2 一直失敗，不知道為何
            return;
        }
        if(strncmp(buffer, "Yes", 3) == 0)
        {
            pthread_mutex_lock(&delivery_mutex);
            if(delivery_time[0] <= delivery_time[1])
            {
                driver_flag = 0;
            }
            else
            {
                driver_flag = 1;
            }
            delivery_time[driver_flag] += delivery_distance; // 把訂單給比較閒的外送員
            pthread_mutex_unlock(&delivery_mutex);

            send(order->client_sock, "Please wait a few minutes...\n", 256, 0);
            sleep(delivery_distance);
        }
    }
    else
    {
        // 如果等待時間不超過 30 秒
        send(order->client_sock, "Please wait a few minutes...\n", 256, 0);
        sleep(delivery_distance);
    }

    pthread_mutex_lock(&delivery_mutex);
    delivery_time[driver_flag] -= delivery_distance; // 送完訂單要把等待時間從陣列裡面扣掉
    pthread_mutex_unlock(&delivery_mutex);

    snprintf(response_confirm, sizeof(response_confirm), "Delivery has arrived and you need to pay %d$\n", total_cost);
    send(order->client_sock, response_confirm, 256, 0);
}

void *handle_client(void *arg)
{
    all_data *order = (all_data *)arg;
    char buffer[256];
    char *response_deal = NULL;

        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t received = recv(order->client_sock, buffer, sizeof(buffer), 0);
            buffer[received] = '\0';
            printf("%s", buffer);

            if(strcmp(buffer, "shop list") == 0)
            {
                send(order->client_sock, shop_list, 256, 0);
            }
            
            if(strncmp(buffer, "order", 5) == 0)
            {
                char item[100];
                int quantity;

                if(sscanf(buffer, "order %s %d", item, &quantity) == 2)
                {
                    if(strstr(item, "cookie") || strstr(item, "cake"))
                    {
                        if(order->shop_flag == unset || order->shop_flag == select_dessert)
                        {
                            order->shop_flag = select_dessert;
                            if(strcmp(item, "cookie") == 0)
                            {
                                order->cookie_amount += quantity;
                            }
                            if(strcmp(item, "cake") == 0)
                            {
                                order->cake_amount += quantity;
                            }
                        }
                    }
                    
                    if(strstr(item, "tea") || strstr(item, "boba"))
                    {
                        if(order->shop_flag == unset || order->shop_flag == select_beverage)
                        {
                            order->shop_flag = select_beverage;
                            if(strcmp(item, "tea") == 0)
                            {
                                order->tea_amount += quantity;
                            }
                            if(strcmp(item, "boba") == 0)
                            {
                                order->boba_amount += quantity;
                            }
                        }
                    }
                    
                    if(strstr(item, "fried-rice") || strstr(item, "Egg-drop-soup"))
                    {
                        if(order->shop_flag == unset || order->shop_flag == select_diner)
                        {
                            order->shop_flag = select_diner;
                            if(strcmp(item, "fried-rice") == 0)
                            {
                                order->fried_amount += quantity;
                            }
                            if(strcmp(item, "Egg-drop-soup") == 0)
                            {
                                order->egg_amount += quantity;
                            }
                        }
                    }
                }

                // 回傳當前訂單
                char response[256] = "";
                if(order->cookie_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "cookie %d|", order->cookie_amount);
                }
                    
                if(order->cake_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "cake %d|", order->cake_amount);
                }
                    
                if(order->tea_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "tea %d|", order->tea_amount);
                }
                    
                if(order->boba_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "boba %d|", order->boba_amount);
                }

                if(order->fried_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "fried-rice %d|", order->fried_amount);
                }
                    
                if(order->egg_amount > 0)
                {
                    snprintf(response + strlen(response), sizeof(response), "Egg-drop-soup %d|", order->egg_amount);
                }

                if(strlen(response) > 0)
                {
                    response[strlen(response) - 1] = '\n';
                }
                send(order->client_sock, response, 256, 0);
            }
            
            if(strncmp(buffer, "confirm", 7) == 0)
            {
                order->confirm_flag += 1;
                confirm(order, buffer);
                order->confirm_flag = 0;
                break;
            }
            
            if(strncmp(buffer, "cancel", 6) == 0)
            {
                break;
            }
    }
    close(order->client_sock);
    free(arg);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if(listen(server_sock, MAX_CLIENTS) == -1)
    {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n", argv[1]);

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);

        if(client_sock == -1)
        {
            perror("accept failed");
            continue;
        }

        // 在 heap 創建一個空間，然後讓 order 去指向它
        all_data *order;
        order = (all_data *)malloc(sizeof(all_data));
        memset(order, 0, sizeof(all_data));
        order->client_sock = client_sock;

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client, (void *)order) != 0)
        {
            perror("pthread_create failed");
            close(client_sock);
            free(order);
            continue;
        }
        pthread_detach(thread_id);
        printf("\n");
    }
    close(server_sock);
    return 0;
}

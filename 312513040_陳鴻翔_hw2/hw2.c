#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define max_client 1

// 餐廳列表
char *shop_list = "Dessert shop:3km\n"
                    "- cookie:$60|cake:$80\n"
                    "Beverage shop:5km\n"
                    "- tea:$40|boba:$70\n"
                    "Diner:8km\n"
                    "- fried-rice:$120|Egg-drop-soup:$50\n";
// 選擇餐廳
enum select_shop
{
    unset,
    select_dessert,
    select_beverage,
    select_diner
};

// 選擇餐廳的旗標、各種餐點的數量
int shop_flag = 0;
int cookie_amount = 0;
int cake_amount = 0;
int tea_amount = 0;
int boba_amount = 0;
int fried_amount = 0;
int egg_amount = 0;

// 計算各種餐點的陣列
char response[256];

// confirm 要傳的訊息
char *wait_message = "Please wait a few minutes...\n";

// 沒送餐就 confirm 的訊息
char *order_message = "Please order some meals\n";

void confirm(int client_sock)
{
    char confirm_response[256];
    int total_cost = 0;
    if(shop_flag == select_dessert)
    {
        total_cost = cookie_amount * 60 + cake_amount * 80;
        send(client_sock, wait_message, 256, 0);
        sleep(3);
        sprintf(confirm_response, "Delivery has arrived and you need to pay %d$\n", total_cost);
        send(client_sock, confirm_response, 256, 0);
    }
    if(shop_flag == select_beverage)
    {
        total_cost = tea_amount * 40 + boba_amount * 70;
        send(client_sock, wait_message, 256, 0);
        sleep(5);
        sprintf(confirm_response, "Delivery has arrived and you need to pay %d$\n", total_cost);
        send(client_sock, confirm_response, 256, 0);
    }
    if(shop_flag == select_diner)
    {
        total_cost = fried_amount * 120 + egg_amount * 50;
        send(client_sock, wait_message, 256, 0);
        sleep(8);
        sprintf(confirm_response, "Delivery has arrived and you need to pay %d$\n", total_cost);
        send(client_sock, confirm_response, 256, 0);
    }
}

char* dessert_shop(char *desert)
{
    char item[100];
    int X = 0;
    int Y = 0;
    int offset = 0;

    if(strncmp(desert, "order cookie", 12) == 0)
    {
        sscanf(desert, "order %s %d", item, &X);
        cookie_amount += X;
    }
    else if(strncmp(desert, "order cake", 10) == 0)
    {
        sscanf(desert, "order %s %d", item, &Y);
        tea_amount += Y;
    }

    if(cookie_amount > 0)
    {
        offset += sprintf(response + offset, "cookie %d", cookie_amount);
    }

    if(cake_amount > 0)
    {
        if(offset > 0)
        {
            offset += sprintf(response + offset, "|");
        }
        offset += sprintf(response + offset, "cake %d", cake_amount);
    }

    if(offset > 0)
    {
        sprintf(response + offset, "\n");
    }

    return response;
}

char* beverage_shop(char *beverage)
{
    char item[100];
    int X = 0;
    int Y = 0;
    int offset = 0;

    if(strncmp(beverage, "order tea", 9) == 0)
    {
        sscanf(beverage, "order %s %d", item, &X);
        tea_amount += X;
    }
    else if(strncmp(beverage, "order boba", 10) == 0)
    {
        sscanf(beverage, "order %s %d", item, &Y);
        boba_amount += Y;
    }

    if(tea_amount > 0)
    {
        offset += sprintf(response + offset, "tea %d", tea_amount);
    }

    if(boba_amount > 0)
    {
        if(offset > 0)
        {
            offset += sprintf(response + offset, "|");
        }
        offset += sprintf(response + offset, "boba %d", boba_amount);
    }

    if(offset > 0)
    {
        sprintf(response + offset, "\n");
    }

    return response;
}

char* diner_shop(char *diner)
{
    char item[100];
    int X = 0;
    int Y = 0;
    int offset = 0;

    if(strncmp(diner, "order fried-rice", 16) == 0)
    {
        sscanf(diner, "order %s %d", item, &X);
        fried_amount += X;
    }
    else if(strncmp(diner, "order Egg-drop-soup", 19) == 0)
    {
        sscanf(diner, "order %s %d", item, &Y);
        egg_amount += Y;
    }

    if(fried_amount > 0)
    {
        offset += sprintf(response + offset, "fried-rice %d", fried_amount);
    }

    if(egg_amount > 0)
    {
        if (offset > 0)
        {
            offset += sprintf(response + offset, "|");
        }
        offset += sprintf(response + offset, "Egg-drop-soup %d", egg_amount);
    }

    if(offset > 0)
    {
        sprintf(response + offset, "\n");
    }

    return response;
}

void handle_client(int client_sock)
{
    char buffer[256];
    int distance = 0;
    int total_cost = 0;
    char *response1 = NULL;

    while(1)
    {
        memset(buffer, 0, 256);
        ssize_t received = recv(client_sock, buffer, sizeof(buffer)-1, 0);

        buffer[received] = '\0';
        printf("%s\n", buffer);  // Client 傳過來的字串

        if(strcmp(buffer, "shop list") == 0)
        {
            send(client_sock, shop_list, 256, 0);
        }
        
        if(strncmp(buffer, "order cookie", 12) == 0 || strncmp(buffer, "order cake", 10) == 0)
        {
            if(shop_flag == unset || shop_flag == select_dessert)
            {
                shop_flag = select_dessert;
                response1 = dessert_shop(buffer);
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_beverage)
            {
                response1 = beverage_shop("");
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_diner)
            {
                response1 = diner_shop("");
                send(client_sock, response1, 256, 0);
            }
        }

        if(strncmp(buffer, "order tea", 9) == 0 || strncmp(buffer, "order boba", 10) == 0)
        {
            if(shop_flag == unset || shop_flag == select_beverage)
            {
                shop_flag = select_beverage;
                response1 = beverage_shop(buffer);
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_dessert)  // 如果輸入 cookie 3、tea1，送 cookie 3 給 Client
            {
                response1 = dessert_shop("");
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_diner)
            {
                response1 = diner_shop("");
                send(client_sock, response1, 256, 0);
            }
        }

        if(strncmp(buffer, "order fried-rice", 16) == 0 || strncmp(buffer, "order Egg-drop-soup", 19) == 0)
        {
            if(shop_flag == unset || shop_flag == select_diner)
            {
                shop_flag = select_diner;
                response1 = diner_shop(buffer);
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_dessert)
            {
                response1 = dessert_shop("");
                send(client_sock, response1, 256, 0);
            }
            else if(shop_flag == select_beverage)
            {
                response1 = beverage_shop("");
                send(client_sock, response1, 256, 0);
            }
        }

        if(strncmp(buffer, "confirm", 7) == 0)
        {
            if(shop_flag == unset)
            {
                send(client_sock, order_message, 256, 0);
                continue;
            }
            confirm(client_sock);
            return;
        }

        if(strncmp(buffer, "cancel", 6) == 0)
        {
            // 要清除所有全域變數，否則下一個 Client 連進來會讀到上個 Client 的餐點
            memset(response, 0, 256);
            shop_flag = unset;
            cookie_amount = 0;
            cake_amount = 0;
            tea_amount = 0;
            boba_amount = 0;
            fried_amount = 0;
            egg_amount = 0;
            return;
        }
    }
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

    if(listen(server_sock, max_client) == -1)
    {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n\n", argv[1]);

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
        handle_client(client_sock);
        printf("\n\n");
        shop_flag = unset;  // 當 Server 送餐成功必須清除 flag，否則下個 Client 進來會選到上個 Client 的餐廳
        close(client_sock);
    }
    close(server_sock);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>  // open()、read()、write()
#include <unistd.h> // sleep()
#include <sys/wait.h> // waitpid()
#include <termios.h>

void waitForAnyKey()
{
    struct termios oldt, newt;
    
    // 獲取當前終端屬性並保存
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 設置終端為無回應模式（不需按下 Enter 鍵）
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("!-- 按任意鍵回主選單 --\n");
    getchar();

    // 還原終端屬性
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void showShopList()
{
    printf("\n");
    printf("Dessert shop: 3km\n");
    printf("Beverage shop: 5km\n");
    printf("Diner: 8km \n");
    waitForAnyKey();
}

void display_seg(int totalCost)
{
    int fd = open("/dev/seg_Dev", O_WRONLY);
    int divisor = 1;

    while(totalCost/divisor >= 10)
    {
        divisor *= 10;
    }

    while(divisor > 0)
    {
        int current_digit = (totalCost/divisor) % 10;
        write(fd, &current_digit, sizeof(current_digit));
        sleep(1);
        divisor /= 10;
    }

    close(fd);
}

void display_led(int distance)
{
    int fd = open("/dev/led_Dev", O_WRONLY);
    printf("​​​​Please wait for a few minutes...\n");
    if(distance == 3)
    {
        for(int i = 0; i <= 3; i++)
        {
            write(fd, &distance, sizeof(distance));
            distance--;
            sleep(1);
        }
    }

    if(distance == 5)
    {
        for(int i = 0; i <= 5; i++)
        {
            write(fd, &distance, sizeof(distance));
            distance--;
            sleep(1);
        }
    }

    if(distance == 8)
    {
        for(int i = 0; i <= 8; i++)
        {
            write(fd, &distance, sizeof(distance));
            distance--;
            sleep(1);
        }
    }
    printf("​​​​​​​please pick up your meal\n");
    getchar();  // 清除換行符號
    close(fd);
    waitForAnyKey();
}

void takeOrder()
{
    int distance = 0;
    int totalCost = 0;
    int shopChoice;  // 選擇哪個商店
    int itemChoice;  // 選擇哪項商品
    int number;  // 商品的價格

    totalCost = 0;  // 重設每次新訂單的金額
    distance = 0;

    printf("\n");
    printf("!-- 餐廳選單 --\n");
    printf("Please choose from 1~3\n");
    printf("1. Dessert shop\n");
    printf("2. Beverage shop\n");
    printf("3. Diner\n");
    printf("Enter choice: ");
    scanf("%d", &shopChoice);

    switch(shopChoice)
    {
        case 1:
            // into dessert shop
            distance = 3;
            break;

        case 2:
            // into beverage shop
            distance = 5;
            break;

        case 3:
            // into diner
            distance = 8;
            break;
    }

    while(1)
    {
        if(distance == 3)
        {
            printf("\n");
            printf("!-- 訂餐選單 --\n");
            printf("Please choose from 1~4\n");
            printf("1. cookie: $60\n");
            printf("2. cake: $80\n");
            printf("3. confirm\n");
            printf("4. cancel\n");
            printf("Enter choice: ");
            scanf("%d", &itemChoice);

            if(itemChoice == 1)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 60;
            }

            if(itemChoice == 2)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 80; 
            }

            if(itemChoice == 3)
            {
                if(totalCost == 0)
                {
                    printf("!-- 取消訂單，回到主選單 --\n\n");
                    return;  
                }

                pid_t pid1 = fork(); // create child process 1
                if(pid1 == 0)
                {
                    display_seg(totalCost);
                    exit(0);
                }

                pid_t pid2 = fork(); // // create child process 2
                if(pid2 == 0)
                {
                    display_led(distance);
                    exit(0);
                }

                // this is parent process
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
                return;
            }
            
            if(itemChoice == 4)
            {
                printf("!-- 取消訂單，回到主選單 --\n\n");
                return;
            }
        }

        if(distance == 5)
        {
            printf("\n");
            printf("Please choose from 1~4\n");
            printf("1. tea: $40\n");
            printf("2. boba: $70\n");
            printf("3. confirm\n");
            printf("4. cancel\n");
            printf("Enter choice: ");
            scanf("%d", &itemChoice);

            if(itemChoice == 1)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 40;
            }

            if(itemChoice == 2)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 70; 
            }

            if(itemChoice == 3)
            {
                if(totalCost == 0)
                {
                    printf("!-- 取消訂單，回到主選單 --\n\n");
                    return;  
                }

                pid_t pid1 = fork(); // create child process 1
                if(pid1 == 0)
                {
                    display_seg(totalCost);
                    exit(0);
                }

                pid_t pid2 = fork(); // // create child process 2
                if(pid2 == 0)
                {
                    display_led(distance);
                    exit(0);
                }

                // this is parent process
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
                return;
            }

            if(itemChoice == 4)
            {
                printf("!-- 取消訂單，回到主選單 --\n\n");
                return;
            }
        }

        if(distance == 8)
        {
            printf("\n");
            printf("Please choose from 1~4\n");
            printf("1. fried rice: $120\n");
            printf("2. egg-drop soup: $50\n");
            printf("3. confirm\n");
            printf("4. cancel\n");
            printf("Enter choice: ");
            scanf("%d", &itemChoice);

            if(itemChoice == 1)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 120;
            }

            if(itemChoice == 2)
            {
                printf("How many? ");
                scanf("%d", &number);
                totalCost += number * 50; 
            }

            if(itemChoice == 3)
            {
                if(totalCost == 0)
                {
                    printf("!-- 取消訂單，回到主選單 --\n\n");
                    return;  
                }
                pid_t pid1 = fork(); // create child process 1
                if(pid1 == 0)
                {
                    display_seg(totalCost);
                    exit(0);
                }

                pid_t pid2 = fork(); // // create child process 2
                if(pid2 == 0)
                {
                    display_led(distance);
                    exit(0);
                }

                // this is parent process
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
                return;
            }
            
            if(itemChoice == 4)
            {
                printf("!-- 取消訂單，回到主選單 --\n\n");
                return;
            }
        }
    }
}

int main()
{
    int choice;
    while(1)
    {
        printf("1. Shop list \n");
        printf("2. Order \n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        getchar(); // 清除換行符號

        if(choice == 1)
        {
            showShopList();
        }
        else if(choice == 2)
        {
            takeOrder();
        }
    }
    return 0;
}
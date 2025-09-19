#include <stdio.h>      // fprintf(), perror()
#include <stdlib.h>     // exit()
#include <string.h>     // memset()
#include <signal.h>    // signal()
#include <fcntl.h>     // open()
#include <unistd.h>    // read(), write(), close()

#include <sys/socket.h> // socket(), connect()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // htons()

int connfd, fd;

void sigint_handler(int signo)
{
	// 這裡的 signo (SIGINT) 用不到，之所以跳來這個 func 只是為了安全關閉 driver file
    close(fd);
    close(connfd);
}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        fprintf(stderr, "Usage: ./reader <server_ip> <port> <device_path>");
        exit(EXIT_FAILURE);  // exit(1)
    }

    signal(SIGINT, sigint_handler);

    if((connfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
		
	// 設定 Server socket 的 IP、Port number
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));  // 把 &cli_addr 上的資料都清空
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = inet_addr(argv[1]);
    cli_addr.sin_port = htons((u_short)atoi(argv[2]));

    if(connect(connfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) == -1)
    {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    if((fd = open(argv[3], O_RDWR)) < 0)
    {
        perror(argv[3]);
        exit(EXIT_FAILURE);
    }

    int ret;
    char buf[16] = {};

    while(1)
    {
        if((ret = read(fd, buf, sizeof(buf))) == -1)
        {
            perror("read()");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < 16; ++i)
        {
            printf("%c ", buf[i]);
        }
        printf("\n");

        if(write(connfd, buf, 16) == -1)
        {
            perror("write()");
            exit(EXIT_FAILURE);
        }
        sleep(1);
    }

    close(fd);
    close(connfd);
    return 0;
}

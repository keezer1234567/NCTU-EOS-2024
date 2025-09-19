#include <stdio.h>    // perror()
#include <stdlib.h>   // exit()
#include <fcntl.h>    // open()
#include <unistd.h>   // dup2()
#include <signal.h>   // signal()
#include <sys/wait.h> // waitpid()
#include <sys/socket.h> // socket()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_addr()

void handler(int signum)
{
    // = wait()
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
    int server_socket, client_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4444);

    // Force using socket address already in use
    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind socket error");
        close(server_socket);
        exit(1);
    }

    if(listen(server_socket, 1) == -1)
    {
        perror("listen port error");
        close(server_socket);
        exit(1);
    }

    signal(SIGCHLD, handler);

    while(1)
    {
        client_socket = accept(server_socket, NULL, NULL);

        pid_t childpid = fork();

        if(childpid == -1)
        {
            perror("fork error");
            exit(1);
        }
        else if(childpid == 0)
        {
            // child process
            printf("Train ID = %d \n", getpid());
            dup2(client_socket, STDOUT_FILENO);
            execlp("sl", "sl", "-l", (char *)NULL);
        }
        else
        {
            // parent process
            // trigger and than come back here
        }
    }
    return 0;
}

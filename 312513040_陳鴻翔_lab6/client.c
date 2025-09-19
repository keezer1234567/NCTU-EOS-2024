#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    const char* operation = argv[3];
    int amount = atoi(argv[4]);
    int times = atoi(argv[5]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    for(int i = 0; i < times; i++)
    {
        snprintf(buffer, BUFFER_SIZE, "%s %d", operation, amount);
        send(client_socket, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
    }

    close(client_socket);
    return 0;
}

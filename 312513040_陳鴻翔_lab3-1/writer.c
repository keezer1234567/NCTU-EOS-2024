#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>  // open()
#include <unistd.h> // read(), write()

int main(int argc, char *argv[])
{
    int fd;

    fd = open("/dev/etx_Dev", O_WRONLY);
    if (fd == -1)
    {
        perror("open error");
        return EXIT_FAILURE;
    }

    char *input = argv[1];

    for (int i = 0; input[i] != '\0'; i++)
    {
        int input_value = input[i] - '0';
        char data[4];
        
        for (int j = 3; j >= 0; j--)
        {
            if(((input_value >> j) & 1) == 1)
            {
                data[3 - j] = '1';
            }
            else
            {
                data[3 - j] = '0';
            }

            if(write(fd, data, 4) == -1)
            {
                perror("write error");
                close(fd);
                return EXIT_FAILURE;
            }
        }
        sleep(1);
    }

    close(fd);
    return EXIT_SUCCESS;
}
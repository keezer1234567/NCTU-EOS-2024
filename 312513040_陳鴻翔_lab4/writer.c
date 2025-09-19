#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>  // open()
#include <unistd.h> // read(), write()

int main(int argc, char *argv[])
{
    int fd;

    fd = open("/dev/my_dev", O_WRONLY);
    if(fd == -1)
    {
        perror("open error");
        return EXIT_FAILURE;
    }

    char *input = argv[1];

    for(int i = 0; input[i] != '\0'; i++)
    {
        if(write(fd, &input[i], 1) == -1)
        {
            perror("write error");
            close(fd);
            return EXIT_FAILURE;
        }
        sleep(1);
    }

    close(fd);
    return EXIT_SUCCESS;
}

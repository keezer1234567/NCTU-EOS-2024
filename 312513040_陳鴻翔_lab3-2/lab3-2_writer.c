#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>  // open()
#include <unistd.h> // read(), write()

u_int8_t seg_for_c[10] = 
{
    0b01111110, // 0
    0b00110000, // 1
    0b01101101, // 2
    0b01111001, // 3
    0b00110011, // 4
    0b01011011, // 5
    0b01011111, // 6
    0b01110000, // 7
    0b01111111, // 8
    0b01111011, // 9
};

int main(int argc, char *argv[])
{
    int fd;

    // 打開設備文件
    fd = open("/dev/etx_Dev", O_WRONLY);
    if (fd == -1)
    {
        perror("open error");
        return EXIT_FAILURE;
    }

    char *input = argv[1];

    for (int i = 0; input[i] != '\0'; i++)
    {
        int index = input[i] - '0';

        u_int8_t input_value = seg_for_c[index];
        char data[8];
            
        for (int j = 7; j >= 0; j--)
        {
            if(((input_value >> j) & 1) == 1)
            {
                data[7 - j] = '1';
            }
            else
            {
                data[7 - j] = '0';
            }
        }

        if(write(fd, data, 8) == -1)
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

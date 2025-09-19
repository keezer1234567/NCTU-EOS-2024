#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define DEVICE_FILE "/dev/button_driver"
#define BUFFER_SIZE 256

int fd;

void handle_sigint(int sig)
{
    // 發送 clear 給 driver
    if(write(fd, "clear", 5) < 0)
    {
        perror("Failed to clear buffer in driver");
    }
    close(fd);
    exit(0);
}

int main()
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // 開啟裝置檔案
    fd = open(DEVICE_FILE, O_RDWR);
    if(fd < 0)
    {
        perror("Failed to open the device");
        return errno;
    }

    // 設定信號處理
    signal(SIGINT, handle_sigint);

    while(1)
    {
        // 清空緩衝區
        memset(buffer, 0, BUFFER_SIZE);

        // 嘗試從裝置檔案讀取
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("Failed to read the device");
            close(fd);
            return errno;
        }

        // 如果有讀取到資料，印出按鈕資訊
        if(bytes_read > 0)
        {
            printf("%s", buffer);
        }
    }
    close(fd);
    return 0;
}
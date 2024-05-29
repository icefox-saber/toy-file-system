// FC.c
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"
//./FC 12356
// 定义输入命令结构体
#define BLOCK_SIZE 256
char tmp[BLOCK_SIZE];
typedef struct
{
    char type;             // 命令类型
    int block_id;          // 块号
    char info[BLOCK_SIZE]; // 信息
} disk_cmd;
int main(int argc, char *argv[])
{
    char *cmd_ptr;
    int port = 8086;
    tcp_client client = client_init("localhost", 8086);
    static char buf[4096];
    while (1)
    {
        disk_cmd disk_command;
        disk_command.type = 'I';
        disk_command.block_id = -1;
        cmd_ptr = (char *)&disk_command;
        client_send(client, cmd_ptr, sizeof(disk_command));

        // 清空临时缓冲区
        memset(tmp, 0, sizeof(tmp));
        // 接收响应到临时缓冲区
        int n = client_recv(client, tmp, sizeof(tmp)); // 这里出bug了
        buf[n] = 0;
        printf("%s\n", tmp);
        if (strcmp(buf, "Bye!") == 0) break;
    }
    client_destroy(client);
}

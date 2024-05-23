
#include "diskop.h"

// 外部磁盘服务器客户端
tcp_client client;

// 定义磁盘文件为一个二维字符数组
char disk_file[MAX_BLOCK_COUNT][BLOCK_SIZE];    //磁盘数据文件
// 定义日志文件指针
FILE *log_fp;

// 临时缓冲区
char tmp[TCP_BUF_SIZE];

// 定义输入命令结构体
typedef struct
{
    char type;             // 命令类型
    int block_id;          // 块号
    char info[BLOCK_SIZE]; // 信息
} disk_cmd;
// 定义一个缓冲区用于命令传输
char *cmd_ptr;
int write_block(int block_id, char *buf, int block_num)
{
    // 检查块号范围是否合法
    if (block_id + block_num - 1 < 0 || block_id + block_num > MAX_BLOCK_COUNT)
        return -1;
    for (int i = 0; i < block_num; i++)
    {
        // 创建一个写命令
        disk_cmd disk_command;
        disk_command.type = 'W';
        disk_command.block_id = block_id + i;
        // 将数据拷贝到命令结构体中
        memcpy(disk_command.info, buf + BLOCK_SIZE * i, BLOCK_SIZE);
        cmd_ptr=(char*)&disk_command;
        // 发送命令到磁盘服务器
        client_send(client, cmd_ptr, sizeof(disk_command));
        // 清空临时缓冲区
        memset(tmp, 0, sizeof(tmp));
        // 接收磁盘服务器的响应
        int n = client_recv(client, tmp, BLOCK_SIZE);

        if (n <= 0)
        {
            perror("Failed to receive response from disk server");
            return -1;
        }
    }
    return 0;
}

int read_block(int block_id, char *buf)
{
    // 检查块号是否合法
    if (block_id < 0 || block_id > MAX_BLOCK_COUNT)
        return -1;
    // 创建一个读命令
    disk_cmd disk_command;
    disk_command.type = 'R';
    disk_command.block_id = block_id;
    cmd_ptr=(char*)&disk_command;
    client_send(client, cmd_ptr, sizeof(disk_command));
    char tmpbuf[TCP_BUF_SIZE];

    // 清空缓冲区
    memset(tmpbuf, 0, TCP_BUF_SIZE);
    // 接收数据到缓冲区
    int n = client_recv(client, tmpbuf, sizeof(tmpbuf));
    memcpy(buf,tmpbuf,n);
    if (n <= 0)
    {
        perror("Failed to receive response from disk server");
        return -1;
    }

    return 0;
}

int get_disk_info()
{
    // 创建一个获取信息的命令
    disk_cmd disk_command;
    disk_command.type = 'I';
    disk_command.block_id = -1;
    cmd_ptr=(char*)&disk_command;
    client_send(client, cmd_ptr, sizeof(disk_command));

    // 清空临时缓冲区
    memset(tmp, 0, sizeof(tmp));
    // 接收响应到临时缓冲区
    int n = client_recv(client, tmp, sizeof(tmp));//这里出bug了
    if (n <= 0)
    {
        perror("Failed to receive response from disk server");
        return -1;
    }

    // 解析返回的磁盘信息
    int num1, num2;
    sscanf(tmp, "%d %d", &num1, &num2);
    return num1 * num2;
}
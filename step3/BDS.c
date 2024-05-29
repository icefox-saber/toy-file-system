#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tcp_utils.h"

// Block size in bytes
#define BLOCKSIZE 256

int ncyl, nsec, ttd;
int currentcyl = 0;
char *diskfname;
int fd;          // 文件描述符
char *disk_map;  // 映射到内存中的文件指针
size_t FILESIZE; // 文件大小
typedef struct
{
    char type;    // 命令类型
    int block_id; // 整除运算求cylinder,sector
    char data[BLOCKSIZE];
    /* data */
} cmd_t;
// return a negative value to exit
// information
// args和len没用，为了模板化加的参数

int cmd_i(tcp_buffer *write_buf, int block_id, char data[BLOCKSIZE])
{
    static char buf[64];
    sprintf(buf, "%d %d", ncyl, nsec);

    // send to buffer, including the null terminator
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
    return 0;
}

// read cylinder sector args len
int cmd_r(tcp_buffer *write_buf, int block_id, char data[BLOCKSIZE])
{
    static char *msg0 = "error command";
    static char buf[BLOCKSIZE];
    int cylinder =  block_id/nsec; // 这里不会检查溢出
    int sector = block_id%nsec;
    usleep(ttd * abs(currentcyl - cylinder));
    currentcyl = cylinder;
    // 计算文件偏移量
    long offset = (cylinder * nsec + sector) * BLOCKSIZE;

    if (offset + BLOCKSIZE > FILESIZE)
    {
        send_to_buffer(write_buf, "offset out of range", 19);
        return 0;
    }

    // 从映射的内存中读取数据
    memcpy(buf, disk_map + offset, BLOCKSIZE);



    // 发送数据到缓冲区
    send_to_buffer(write_buf, buf, BLOCKSIZE); // 发送 BLOCKSIZE + 3 字节数据
    return 0;
}

// W
// args:cylinder sector len data
// len:strlen(args)
int cmd_w(tcp_buffer *write_buf, int block_id, char data[BLOCKSIZE])
{
    static char *msg0 = "error command";
    int cylinder =  block_id/nsec; // 这里不会检查溢出
    int sector = block_id%nsec;
    usleep(ttd * abs(currentcyl - cylinder));
    currentcyl = cylinder;

    static char d[BLOCKSIZE];
    memcpy(d,data,BLOCKSIZE);
    usleep(ttd * abs(currentcyl - cylinder));
    currentcyl = cylinder;
    long offset = (cylinder * nsec + sector) * BLOCKSIZE;


    // 写入数据到映射的内存中
    memcpy(disk_map + offset, d, BLOCKSIZE);

    // 发送成功消息到缓冲区
    send_to_buffer(write_buf, "Yes", 4);
    return 0;
}

// exit
int cmd_e(tcp_buffer *write_buf, int block_id, char data[BLOCKSIZE])
{
    send_to_buffer(write_buf, "Bye!", 5);
    return -1;
}

// 这里cmd_table的4个元素的handler函数被赋值为命令名的对应函数
static struct
{
    const char *name;
    int (*handler)(tcp_buffer *write_buf, int block_id, char data[BLOCKSIZE]);
} cmd_table[] = {
    {"I", cmd_i},
    {"R", cmd_r},
    {"W", cmd_w},
    {"E", cmd_e},
};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void add_client(int id)
{
    // some code that are executed when a new client is connected
    // you don't need this in step1
}

int handle_client(int id, tcp_buffer *write_buf, char *input_cmd, int len)
{
    static cmd_t cmd;
    char *cmd_pointer=(char*)&cmd;
    memset(cmd_pointer, 0, sizeof(cmd));
    memcpy(cmd_pointer, input_cmd, sizeof(cmd));

    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (cmd.type == *(cmd_table[i].name))
        {
            ret = cmd_table[i].handler(write_buf, cmd.block_id, cmd.data);
            break;
        }
    if (ret == 1)
    {
        static char unk[] = "Unknown command";
        send_to_buffer(write_buf, unk, sizeof(unk));
    }
    if (ret < 0)
    {
        return -1;
    }

    return 0;
}

void clear_client(int id)
{
    // some code that are executed when a client is disconnected
    // you don't need this in step2
}

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        fprintf(stderr,
                "Usage: %s <disk file name> <cylinders> <sector per cylinder> "
                "<track-to-track delay> <port>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    // ./BDS name 3 8 10 8086
    // args
    diskfname = argv[1];
    ncyl = atoi(argv[2]);
    nsec = atoi(argv[3]);
    ttd = atoi(argv[4]); // ms
    int port = atoi(argv[5]);

    // open file
    char *filename = diskfname;
    fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // stretch the file
    FILESIZE = BLOCKSIZE * nsec * ncyl;
    int result = lseek(fd, FILESIZE - 1, SEEK_SET);
    if (result == -1)
    {
        perror("Error calling lseek() to 'stretch' the file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    result = write(fd, "", 1);
    if (result != 1)
    {
        perror("Error writing last byte of the file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // mmap
    disk_map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk_map == MAP_FAILED)
    {
        perror("Error mmapping the file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // command
    tcp_server server = server_init(port, 1, add_client, handle_client, clear_client);
    server_loop(server);

    // clean up
    if (munmap(disk_map, FILESIZE) == -1)
    {
        perror("Error un-mmapping the file");
    }
    close(fd);
}

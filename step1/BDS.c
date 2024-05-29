// ./BDS disk 15 15 2 8086
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>


#include "tcp_utils.h"
// 需要完成4个cmd函数
//  Block size in bytes
#define BLOCKSIZE 256
int ncyl, nsec, ttd;
int currentcyl = 0;
char *diskfname; //diskfname
FILE *fp;         //全局文件指针

// return a negative value to exit
// information
// args和len没用，为了模板化加的参数

int is_number(const char *str)
{
    while (*str)
    {
        if (!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

int cmd_i(tcp_buffer *write_buf, char *args, int len)
{
    static char buf[64];
    sprintf(buf, "%d %d", ncyl, nsec);

    // send to buffer, including the null terminator
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
    return 0;
}

// read cylinder sector args len
// read cylinder sector args len
// read cylinder sector args len
int cmd_r(tcp_buffer *write_buf, char *args, int len)
{
    static char *msg0 = "NO";
    static char buf[BLOCKSIZE];
    char *c = strtok(args, " ");
    char *s = strtok(NULL, " ");
    *(s+strlen(s)-1)=0;
    if (!c || !s || !is_number(c) || !is_number(s))
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0;
    }
    int cylinder = atoi(c);
    int sector = atoi(s);
    if (cylinder < 0 || cylinder >= ncyl || sector < 0 || sector >= nsec)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0;
    }

    usleep(ttd * abs(currentcyl - cylinder));
    currentcyl = cylinder;
    // 计算文件偏移量
    long offset = (cylinder * nsec + sector) * BLOCKSIZE;

    if (fseek(fp, offset, SEEK_SET) == -1)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0; // 或者返回错误代码
    }

    size_t num = fread(buf, 1, BLOCKSIZE, fp);
    if (num != BLOCKSIZE)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0; // 或者返回错误代码
    }

    static char response[BLOCKSIZE + 5];
    snprintf(response, sizeof(response), "Yes");
    memcpy(response + 4, buf, BLOCKSIZE);

    send_to_buffer(write_buf, response, BLOCKSIZE + 4);
    return 0;
}



//W
//args:cylinder sector len data
//len:strlen(args)
// args:cylinder sector len data
// len:strlen(args)
// args:cylinder sector len data
// len:strlen(args)
int cmd_w(tcp_buffer *write_buf, char *args, int len)
{
    static char *msg0 = "NO";
    char *c = strtok(args, " ");
    char *s = strtok(NULL, " ");
    char *l = strtok(NULL, " ");
    char *d = strtok(NULL, "\0");

    if (!c || !s || !l || !d || !is_number(c) || !is_number(s) || !is_number(l))
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0;
    }
    int cylinder = atoi(c);
    int sector = atoi(s);
    int datalen = atoi(l);

    if (cylinder < 0 || cylinder >= ncyl || sector < 0 || sector >= nsec || datalen < 0 || datalen > BLOCKSIZE)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0;
    }

    usleep(ttd * abs(currentcyl - cylinder));
    currentcyl = cylinder;
    long offset = (cylinder * nsec + sector) * BLOCKSIZE;

    if (fseek(fp, offset, SEEK_SET) == -1)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0; // 或者返回错误代码
    }

    char buffer[BLOCKSIZE] = {0};
    memcpy(buffer, d, datalen);

    size_t written = fwrite(buffer, 1, BLOCKSIZE, fp);
    if (written != BLOCKSIZE)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0));
        return 0; // 或者返回错误代码
    }

    send_to_buffer(write_buf, "Yes", 4);
    return 0;
}



// exit
int cmd_e(tcp_buffer *write_buf, char *args, int len)
{
    send_to_buffer(write_buf, "Bye!", 5);
    return -1;
}
// 这里cmd_table的4个元素的handler函数被赋值为命令名的对应函数
static struct
{
    const char *name;
    int (*handler)(tcp_buffer *write_buf, char *, int);
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

int handle_client(int id, tcp_buffer *write_buf, char *msg, int len)
{

    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (strcmp(p, cmd_table[i].name) == 0)
        {
            ret = cmd_table[i].handler(write_buf, p + strlen(p) + 1, len - strlen(p) - 1);
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
    //./BDS name 3 8 10 8086
    // args
    diskfname = argv[1];
    ncyl = atoi(argv[2]);
    nsec = atoi(argv[3]);
    ttd = atoi(argv[4]); // ms
    int port = atoi(argv[5]);
    // my code:

    // open file
    char *filename = diskfname;
    fp = fopen(filename, "w+");
    if (!fp)
    {
        printf("Error: Could not open file '%s'.\n", filename);
        exit(-1);
    }

    // stretch the file
    long FILESIZE = BLOCKSIZE * nsec * ncyl;
    int result = fseek(fp, FILESIZE - 1, SEEK_SET);
    if (result == -1)
    {
        perror("Error calling fseek() to 'stretch' the file");
        fclose(fp);
        exit(-1);
    }
    result = fwrite("", 1, 1, fp);
    if (result != 1)
    {
        perror("Error writing last byte of the file");
        fclose(fp);
        exit(-1);
    }

    // mmap
    // 这里应该在command里写
    // command
    tcp_server server = server_init(port, 1, add_client, handle_client, clear_client);
    server_loop(server);
}

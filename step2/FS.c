#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <regex.h>
#include "tcp_utils.h"
#include "superblock.h"
#include "inode.h"
#include "diskop.h"
#include "dir.h"
//./FS localhost 10356 12356
#define COMMAND_LENGTH 4096
#define RESPONSE_LENGTH 4096
#define MAX_PATH_STR_LENGTH 128 // inode_num(1024)*(max_name_size(28)+'/'(1)）
dir_item dir_items[8];          // 256bytes
int server_socket;
int disk_server_socket;
char response[RESPONSE_LENGTH] = "";

uint16_t current_dir[256] = {65535};                   // 当前目录结点编号
char cur_path_string[256][MAX_PATH_STR_LENGTH]; // 当前目录字符串
uint8_t uid[256];
bool formatted = false; // 是否格式化

/*增加到response*/
void _printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int current_length = strlen(response);

    // 格式化并追加
    vsnprintf(response + current_length, RESPONSE_LENGTH - current_length, format, args);

    va_end(args);
}

/** @brief 检查名称合法性（只允许数字、字母、_和.）
 * @return 返回1表示不合法，0表示合法 */
int check_name(const char *text)
{
    // 定义正则表达式
    const char *pattern = "^[a-zA-Z0-9_.]+$";
    regex_t regex;
    int reti;

    // 编译正则表达式
    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti)
    {
        fprintf(stderr, "Could not compile regex\n");
        return 1;
    }

    // 执行正则表达式
    reti = regexec(&regex, text, 0, NULL, 0);
    if (!reti)
    {
        // 检查名称长度
        if (strlen(text) > MAX_NAME_SIZE)
        {
            _printf("Illegal name! (Must be shorter than 28)\n");
            regfree(&regex);
            return 1;
        }

        // 合法名称
        regfree(&regex);
        return 0;
    }
    else if (reti == REG_NOMATCH)
    {
        _printf("Illegal name! (Only accept numbers, letters, _ and .)\n");
    }
    else
    {
        char msgbuf[100];
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    }

    // 释放正则表达式对象
    regfree(&regex);
    return 1;
}

/** @brief 初始化文件系统 */
int format_handle(char *params, uint8_t owner)
{
    if (owner != 0)
    {
        _printf("only user uid:0 can format");
        return 0;
    }
    if (init_spb() < 0)
    { // 初始化super block
        _printf("Init superblock failed. Initializing this file system requires at least 132 blocks of connected disks.\n");
        return 0;
    }
    if (init_root_inode() < 0)
    { // 初始化root inode
        _printf("Init root inode failed.\n");
        return 0;
    };
    current_dir[owner] = 0; // 初始化当前目录为root
    strcpy(cur_path_string[owner], "/");
    for (int i = 1; i < MAX_INODE_COUNT; i++)
    { // 初始化inode table
        if (init_inode(&inode_table[i], i, 0, 0, 0, 65535, 0, 0) < 0 || write_inode(&inode_table[i], i) < 0)
        {
            _printf("Init inode failed.\n");
            return 0;
        }
    }
    formatted = true;
    return 0;
}
/** @brief 检查格式化并在非格式化时_printf("Please first format.\n")*/
int check_formatted()
{
    if (!formatted)
    {
        _printf("Please first format.\n");
        return 0;
    }
    return 1;
}
/** @brief 创建文件
 * @param params 格式<f>，f为文件名*/
int mk_handle(char *params, uint8_t owner)
{
    if (!check_formatted())
        return 0;
    char name[MAX_NAME_SIZE];
    char *p = strtok(params, " ");
    strcpy(name, p);
    p = strtok(NULL, " ");
    uint8_t lock;
    if (p)
    {
        lock = atoi(p);
    }
    else
    {
        lock = 0;
    }
    if (check_name(params))
    {
        return 0;
    } // 文件名非法
    if (search_in_dir_inode(&inode_table[current_dir[owner]], params, 0, 0) >= 0)
    { // 重名
        _printf("A file with the same name already exists!\n");
        return 0;
    }

    if (!(lock <= 2) || !(lock >= 0))
    {
        _printf("lock level error");
        return 0;
    }
    add_to_dir_inode(&inode_table[current_dir[owner]], name, 0, owner, lock);
    return 0;
}

/** @brief 创建文件夹
 * @param params 格式<f>，f为文件夹名*/
int mkdir_handle(char *params, uint8_t owner)
{
    if (!check_formatted())
        return 0;
    char name[MAX_NAME_SIZE];
    char *p = strtok(params, " ");
    strcpy(name, p);
    p = strtok(NULL, " ");
    uint8_t lock;
    if (p)
    {
        lock = atoi(p);
    }
    else
    {
        lock = 0;
    }
    if (lock > 2 || lock < 0)
    {
        _printf("lock level error");
        return 0;
    }
    if (check_name(params))
    {
        return 0;
    } // 文件夹名非法
    if (search_in_dir_inode(&inode_table[current_dir[owner]], params, 1, 0) >= 0)
    { // 重名
        _printf("A sub-dir with the same name already exists!\n");
        return 0;
    }

    add_to_dir_inode(&inode_table[current_dir[owner]], name, 1, owner, lock);
    return 0;
}

/** @brief 删除文件
 * @param params 格式<f>，f为文件名*/
int rm_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    if (check_name(params))
    {
        return 0;
    } // 文件名非法
    rm_from_dir_inode(&inode_table[current_dir[user]], params, user);
    return 0;
}

/** @brief 更改路径
 * @param params 格式<f>，f为路径字符串*/
int cd_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;

    uint16_t ori_dir = current_dir[user]; // 保存当前目录ID和路径字符串
    char ori_path_string[MAX_PATH_STR_LENGTH];
    strcpy(ori_path_string, cur_path_string[user]);

    if (params[0] == '/')
    {
        // 处理绝对路径
        current_dir[user] = 0; // 根目录ID，假设根目录ID为0
        strcpy(cur_path_string[user], "/");
        params++; // 跳过第一个 '/'
    }

    char *path = strtok(params, "/");
    while (path != NULL)
    {
        if (strcmp(path, "..") == 0)
        { // 回到上一级
            uint16_t parent = inode_table[current_dir[user]].i_parent;
            if (parent != 65535)
            { // 65535表示无父目录或根目录
                current_dir[user] = parent;

                if (strcmp(cur_path_string[user], "/") != 0)
                {
                    char *last_slash = strrchr(cur_path_string[user], '/');
                    if (last_slash != NULL)
                        *last_slash = '\0';
                }
                if (parent == 0)
                    strcpy(cur_path_string[user], "/");
            }
        }
        else if (strcmp(path, ".") != 0)
        { // . 代表当前目录，无视之
            int result = search_in_dir_inode(&inode_table[current_dir[user]], path, 1, user);
            if (result == -1)
            {
                _printf("Directory not found.\n");
                current_dir[user] = ori_dir; // 找不到对应文件夹，回退
                strcpy(cur_path_string[user], ori_path_string);
                return 0;
            }
            if (current_dir[user] != 0 && strcmp(cur_path_string[user], "/") != 0)
                strcat(cur_path_string[user], "/");
            current_dir[user] = result;
            strcat(cur_path_string[user], path);
        }
        path = strtok(NULL, "/");
    }
    return 0;
}

/** @brief 删除文件夹
 * @param params 格式<f>，f为文件夹名*/
int rmdir_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    if (check_name(params))
    {
        return 0;
    } // 文件名非法
    if (rmdir_from_dir_inode(&inode_table[current_dir[user]], params, user) < 0)
    {
        _printf("Failed: the possible reason is the dir is not empty!\n");
    }
    return 0;
}

/** @brief 列出文件夹和文件名称*/
int ls_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    char buf[64 * MAX_NAME_SIZE];
    char buf2[64 * MAX_NAME_SIZE];
    bool detailed = false;
    if (params && strcmp(params, "-l") == 0)
    {
        detailed = true;
    }
    if (ls_dir_inode(&inode_table[current_dir[user]], buf, buf2, detailed, user) == 0)
    {
        _printf("%s", buf2);
        printf("%s", buf2);
    }
    return 0;
}

/** @brief 读取文件内容
 * @param params 格式<f>，f为文件名*/
int cat_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    // char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char buf[65535];
    memset(buf, 0, sizeof(buf));
    int ret_id;
    if (check_name(params))
    {
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir[user]], params, 0, user)) < 0)
    { // 文件不存在
        _printf("File does not exist!\n");
        return 0;
    }
    if (read_file_inode(&inode_table[ret_id], buf, user) == 0)
    {
        _printf(buf);
        _printf("\n");
    }
    else
        return 0;
}

/** @brief 读取文件内容
 * @param params 格式<f> <l> <data>，f为文件名，l为长度，data为写入数据*/
int w_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0;
    int len = 0;
    memset(buf, 0, sizeof(buf));
    sscanf(params, "%s %d %[^\n]", file_name, &len, buf);
    if (check_name(file_name) || (len == 0))
    {
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir[user]], file_name, 0, user)) < 0)
    { // 文件不存在
        _printf("File does not exist!\n");
        return 0;
    }
    if (len != strlen(buf))
    {
        _printf("\033[1;33mWarning: The string length parameter does not match the actual length!\033[0m\n");
        len = strlen(buf);
    }
    if (write_file_inode(&inode_table[ret_id], buf, user) != 0)
    {
        _printf("Failed. Disk space may be insufficient\n");
    }
    return 0;
}

/** @brief 插入
 * @param params 格式<f> <pos> <l> <data>，f为文件名，pos为位置，l为长度，data为写入数据*/
int i_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char src[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0, len = 0, pos = 0;
    memset(src, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    sscanf(params, "%s %d %d %[^\n]", file_name, &pos, &len, src);
    if (check_name(file_name) || (len == 0))
    {
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir[user]], file_name, 0, user)) < 0)
    { // 文件不存在
        _printf("File does not exist!\n");
        return 0;
    }
    if (len != strlen(src))
    {
        _printf("\033[1;33mWarning: The string length parameter does not match the actual length!\033[0m\n");
        len = strlen(src);
    }

    int file_size = inode_table[ret_id].i_size; // 获取文件当前的大小
    if (pos > file_size)
    {
        _printf("\033[1;33mWarning: The <pos> parameter is greater than the file size and is appended to the end of the file!\033[0m\n");
        pos = file_size; // 如果 pos 大于文件大小，则将数据插入到文件末尾
    }
    int new_size = file_size + len;
    if (new_size > MAX_LINKS_PER_NODE * BLOCK_SIZE - 1)
    {
        return 0;
    } // 过长
    memset(buf, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    int ret = read_file_inode(&inode_table[ret_id], buf, user);
    if (ret < 0)
    {
        return 0;
    } // 读取文件内容失败，处理错误情况

    // 在指定位置插入数据
    memmove(buf + pos + len, buf + pos, file_size - pos); // 向后移动数据
    memcpy(buf + pos, src, len);                          // 插入数据
    buf[new_size] = '\0';

    if (write_file_inode(&inode_table[ret_id], buf, user) != 0)
    {
        _printf("Failed.\n");
    }
    return 0;
}

/** @brief 删除
 * @param params 格式<f> <pos> <l>，f为文件名，pos为位置，l为长度*/
int d_handle(char *params, uint8_t user)
{
    if (!check_formatted())
        return 0;
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char src[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0, len = 0, pos = 0;
    sscanf(params, "%s %d %d", file_name, &pos, &len);
    if (check_name(file_name) || (len == 0))
    {
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir[user]], file_name, 0, user)) < 0)
    { // 文件不存在
        _printf("File does not exist!\n");
        return 0;
    }

    int file_size = inode_table[ret_id].i_size; // 获取文件当前的大小
    if (pos > file_size)
    {
        return 0;
    }
    if (pos + len > file_size)
        len = file_size - pos;
    memset(buf, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    int ret = read_file_inode(&inode_table[ret_id], buf, user);
    if (ret < 0)
    {
        return 0;
    } // 读取文件内容失败，处理错误情况

    // 在指定位置删除数据

    memcpy(src, buf, pos);
    memcpy(src + pos, buf + pos + len, file_size - pos - len);
    src[file_size - len] = '\0';
    // _printf("buf: %s\n", buf);

    if (write_file_inode(&inode_table[ret_id], src, user) != 0)
    {
        _printf("Failed\n");
    }
    return 0;
}

/** @brief 退出系统 */
int exit_handle(char *params, uint8_t owner)
{
    _printf("Goodbye!\n");
    return -1; //  返回-1使main中循环退出
}
int ini_handle(char *params, uint8_t id)
{
    uint8_t newid = atoi(params);
    uid[id] = newid;
    if (!check_formatted())
    {
        _printf("!format");
        return 0;
    }
    current_dir[uid[id]] = 0;
    strcpy(cur_path_string[uid[id]], "/");
    char home[MAX_NAME_SIZE];
    snprintf(home, MAX_NAME_SIZE, "home%d", uid[id]);
    char mkd[MAX_NAME_SIZE];
    snprintf(mkd, MAX_NAME_SIZE, "home%d 2", uid[id]);
    mkdir_handle(mkd, uid[id]);
    cd_handle(home, uid[id]);
    return 1;
}
typedef struct Command
{
    char cmd_head[6];                           // 命令名称，对每行输入的首段进行匹配
    int (*handle)(char *params, uint8_t owner); // 处理该命令的函数(char *params,uint8_t owner)
} Command;

Command commands_list[] = {
    {"f", format_handle},
    {"mk", mk_handle},
    {"mkdir", mkdir_handle},
    {"rm", rm_handle},
    {"cd", cd_handle},
    {"rmdir", rmdir_handle},
    {"ls", ls_handle},
    {"cat", cat_handle},
    {"w", w_handle},
    {"i", i_handle},
    {"d", d_handle},
    {"e", exit_handle},
    {"ini",ini_handle},
};

const int num_commands = sizeof(commands_list) / sizeof(Command);

//
void add_client(int id)
{

    memset(response, 0, sizeof(response));
    // send_cur_path_string[owner](client_socket);

    if (load_spb() >= 0)
    { // 加载spb
        if (spb.s_magic == 0xE986 && check_spb() == 0)
        { // 已有文件系统且核对磁盘块数目正确
            formatted = true;
            for (int i = 0; i < MAX_INODE_COUNT; i++)
            {                                           // 写回inode table到diskfile
                if (read_inode(&inode_table[i], i) < 0) // 0是super user
                {
                    printf("Read Inode failed.\n");
                    formatted = false;
                    break;
                }
            }
            
            if (formatted)
                _printf("Already have formatted file system. We've loaded it.\n");
        }
    }
    else
    {
        _printf("Don't find formatted file system. Please first format.\n");
    }
}

int handle_client(int id, tcp_buffer *write_buf, char *msg, int len)
{
    char raw_command[COMMAND_LENGTH];
    char cmd_head[6];
    char params[COMMAND_LENGTH];
    int ret;
    memset(raw_command, 0, sizeof(raw_command));
    memset(response, 0, sizeof(response));
    memset(params, 0, sizeof(params));
    memcpy(raw_command, msg, sizeof(raw_command));
    bool handled = false;
    // 初始化响应字符串
    for (int i = 0; i < num_commands; i++)
    {
        sscanf(raw_command, "%s %[^\n]%*c", cmd_head, params);
        if (strcmp(cmd_head, commands_list[i].cmd_head) == 0)
        {
            ret = commands_list[i].handle(params, id);
            // if (!strlen(response)) _printf("Done.\n");
            handled = true;
            _printf(cur_path_string[uid[id]]);
            _printf("$");
            send_to_buffer(write_buf, response, strlen(response));
            return 0;
        }
    }
    char msg0[MAX_PATH_STR_LENGTH + 10] = "!handled\n";
    strcat(msg0, cur_path_string[uid[id]]);
    strcat(msg0, "$");
    if (!handled)
    {
        send_to_buffer(write_buf, msg0, strlen(msg0) + 1);
        return 0;
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
    if (argc < 3)
    {
        fprintf(stderr,
                "Usage: %s <disk file name> <cylinders> <sector per cylinder> "
                "<track-to-track delay> <port>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    log_file = fopen("fs.log", "w");
    int port_server = atoi(argv[2]);
    int port_client = atoi(argv[1]);
    client = client_init("localhost", port_client);
    tcp_server server = server_init(port_server, 1, add_client, handle_client, clear_client);
    static char buf[4096];

    server_loop(server);
}

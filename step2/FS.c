#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "tcp_utils.h"
#define MAX_BLOCK_COUNT 4096
#define BLOCK_SIZE 256
#define TCP_BUF_SIZE 4096
extern FILE *log_fp;
extern char disk_file[MAX_BLOCK_COUNT][BLOCK_SIZE];    //磁盘数据文件
extern tcp_client client;
/**
 * @brief 将数据写入块
 * @return 返回0代表成功，-1代表失败
 * @param block_no--写入起始块号
 * @param buf--要写入的数据
 * @param block_num--写入块的数量（默认连续写入且不做覆盖检查，检查应在上游函数完成） 
 */
int write_block(int block_no, char *buf, int block_num);

/**
 * @brief 将块中数据读出
 * @return 返回0代表成功，-1代表失败
 * @param block_no--读数据的块号
 * @param buf--读出数据写入buf
 */
int read_block(int block_no, char *buf);

/**
 * @brief 查询磁盘信息
 * @return 返回-1失败，正数则为磁盘总块数
*/
int get_disk_info();
#define BLOCK_SIZE 256
#define MAX_INODE_MAP 32  
#define MAX_BLOCK_MAP 128
#define MAX_INODE_COUNT 1024            
#define MAX_BLOCK_COUNT 4096            
#define RESERVE_BLOCK 131
#define SUPERBLOCK_SIZE 3
#define MAGIC_NUMBER 0xE986
#define INITIAL_USED_BLOCKS 131

typedef struct super_block {
    uint16_t s_magic;              
    uint32_t s_inodes_count;       
    uint32_t s_blocks_count;       
    uint32_t s_free_inodes_count;  
    uint32_t s_free_blocks_count;  
    uint32_t s_root;               
    uint32_t block_map[MAX_BLOCK_MAP];  
    uint32_t inode_map[MAX_INODE_MAP];  
} super_block;

extern super_block spb;

int init_spb();
int load_spb();
int check_spb();
int alloc_block();
int free_block(uint16_t index);

#define MAX_NAME_SIZE 28

typedef struct dir_item {   
    uint16_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint8_t valid;              // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[MAX_NAME_SIZE];   // 目录项表示的文件/目录的文件名/目录名
} dir_item;  // 32bytes

extern dir_item dir_items[8]; //256bytes

#define INODE_PER_BLOCK 8
#define INODE_TABLE_START_BLOCK 3 
#define MAX_LINKS_PER_NODE 136          //8 for direct, 128 for single-indirect
#define DIR_ITEM_PER_BLOCK 8
#define DIR_ITEM_NAME_LENGTH MAX_NAME_SIZE
typedef struct inode{
    uint8_t i_mode;                     //0代表文件，1代表目录
    uint8_t i_link_count;               //已有的连接数量
    uint16_t i_size;                    //记录文件大小
    uint32_t i_timestamp;               //记录文件修改时间
    uint16_t i_parent;                  //父inode编号（65535表示无）
    uint16_t i_direct[8];               //直接块（记录block编号，block编号统一使用uine16_t）
    uint16_t i_single_indirect;         //一层间接块
    uint16_t reserve;
    uint16_t i_index;
} inode; //32 bytes

extern inode inode_table[MAX_INODE_COUNT];

/**
 * @brief 初始化root inode节点
 * @return 返回0代表成功，-1代表失败
 */
int init_root_inode();

/**
 * @brief 初始化inode节点
 * @return 返回0代表成功，-1代表失败
 */
int init_inode(inode* node, uint16_t index, uint8_t mode, uint8_t link, uint16_t size, uint16_t parent);

/**
 * @brief 写inode到inode_table的block
 * @param node 要写的inode数据
 * @param index 要写的inode在inode_table中的编号
 * @return 返回0代表成功，-1代表失败
 */
int write_inode(inode* node, uint16_t index);

/**
 * @brief 读disk的inode_table
 * @param node 要读取的inode数据
 * @param index 要读取的inode在inode_table中的编号
 * @return 返回0代表成功，-1代表失败
 */
int read_inode(inode* node, uint16_t index);

/**
 * @brief 寻找并分配空闲inode节点
 * @return 返回大于0代表成功（即新inode编号），-1代表失败
 */
int alloc_inode();

/**
 * @brief 将已分配inode节点还原
 * @param node 节点指针
 * @return 返回0代表成功，-1代表失败，1代表本块本身已经空闲
 */
int free_inode(inode* node);

/**
 * @brief 在目录结点下新增文件或文件夹
 * @param dir_node 当前所在位置dir_inode指针
 * @param name 要新建dir_item的名称
 * @param type 要新建dir_item的种类（0代表文件，1代表目录）
 * @return 返回0代表成功，-1代表失败
*/
int add_to_dir_inode(inode* dir_node, char* name, uint8_t type);

/**
 * @brief 从文件夹中删除文件dir_item
 * @param dir_node 目标路径dir_inode指针
 * @param name 要删除dir_item的名称
 * @return 返回0代表成功，-1代表失败
*/
int rm_from_dir_inode(inode* dir_node, char* name);

/**
 * @brief 从文件夹中删除文件夹dir_item
 * @param dir_node 目标路径dir_inode指针
 * @param name 要删除dir_item的名称
 * @return 返回0代表成功，-1代表失败
*/
int rmdir_from_dir_inode(inode* dir_node, char* name);

/**
 * @brief 检查dir_node是否为空，若空（无block或block中全valid = 0）则删除并归还block
 * @param dir_node 目标路径dir_inode指针
 * @return 返回0代表成功，-1代表失败
*/
int check_dir_isempty_delete(inode* dir_node);

/**
 * @brief 用于按字典序std-qsort时作为cmp函数
*/
int lexic_cmp(const void *a, const void *b);

/**
 * @brief 列出dir下所有文件夹和文件
 * @param dir_node 目标路径dir_inode指针
 * @param ret 结果字符串(写入log 不带颜色)
 * @param ret2 结果字符串(显示 带颜色)
 * @param detailed true表示ls -l命令,false表示ls
 * 
 * @return 返回0代表成功，-1代表失败或空
*/
int ls_dir_inode(inode *dir_node, char *ret, char *ret2, bool detailed);

/**
 * @brief 按名称从文件夹寻找子文件夹或子文件
 * @param dir_node 目标路径dir_inode指针
 * @param name 目标dir_item名称
 * @return 返回大于等于0代表成功（即子项的inode_id），失败返回-1
*/
int search_in_dir_inode(inode* dir_node, char* name, int type);

/**
 * @brief 读文件inode的内容
 * @param file_node 目标文件file_inode指针
 * @param ret 将读取结果返回的字符串
 * @return 返回0代表成功，失败返回-1
*/
int read_file_inode(inode* file_node, char* ret);

/**
 * @brief 写文件inode的内容
 * @param file_node 目标文件file_inode指针
 * @param src 要写入的字符串
 * @return 返回0代表成功，失败返回-1
*/
int write_file_inode(inode* file_node, char* src);


//./FS localhost 10356 12356
#define COMMAND_LENGTH 4096
#define RESPONSE_LENGTH 4096
#define MAX_PATH_STR_LENGTH 128 // inode_num(1024)*(max_name_size(28)+'/'(1)）
dir_item dir_items[8];          // 256bytes
int server_socket;
int disk_server_socket;
char response[RESPONSE_LENGTH] = "";

uint16_t current_dir = 65535;                    // 当前目录结点编号
char cur_path_string[MAX_PATH_STR_LENGTH] = "$"; // 当前目录字符串
bool formatted = false;                          // 是否格式化

/*增加到response*/
void responsecat(const char *format, ...)
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
int check_name(char *text)
{
    int i = 0;
    char a = text[0];
    while (a != '\0')
    {
        if ((a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z') || (a >= '0' && a <= '9') || a == '_' || a == '.')
        {
            a = text[++i];
        }
        else
        {
            responsecat("Illegal name! (Only accept numbers ,letters ,_ and . )\n");
            return 1;
        }
    }
    if (i > MAX_NAME_SIZE)
    {
        responsecat("Illegal name! (Must be shorter than 28)\n");
        return 1;
    }
    return 0;
}

/** @brief 初始化文件系统 */
int format_handle(char *params)
{
    if (init_spb() < 0)
    { // 初始化super block
        responsecat("Init superblock failed. Initializing this file system requires at least 132 blocks of connected disks.\n");
        return 0;
    }
    if (init_root_inode() < 0)
    { // 初始化root inode
        responsecat("Init root inode failed.\n");
        return 0;
    };
    current_dir = 0; // 初始化当前目录为root
    strcpy(cur_path_string, "/");
    for (int i = 1; i < MAX_INODE_COUNT; i++)
    { // 初始化inode table
        if (init_inode(&inode_table[i], i, 0, 0, 0, 65535) < 0)
        {
            responsecat("Init inode failed.\n");
            return 0;
        }
    }
    for (int i = 0; i < MAX_INODE_COUNT; i++)
    { // 写回inode table到diskfile
        if (write_inode(&inode_table[i], i) < 0)
        {
            responsecat("Write Inode failed.\n");
            return 0;
        }
    }
    fprintf(log_fp, "[INFO] Format Done\n");
    formatted = true;
    return 0;
}

/** @brief 创建文件
 * @param params 格式<f>，f为文件名*/
int mk_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    if (check_name(params))
    {
        fprintf(log_fp, "[ERROR] 'mk' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if (search_in_dir_inode(&inode_table[current_dir], params, 0) >= 0)
    { // 重名
        fprintf(log_fp, "[ERROR] 'mk' command error: A file with the same name already exists!\n");
        responsecat("A file with the same name already exists!\n");
        return 0;
    }
    if (add_to_dir_inode(&inode_table[current_dir], params, 0) < 0)
        fprintf(log_fp, "[ERROR] 'mk' command error: Add new inode failed!\n");
    else
        fprintf(log_fp, "[INFO] 'mk' executed sucessfully.\n");
    return 0;
}

/** @brief 创建文件夹
 * @param params 格式<f>，f为文件夹名*/
int mkdir_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    if (check_name(params))
    {
        fprintf(log_fp, "[ERROR] 'mk' command error: Illegal dir name!\n");
        return 0;
    } // 文件夹名非法
    if (search_in_dir_inode(&inode_table[current_dir], params, 1) >= 0)
    { // 重名
        fprintf(log_fp, "[ERROR] 'mkdir' command error: A sub-dir with the same name already exists!\n");
        responsecat("A sub-dir with the same name already exists!\n");
        return 0;
    }
    if (add_to_dir_inode(&inode_table[current_dir], params, 1) < 0)
        fprintf(log_fp, "[ERROR] 'mkdir' command error: Add new inode failed!\n");
    else
        fprintf(log_fp, "[INFO] 'mkdir' executed sucessfully.\n");
    return 0;
}

/** @brief 删除文件
 * @param params 格式<f>，f为文件名*/
int rm_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    if (check_name(params))
    {
        fprintf(log_fp, "[ERROR] 'rm' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if (rm_from_dir_inode(&inode_table[current_dir], params) < 0)
        fprintf(log_fp, "[ERROR] 'rm' command error\n");
    else
        fprintf(log_fp, "[INFO] 'rm' executed sucessfully.\n");
    return 0;
}

/** @brief 更改路径
 * @param params 格式<f>，f为路径字符串*/
int cd_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }

    uint16_t ori_dir = current_dir; // 先保存当前dir_id和路径字符串
    char ori_path_string[MAX_PATH_STR_LENGTH];
    strcpy(ori_path_string, cur_path_string);

    char *path = strtok(params, "/");
    while (path != NULL)
    {
        if (strcmp(path, "..") == 0)
        { // 回到上一级
            uint16_t parent = inode_table[current_dir].i_parent;
            if (parent != 65535)
            { // 65535为无父母或为root
                current_dir = parent;

                if (strcmp(cur_path_string, "/") != 0)
                {
                    char *last_slash = strrchr(cur_path_string, '/');
                    *last_slash = '\0';
                }
                if (parent == 0)
                    strcpy(cur_path_string, "/");
            }
        }
        else if (strcmp(path, ".") != 0)
        { //.代表当前目录，无视之
            int result = search_in_dir_inode(&inode_table[current_dir], path, 1);
            if (result == -1)
            {
                responsecat("Directory not found.\n");
                current_dir = ori_dir; // 找不到对应文件夹，回退
                strcpy(cur_path_string, ori_path_string);
                return 0;
            }
            if (current_dir != 0)
                strcat(cur_path_string, "/");
            current_dir = result;
            strcat(cur_path_string, path);
        }
        path = strtok(NULL, "/");
    }
    return 0;
}

/** @brief 删除文件夹
 * @param params 格式<f>，f为文件夹名*/
int rmdir_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    if (check_name(params))
    {
        fprintf(log_fp, "[ERROR] 'rmdir' command error: Illegal dir name!\n");
        return 0;
    } // 文件名非法
    if (rmdir_from_dir_inode(&inode_table[current_dir], params) < 0)
    {
        fprintf(log_fp, "[ERROR] 'rmdir' command error\n");
        responsecat("Failed: the possible reason is the dir is not empty!\n");
    }
    else
        fprintf(log_fp, "[INFO] 'rmdir' executed sucessfully.\n");
    return 0;
}

/** @brief 列出文件夹和文件名称*/
int ls_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    char buf[64 * MAX_NAME_SIZE];
    char buf2[64 * MAX_NAME_SIZE];
    bool detailed = false;
    if (params && strcmp(params, "-l") == 0)
    {
        detailed = true;
    }
    if (ls_dir_inode(&inode_table[current_dir], buf, buf2, detailed) == 0)
    {
        fprintf(log_fp, "[DEBUG] response of 'ls': %s", buf);
        if(detailed)
            buf2[strlen(buf2)-13]=0;//手动修正多一个空格
        responsecat("%s", buf2);
        printf("%s", buf2);
    }
    return 0;
}


/** @brief 读取文件内容
 * @param params 格式<f>，f为文件名*/
int cat_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    // char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char buf[65535];
    memset(buf, 0, sizeof(buf));
    int ret_id;
    if (check_name(params))
    {
        fprintf(log_fp, "[ERROR] 'cat' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir], params, 0)) < 0)
    { // 文件不存在
        fprintf(log_fp, "[ERROR] 'cat' command error: File does not exist!\n");
        responsecat("File does not exist!\n");
        return 0;
    }
    if (read_file_inode(&inode_table[ret_id], buf) == 0)
    {
        fprintf(log_fp, "[DEBUG] cat response:%s\n", buf);
        responsecat(buf);
        responsecat("\n");
    }
    else
        fprintf(log_fp, "[ERROR] 'cat' command error\n");
    return 0;
}

/** @brief 读取文件内容
 * @param params 格式<f> <l> <data>，f为文件名，l为长度，data为写入数据*/
int w_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0;
    int len = 0;
    memset(buf, 0, sizeof(buf));
    sscanf(params, "%s %d %[^\n]", file_name, &len, buf);
    if (check_name(file_name) || (len == 0))
    {
        fprintf(log_fp, "[ERROR] 'w' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir], file_name, 0)) < 0)
    { // 文件不存在
        fprintf(log_fp, "[ERROR] 'w' command error: File does not exist!\n");
        responsecat("File does not exist!\n");
        return 0;
    }
    if (len != strlen(buf))
    {
        responsecat("\033[1;33mWarning: The string length parameter does not match the actual length!\033[0m\n");
        len = strlen(buf);
    }
    if (write_file_inode(&inode_table[ret_id], buf) == 0)
        fprintf(log_fp, "[INFO] 'w' executed sucessfully.\n");
    else
    {
        fprintf(log_fp, "[ERROR] 'w' command error: Disk space may be insufficient.\n");
        responsecat("Failed. Disk space may be insufficient\n");
    }
    return 0;
}

/** @brief 插入
 * @param params 格式<f> <pos> <l> <data>，f为文件名，pos为位置，l为长度，data为写入数据*/
int i_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char src[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0, len = 0, pos = 0;
    memset(src, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    sscanf(params, "%s %d %d %[^\n]", file_name, &pos, &len, src);
    if (check_name(file_name) || (len == 0))
    {
        fprintf(log_fp, "[ERROR] 'i' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir], file_name, 0)) < 0)
    { // 文件不存在
        fprintf(log_fp, "[ERROR] 'i' command error: File does not exist!\n");
        responsecat("File does not exist!\n");
        return 0;
    }
    if (len != strlen(src))
    {
        responsecat("\033[1;33mWarning: The string length parameter does not match the actual length!\033[0m\n");
        len = strlen(src);
        fprintf(log_fp, "[WARNING] 'i' command warning: The string length parameter does not match the actual length\n");
    }

    int file_size = inode_table[ret_id].i_size; // 获取文件当前的大小
    if (pos > file_size)
    {
        responsecat("\033[1;33mWarning: The <pos> parameter is greater than the file size and is appended to the end of the file!\033[0m\n");
        fprintf(log_fp, "[WARNING] 'i' command warning: Invalid pos\n");
        pos = file_size; // 如果 pos 大于文件大小，则将数据插入到文件末尾
    }
    int new_size = file_size + len;
    if (new_size > MAX_LINKS_PER_NODE * BLOCK_SIZE - 1)
    {
        fprintf(log_fp, "[ERROR] 'i' command error: data too long.\n");
        return 0;
    } // 过长
    memset(buf, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    int ret = read_file_inode(&inode_table[ret_id], buf);
    if (ret < 0)
    {
        fprintf(log_fp, "[ERROR] 'i' command error\n");
        return 0;
    } // 读取文件内容失败，处理错误情况

    // 在指定位置插入数据
    memmove(buf + pos + len, buf + pos, file_size - pos); // 向后移动数据
    memcpy(buf + pos, src, len);                          // 插入数据
    buf[new_size] = '\0';

    if (write_file_inode(&inode_table[ret_id], buf) == 0)
        fprintf(log_fp, "[INFO] 'i' executed sucessfully.\n");
    else
    {
        fprintf(log_fp, "[ERROR] 'i' command error\n");
        responsecat("Failed.\n");
    }
    return 0;
}

/** @brief 删除
 * @param params 格式<f> <pos> <l>，f为文件名，pos为位置，l为长度*/
int d_handle(char *params)
{
    if (!formatted)
    {
        responsecat("Please first format.\n");
        return 0;
    }
    char buf[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char src[MAX_LINKS_PER_NODE * BLOCK_SIZE];
    char file_name[MAX_NAME_SIZE];
    int ret_id = 0, len = 0, pos = 0;
    sscanf(params, "%s %d %d", file_name, &pos, &len);
    if (check_name(file_name) || (len == 0))
    {
        fprintf(log_fp, "[ERROR] 'd' command error: Illegal file name!\n");
        return 0;
    } // 文件名非法
    if ((ret_id = search_in_dir_inode(&inode_table[current_dir], file_name, 0)) < 0)
    { // 文件不存在
        fprintf(log_fp, "[ERROR] 'd' command error: File does not exist!\n");
        responsecat("File does not exist!\n");
        return 0;
    }

    int file_size = inode_table[ret_id].i_size; // 获取文件当前的大小
    if (pos > file_size)
    {
        fprintf(log_fp, "[ERROR] 'd' command error: Invalid pos\n");
        return 0;
    }
    if (pos + len > file_size)
        len = file_size - pos;
    memset(buf, 0, MAX_LINKS_PER_NODE * BLOCK_SIZE);
    int ret = read_file_inode(&inode_table[ret_id], buf);
    if (ret < 0)
    {
        fprintf(log_fp, "[ERROR] 'd' command error\n");
        return 0;
    } // 读取文件内容失败，处理错误情况

    // 在指定位置删除数据

    memcpy(src, buf, pos);
    memcpy(src + pos, buf + pos + len, file_size - pos - len);
    src[file_size - len] = '\0';
    // responsecat("buf: %s\n", buf);

    if (write_file_inode(&inode_table[ret_id], src) == 0)
        fprintf(log_fp, "[INFO] 'd' executed sucessfully.\n");
    else
    {
        fprintf(log_fp, "[ERROR] 'd' command error\n");
        responsecat("Failed\n");
    }
    return 0;
}

/** @brief 退出系统 */
int exit_handle(char *params)
{
    responsecat("Goodbye!\n");
    fprintf(log_fp, "[INFO] Exited.\n");
    fclose(log_fp);
    return -1; //  返回-1使main中循环退出
}

/** @brief 显示块内容
 * @param params 格式<id>，块id*/
int debug_getb_handle(char *params)
{
    int id;
    sscanf(params, "%d", &id);
    if (id < 0 || id >= MAX_BLOCK_COUNT)
        return 0;
    char buf[TCP_BUF_SIZE];
    read_block(id, buf);
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (buf[i] == '\0')
            responsecat("\033[1;34m$\033[0m");
        else
            responsecat("%c", buf[i]);
    }
    responsecat("\n\033[1;34m$\033[0m represents'\\0'\n");
    return 0;
}

/** @brief 显示inode内容
 * @param params 格式<id>，inode id*/
int debug_geti_handle(char *params)
{
    int id;
    sscanf(params, "%d", &id);
    if (id < 0 || id >= MAX_INODE_COUNT)
        return 0;
    inode *node = &inode_table[id];
    if (node->i_mode == 0)
    {
        responsecat("\033[1;33mThis is a file inode\033[0m\n");
        responsecat(" - index: %d\n", node->i_index);
        responsecat(" - mode: %d\n", node->i_mode);
        responsecat(" - link_count: %d\n", node->i_link_count);
        responsecat(" - size: %d\n", node->i_size);
        responsecat(" - timestamp: %d\n", node->i_timestamp);
        responsecat(" - parent: %d\n", node->i_parent);
        responsecat(" - direct: {");
        for (int i = 0; i < 8; i++)
            responsecat("%d,", node->i_direct[i]);
        responsecat("}\n - single_indirect: %d\n", node->i_single_indirect);
        if (node->i_single_indirect != 0)
        {
            char buf[TCP_BUF_SIZE];
            uint16_t indirect_blocks[128];
            read_block(node->i_single_indirect, buf);
            memcpy(&indirect_blocks, buf, BLOCK_SIZE);
            responsecat("{");
            for (int i = 0; i < 128; i++)
                responsecat("%d,", indirect_blocks[i]);
            responsecat("}\n");
        }
    }
    else
    {
        responsecat("\033[1;33mThis is a dir inode\033[0m\n");
        responsecat(" - index: %d\n", node->i_index);
        responsecat(" - mode: %d\n", node->i_mode);
        responsecat(" - link_count: %d\n", node->i_link_count);
        responsecat(" - size: invalid\n");
        responsecat(" - timestamp: %d\n", node->i_timestamp);
        if (node->i_parent == 65535)
            responsecat(" - parent: invalid\n");
        else
            responsecat(" - parent: %d\n", node->i_parent);
        responsecat(" - direct: {");
        for (int i = 0; i < 8; i++)
            responsecat("%d,", node->i_direct[i]);
        responsecat("}\n");
    }
    return 0;
}

typedef struct Command
{
    char cmd_head[6];            // 命令名称，对每行输入的首段进行匹配
    int (*handle)(char *params); // 处理该命令的函数
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
    {"gb", debug_getb_handle}, // DEBUG
    {"gi", debug_geti_handle}, // DEBUG
    {"e", exit_handle},
};

const int num_commands = sizeof(commands_list) / sizeof(Command);
void send_cur_path_string(tcp_buffer *write_buf)
{
    send_to_buffer(write_buf, cur_path_string, strlen(cur_path_string) + 1);
}

//
void add_client(int id)
{

    memset(response, 0, sizeof(response));
    // send_cur_path_string(client_socket);

    if (load_spb() >= 0)
    { // 加载spb
        if (spb.s_magic == 0xE986 && check_spb() == 0)
        { // 已有文件系统且核对磁盘块数目正确
            formatted = true;
            for (int i = 0; i < MAX_INODE_COUNT; i++)
            { // 写回inode table到diskfile
                if (read_inode(&inode_table[i], i) < 0)
                {
                    printf("Read Inode failed.\n");
                    formatted = false;
                    break;
                }
            }
            current_dir = 0;
            strcpy(cur_path_string, "/");
            if (formatted)
                responsecat("Already have formatted file system. We've loaded it.\n");
        }
    }
    else
    {
        responsecat("Don't find formatted file system. Please first format.\n");
    }
    //char *msg="/";
    //int ret=send(id, msg, strlen(msg)+1, 0);
    //if(ret==-1)
    //{
        //printf("error in add client");
    //}
    // some code that are executed when a new client is connected
    // you don't need this in step1
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
            ret = commands_list[i].handle(params);
            // if (!strlen(response)) responsecat("Done.\n");
            handled = true;
            responsecat(cur_path_string);
            send_to_buffer(write_buf,response,strlen(response)+1);
            return 0;
        }
    }
    char *msg0="!handled\n";
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
    log_fp = fopen("fs.log", "w");
    int port_server = atoi(argv[2]);
    int port_client = atoi(argv[1]);
    client = client_init("localhost", port_client);
    tcp_server server = server_init(port_server, 1, add_client, handle_client, clear_client);


    server_loop(server);
}
super_block spb;

static void update_superblock() {
    char buf[TCP_BUF_SIZE] = {0};
    memcpy(buf, &spb, sizeof(spb));
    write_block(0, buf, SUPERBLOCK_SIZE);
}

int init_spb() {
    spb.s_magic = MAGIC_NUMBER;
    spb.s_free_inodes_count = MAX_INODE_COUNT;

    int num_disk_block = get_disk_info();
    if (num_disk_block > MAX_BLOCK_COUNT)
        num_disk_block = MAX_BLOCK_COUNT;
    if (num_disk_block <= RESERVE_BLOCK)
        return -1;

    spb.s_free_blocks_count = num_disk_block - RESERVE_BLOCK;
    spb.s_inodes_count = 0;
    spb.s_blocks_count = INITIAL_USED_BLOCKS;

    memset(spb.inode_map, 0, sizeof(spb.inode_map));
    memset(spb.block_map, 0, sizeof(spb.block_map));
    for (int i = 0; i <= 3; i++)
        spb.block_map[i] = ~0;
    spb.block_map[4] = 0xe0000000;

    update_superblock();
    return 0;
}

int load_spb() {
    char buf[TCP_BUF_SIZE] = {0};
    for (int i = 0; i < SUPERBLOCK_SIZE; i++) {
        if (read_block(i, &buf[i * BLOCK_SIZE]) < 0) {
            perror("Read from disk failed");
            return -1;
        }
    }
    memcpy(&spb, buf, sizeof(super_block));
    return 0;
}

int check_spb() {
    int num_disk_block = get_disk_info();
    if (num_disk_block > MAX_BLOCK_COUNT)
        num_disk_block = MAX_BLOCK_COUNT;
    if (num_disk_block <= RESERVE_BLOCK)
        return -1;
    if ((spb.s_blocks_count + spb.s_free_blocks_count) != num_disk_block)
        return -1;
    return 0;
}

int alloc_block() {
    if (!spb.s_free_blocks_count)
        return -1;

    for (int i = 4; i < MAX_BLOCK_MAP; i++) {
        uint32_t block = spb.block_map[i];
        for (int j = 0; j < 32; j++) {
            if ((block >> (31 - j)) & 1)
                continue;

            spb.s_free_blocks_count--;
            spb.s_blocks_count++;
            spb.block_map[i] |= 1 << (31 - j);

            update_superblock();
            return i * 32 + j;
        }
    }
    return -1;
}

int free_block(uint16_t index) {
    if (index < RESERVE_BLOCK)
        return -1;

    int i = index / 32;
    int j = index % 32;

    if (!((spb.block_map[i] >> (31 - j)) & 1))
        return 1;

    char buf[BLOCK_SIZE] = {0};
    if (write_block(index, buf, 1) < 0)
        return -1;

    spb.block_map[i] ^= 1 << (31 - j);
    spb.s_free_blocks_count++;
    spb.s_blocks_count--;

    update_superblock();
    return 0;
}
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

inode inode_table[MAX_INODE_COUNT];
int init_root_inode()
{
    inode root;
    root.i_index = 0;
    root.i_mode = 1;
    root.i_link_count = 0;
    root.i_size = 0;
    root.i_timestamp = time(NULL);
    root.i_parent = 65535;
    memset(root.i_direct, 0, sizeof(root.i_direct));
    root.i_single_indirect = 0;
    inode_table[0] = root;
    spb.inode_map[0] |= (1 << 31);
    return 0;
}

int init_inode(inode *node, uint16_t index, uint8_t mode, uint8_t link, uint16_t size, uint16_t parent)
{
    node->i_index = index;
    node->i_mode = mode;
    node->i_link_count = link;
    node->i_size = size;
    node->i_parent = parent;
    node->i_timestamp = time(NULL);
    memset(node->i_direct, 0, sizeof(node->i_direct));
    node->i_single_indirect = 0;
    write_inode(node, index);
    return 0;
}

int write_inode(inode *node, uint16_t index)
{
    int block_id = INODE_TABLE_START_BLOCK + index / INODE_PER_BLOCK; // 一个block有8个inode
    int inode_start = sizeof(inode) * (index % INODE_PER_BLOCK);      // 取余
    char buf[TCP_BUF_SIZE];
    if (read_block(block_id, buf) < 0)
        return -1;
    memcpy(buf + inode_start, node, sizeof(inode));
    write_block(block_id, buf, 1);
    return 0;
}

int read_inode(inode *node, uint16_t index)
{
    int block_id = INODE_TABLE_START_BLOCK + index / INODE_PER_BLOCK;
    int inode_start = sizeof(inode) * (index % INODE_PER_BLOCK);
    char buf[TCP_BUF_SIZE];
    if (read_block(block_id, buf) < 0)
        return -1;
    memcpy(node, &buf[inode_start], sizeof(inode));
    return 1;
}

int alloc_inode()
{
    if (!spb.s_free_inodes_count)
        return -1;
    for (int i = 0; i < MAX_INODE_MAP; i++)
    {
        uint32_t node = spb.inode_map[i];
        for (int j = 0; j < 32; j++)
        {
            if ((node >> (31 - j)) & 1)
                continue; // 此块不空闲
            else
            {
                spb.s_free_inodes_count--;
                spb.s_inodes_count++;
                spb.inode_map[i] |= 1 << (31 - j);

                char buf[3 * BLOCK_SIZE]; // 将修改后的spb经由spb写入内存块1-3
                memset(buf, 0, sizeof(buf));
                memcpy(buf, &spb, sizeof(spb));
                write_block(0, buf, 3);
                return i * 32 + j;
            }
        }
    }
    return -1;
}

// dir型inode本函数将直接删除并释放块（不检查块中是否有文件，这应由上游完成）；file型会释放块
int free_inode(inode *node)
{
    if (node->i_index <= 0)
        return -1; // 不允许free root
    int i = node->i_index / 32;
    int j = node->i_index % 32;
    if (((spb.inode_map[i] >> (31 - j)) & 1) == 0)
        return 1; // 此块本身已经空闲
    else
    {
        for (int k = 0; k < 8; k++) // 清空direct
            if (node->i_direct[k] != 0)
                free_block(node->i_direct[k]);
        if (node->i_single_indirect != 0) // 清空indirect
        {
            uint16_t blocks[BLOCK_SIZE / sizeof(uint16_t)];
            char buf[BLOCK_SIZE];
            read_block(node->i_single_indirect, buf);
            memcpy(blocks, buf, BLOCK_SIZE);
            for (int k = 0; k < BLOCK_SIZE / sizeof(uint16_t); k++)
                if (blocks[k] != 0)
                    free_block(blocks[k]);
        }
        spb.s_free_inodes_count++;
        spb.s_inodes_count--;
        spb.inode_map[i] &= ~(1 << (31 - j));

        char buf[3 * BLOCK_SIZE]; // 将修改后的spb经由spb写入内存块1-3
        memset(buf, 0, sizeof(buf));
        memcpy(buf, &spb, sizeof(spb));
        write_block(0, buf, 3);
        return 0;
    }
}

// 目录型inode仅direct有效，每个指向的块中含有8个dir_item，一共有64个dir_item可以存在一个目录下
int add_to_dir_inode(inode *dir_node, char *name, uint8_t type)
{
    if (!dir_node->i_mode)
        return -1; // 不是目录结点
    char buf[TCP_BUF_SIZE];
    dir_item dir_items[8];
    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            continue;
        if (read_block(dir_node->i_direct[i], buf) < 0)
            continue;
        memcpy(&dir_items, buf, BLOCK_SIZE);
        for (int j = 0; j < 8; j++)
        { // 遍历8个direct各自对应block中的8个dir_item，找空
            if (dir_items[j].valid == 0)
            {
                int new_inode_id = alloc_inode();
                if (new_inode_id >= 0)
                {
                    init_inode(&inode_table[new_inode_id], new_inode_id, type, 0, 0, dir_node->i_index);

                    dir_items[j].valid = 1;
                    strcpy(dir_items[j].name, name);
                    dir_items[j].inode_id = new_inode_id;
                    dir_items[j].type = type;

                    memcpy(buf, &dir_items, BLOCK_SIZE);
                    if (write_block(dir_node->i_direct[i], buf, 1) < 0)
                        return -1;

                    dir_node->i_timestamp = time(NULL);
                    if (write_inode(dir_node, dir_node->i_index) < 0)
                        return -1;
                    return 0;
                }
                else
                    return -1;
            }
        }
    }
    // 如果上述过程无结果且link<8，则新分配block
    if (dir_node->i_link_count < 8)
    {
        int k; // 寻找尚未link的direct编号
        for (k = 0; k < 8; k++)
            if (dir_node->i_direct[k] == 0)
                break;
        int new_block_id = alloc_block();
        if (new_block_id >= 0)
        {
            int new_inode_id = alloc_inode();
            if (new_inode_id >= 0)
            {
                dir_node->i_direct[k] = new_block_id; // 新分配block
                dir_node->i_link_count += 1;

                for (int i = 0; i < 8; i++)
                {
                    dir_items[i].valid = 0;
                    dir_items[i].inode_id = 0;
                }
                init_inode(&inode_table[new_inode_id], new_inode_id, type, 0, 0, dir_node->i_index); // 新direct-block第一位创建新inode

                dir_items[0].valid = 1;
                strcpy(dir_items[0].name, name);
                dir_items[0].inode_id = new_inode_id;
                dir_items[0].type = type;

                memcpy(buf, &dir_items, BLOCK_SIZE);
                if (write_block(dir_node->i_direct[k], buf, 1) < 0)
                    return -1;

                dir_node->i_timestamp = time(NULL);
                if (write_inode(dir_node, dir_node->i_index) < 0)
                    return -1;
                return 0;
            }
            else
                return -1;
        }
        else
            return -1;
    }
    return -1;
}
int rm_from_dir_inode(inode *dir_node, char *name)
{
    dir_item dir_items[DIR_ITEM_PER_BLOCK];
    char buf[BLOCK_SIZE];

    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            break;

        read_block(dir_node->i_direct[i], buf);
        memcpy(dir_items, buf, sizeof(buf));

        for (int j = 0; j < DIR_ITEM_PER_BLOCK; j++)
        {
            if (!dir_items[j].valid)
                continue;

            if (strcmp(dir_items[j].name, name) == 0)
            {
                dir_items[j].valid = 0;
                memcpy(buf, dir_items, sizeof(buf));
                write_block(dir_node->i_direct[i], buf, 1);
                dir_node->i_link_count--;

                return 0;
            }
        }
    }
    return -1;
}

int rmdir_from_dir_inode(inode *dir_node, char *name)
{
    dir_item dir_items[DIR_ITEM_PER_BLOCK];
    char buf[BLOCK_SIZE];

    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            break;

        read_block(dir_node->i_direct[i], buf);
        memcpy(dir_items, buf, sizeof(buf));

        for (int j = 0; j < DIR_ITEM_PER_BLOCK; j++)
        {
            if (!dir_items[j].valid)
                continue;

            if (strcmp(dir_items[j].name, name) == 0)
            {
                inode child;
                read_inode(&child, dir_items[j].inode_id);

                // Recursively remove all items within the directory
                for (int k = 0; k < 8; k++)
                {
                    if (child.i_direct[k] == 0)
                        continue;

                    read_block(child.i_direct[k], buf);
                    dir_item child_items[DIR_ITEM_PER_BLOCK];
                    memcpy(child_items, buf, sizeof(buf));

                    for (int l = 0; l < DIR_ITEM_PER_BLOCK; l++)
                    {
                        if (!child_items[l].valid)
                            continue;

                        if (child_items[l].type == 0)
                        { // File
                            rm_from_dir_inode(&child, child_items[l].name);
                        }
                        else if (child_items[l].type == 1)
                        { // Directory
                            rmdir_from_dir_inode(&child, child_items[l].name);
                        }
                    }
                }

                // Free the directory inode itself after all contents are removed
                free_inode(&child);
                dir_items[j].valid = 0;
                memcpy(buf, dir_items, sizeof(buf));
                write_block(dir_node->i_direct[i], buf, 1);
                dir_node->i_link_count--;

                return 0;
            }
        }
    }
    return -1;
}

int check_dir_isempty_delete(inode *dir_node)
{
    dir_item dir_items[DIR_ITEM_PER_BLOCK];
    char buf[BLOCK_SIZE];
    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            continue;
        read_block(dir_node->i_direct[i], buf);
        memcpy(dir_items, buf, sizeof(buf));
        for (int j = 0; j < DIR_ITEM_PER_BLOCK; j++)
            if (dir_items[j].valid)
                return -1;
        free_block(dir_node->i_direct[i]);
    }
    free_inode(dir_node);
    return 0;
}

int lexic_cmp(const void *a, const void *b)
{
    dir_item *item1 = (dir_item *)a;
    dir_item *item2 = (dir_item *)b;
    if (item1->valid && !item2->valid)
        return -1;
    if (!item1->valid && item2->valid)
        return 1;
    if (!item1->valid && !item2->valid)
        return 0;
    return strcmp(item1->name, item2->name);
}

int ls_dir_inode(inode *dir_node, char *ret, char *ret2, bool detailed)
{
    if (!dir_node->i_mode)
        return -1;
    char buf[TCP_BUF_SIZE];
    bool flag = false; // 文件夹是否为空，false代表空

    char dir_name[64][MAX_NAME_SIZE*2];
    char file_name[64][MAX_NAME_SIZE*2];
    char dir[64 * MAX_NAME_SIZE*2];
    char file[64 * MAX_NAME_SIZE*2];
    int num_dir = 0, num_file = 0;

    ret[0] = '\0';
    dir_item dir_items[8];
    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            continue;
        if (read_block(dir_node->i_direct[i], buf) < 0)
            continue;
        memcpy(&dir_items, buf, BLOCK_SIZE);
        for (int j = 0; j < 8; j++)
        {
            if (dir_items[j].valid)
            {
                if (dir_items[j].type == 1)
                    strcpy(dir_name[num_dir++], dir_items[j].name);
                else
                    strcpy(file_name[num_file++], dir_items[j].name);
                flag = true;
            }
        }
    }

    if (flag)
    {
        if (detailed)
        {
            char detailed_info[256];
            ret[0] = '\0';
            for (int i = 0; i < num_file; i++)
            {
                inode file_inode;
                int inode_index = search_in_dir_inode(dir_node, file_name[i], 0);
                if (inode_index >= 0)
                {
                    read_inode(&file_inode, inode_index);
                    sprintf(detailed_info, "%d %d %s\n", file_inode.i_size, file_inode.i_timestamp, "file");
                    strcat(file_name[i], detailed_info);//这里确认file_name不会用来搜索
                }
            }
            for (int i = 0; i < num_dir; i++)
            {
                inode dir_inode;
                int inode_index = search_in_dir_inode(dir_node, dir_name[i], 1);
                if (inode_index >= 0)
                {
                    read_inode(&dir_inode, inode_index);
                    sprintf(detailed_info, "%d %d %s\n", dir_inode.i_size, dir_inode.i_timestamp, "dir");
                    strcat(dir_name[i], detailed_info);
                }
            }
        }
    }

    // 字典序排序文件与文件夹名
    qsort(dir_name, num_dir, sizeof(dir_name[0]), lexic_cmp);
    qsort(file_name, num_file, sizeof(file_name[0]), lexic_cmp);
    dir[0] = '\0';
    for (int i = 0; i < num_dir; i++)
    {
        if (i&&!detailed)
            strcat(dir, "  ");
        strcat(dir, dir_name[i]);
    }
    file[0] = '\0';
    for (int i = 0; i < num_file; i++)
    {
        if (i&&!detailed)
            strcat(file, "  ");
        strcat(file, file_name[i]);
    }
    if (flag)
    {
        sprintf(ret, "%s  &  %s\n", file, dir);
        sprintf(ret2, "%s  \033[1;34m%s\033[0m\n", file, dir);
    }
    else
    {
        sprintf(ret, " \n");
        sprintf(ret2, " \n");
    }
    return 0;
}

int search_in_dir_inode(inode *dir_node, char *name, int type)
{
    dir_item dir_items[DIR_ITEM_PER_BLOCK];
    char buf[BLOCK_SIZE];
    for (int i = 0; i < 8; i++)
    {
        if (dir_node->i_direct[i] == 0)
            break;
        read_block(dir_node->i_direct[i], buf);
        memcpy(dir_items, buf, sizeof(buf));
        for (int j = 0; j < DIR_ITEM_PER_BLOCK; j++)
        {
            if (!dir_items[j].valid)
                continue;
            if (strcmp(dir_items[j].name, name) == 0 && dir_items[j].type == type)
                return dir_items[j].inode_id;
        }
    }
    return -1;
}

int read_file_inode(inode *file_node, char *ret)
{
    if (file_node->i_mode)
        return -1; // 不是文件则退出
    uint16_t indirect_blocks[128];
    char buf[TCP_BUF_SIZE];
    memset(ret, 0, sizeof(&ret));

    // if (file_node->i_link_count <= 8)
    //     for(int i = 0; i < file_node->i_link_count; i++){
    //         if (file_node->i_direct[i]==0) return -1;
    //         memset(buf, 0, BLOCK_SIZE);
    //         if (read_block(file_node->i_direct[i], buf)<0) return -1;
    //         memcpy(ret + strlen(ret), buf, BLOCK_SIZE);  // 使用 memcpy 将块数据复制到目标字符串中，因为strcpy需要null结尾但分block可能没有！
    //     }
    // else{
    //     if (file_node->i_single_indirect == 0) return -1;
    //     if (read_block(file_node->i_single_indirect, buf) < 0) return -1;
    //     memcpy(&indirect_blocks, buf, BLOCK_SIZE);
    //     for (int i=0; i < file_node->i_link_count - 8; i++){
    //         if (indirect_blocks[i] == 0) return -1;
    //         if (read_block(indirect_blocks[i], buf)<0) return -1;
    //         memcpy(ret + strlen(ret), buf, BLOCK_SIZE);
    //     }
    // }
    // return 0;
    int total_blocks = file_node->i_link_count;
    int last_block_size = file_node->i_size % BLOCK_SIZE;
    if (last_block_size == 0 && total_blocks > 0)
        last_block_size = BLOCK_SIZE;

    if (total_blocks <= 8)
        for (int i = 0; i < total_blocks; i++)
        {
            if (file_node->i_direct[i] == 0)
                return -1;
            memset(buf, 0, BLOCK_SIZE);
            if (read_block(file_node->i_direct[i], buf) < 0)
                return -1;
            int size_to_copy = (i == total_blocks - 1) ? last_block_size + 1 : BLOCK_SIZE;
            memcpy(ret + i * BLOCK_SIZE, buf, size_to_copy);
            // printf("%d %ld\n", i, strlen(ret));
        }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            if (file_node->i_direct[i] == 0)
                return -1;
            memset(buf, 0, BLOCK_SIZE);
            if (read_block(file_node->i_direct[i], buf) < 0)
                return -1;
            memcpy(ret + i * BLOCK_SIZE, buf, BLOCK_SIZE);
        }
        if (file_node->i_single_indirect == 0)
            return -1;
        if (read_block(file_node->i_single_indirect, buf) < 0)
            return -1;
        memcpy(&indirect_blocks, buf, BLOCK_SIZE);
        for (int i = 0; i < total_blocks - 8; i++)
        {
            if (indirect_blocks[i] == 0)
                return -1;
            if (read_block(indirect_blocks[i], buf) < 0)
                return -1;
            int size_to_copy = (i == total_blocks - 8 - 1) ? last_block_size : BLOCK_SIZE;
            memcpy(ret + (i + 8) * BLOCK_SIZE, buf, size_to_copy);
        }
    }
    return 0;
}

int write_file_inode(inode *file_node, char *src)
{
    if (file_node->i_mode)
        return -1; // 不是文件则退出
    uint16_t indirect_blocks[128];
    memset(indirect_blocks, 0, sizeof(indirect_blocks));
    int need_link_count = (strlen(src) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int ori_link_count = file_node->i_link_count;

    // printf("need: %d, ori: %d\n", need_link_count, ori_link_count);
    char tmp[need_link_count * BLOCK_SIZE];
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, src, strlen(src) + 1);
    char buf[TCP_BUF_SIZE];

    if (need_link_count <= ori_link_count)
    {
        // 写入
        for (int i = 0; i < (need_link_count >= 8 ? 8 : need_link_count); i++)
        {
            memcpy(buf, tmp + i * BLOCK_SIZE, BLOCK_SIZE);
            write_block(file_node->i_direct[i], buf, 1);
        }
        if (read_block(file_node->i_single_indirect, buf) < 0)
            return -1;
        memcpy(&indirect_blocks, buf, BLOCK_SIZE);
        for (int i = 0; i < (need_link_count - 8); i++)
        {
            memcpy(buf, tmp + (i + 8) * BLOCK_SIZE, BLOCK_SIZE);
            write_block(indirect_blocks[i], buf, 1);
        }

        // 归还多余block
        for (int i = need_link_count; i < (ori_link_count >= 8 ? 8 : ori_link_count); i++)
        {
            free_block(file_node->i_direct[i]);
            file_node->i_direct[i] = 0;
        }
        if (ori_link_count > 8 && file_node->i_single_indirect != 0)
        {
            for (int i = (need_link_count < 8 ? 0 : need_link_count - 8); i < ori_link_count - 8; i++)
            {
                free_block(indirect_blocks[i]);
                indirect_blocks[i] = 0;
            }
            if (need_link_count <= 8)
            {
                free_block(file_node->i_single_indirect);
                file_node->i_single_indirect = 0;
            }
            else
            {
                memcpy(buf, &indirect_blocks, BLOCK_SIZE);
                write_block(file_node->i_single_indirect, buf, 1);
            }
        }
    }
    else
    {
        // 申请不足的新结点
        if (need_link_count - ori_link_count > spb.s_free_blocks_count)
            return -1;
        for (int i = ori_link_count; i < (need_link_count > 8 ? 8 : need_link_count); i++)
        {
            uint16_t new_block_id = alloc_block();
            if (new_block_id < 0)
                return -1;
            file_node->i_direct[i] = new_block_id;
        }

        if (need_link_count > 8)
        {
            if (ori_link_count <= 8 && file_node->i_single_indirect == 0)
            {
                uint16_t new_block_id = alloc_block();
                if (new_block_id < 0)
                    return -1;
                file_node->i_single_indirect = new_block_id;
                memset(indirect_blocks, 0, sizeof(indirect_blocks));
            }
            else
            {
                if (read_block(file_node->i_single_indirect, buf) < 0)
                    return -1;
                memcpy(&indirect_blocks, buf, BLOCK_SIZE);
            }
            for (int i = (ori_link_count < 8 ? 0 : ori_link_count - 8); i < need_link_count - 8; i++)
            {
                uint16_t new_block_id = alloc_block();
                if (new_block_id < 0)
                    return -1;
                indirect_blocks[i] = new_block_id;
            }
            memcpy(buf, &indirect_blocks, BLOCK_SIZE);
            write_block(file_node->i_single_indirect, buf, 1);
        }
        // 写入
        for (int i = 0; i < (need_link_count >= 8 ? 8 : need_link_count); i++)
        {
            memcpy(buf, tmp + i * BLOCK_SIZE, BLOCK_SIZE);
            write_block(file_node->i_direct[i], buf, 1);
        }
        if (read_block(file_node->i_single_indirect, buf) < 0)
            return -1;
        memcpy(&indirect_blocks, buf, BLOCK_SIZE);
        for (int i = 0; i < (need_link_count - 8); i++)
        {
            memcpy(buf, tmp + (i + 8) * BLOCK_SIZE, BLOCK_SIZE);
            write_block(indirect_blocks[i], buf, 1);
        }
    }
    file_node->i_link_count = need_link_count; // 更新连接数
    file_node->i_size = strlen(src);           // size储存文件实际长度
    file_node->i_timestamp = time(NULL);
    write_inode(file_node, file_node->i_index);
    return 0;
}
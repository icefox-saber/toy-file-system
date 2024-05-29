#pragma once
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
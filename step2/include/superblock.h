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
#include <stdint.h>

#define BLOCK_SIZE 256
#define MAX_INODE_MAP 32  
#define MAX_BLOCK_MAP 128

#define MAX_INODE_COUNT 1024            //32*32 = 1024
#define MAX_BLOCK_COUNT 4096            //128*32 = 4096
#define RESERVE_BLOCK 131

typedef struct super_block {
    uint16_t s_magic;              // 魔数，用于标识文件系统
    uint32_t s_inodes_count;       // 总的inode数量
    uint32_t s_blocks_count;       // 已使用的块数量
    uint32_t s_free_inodes_count;  // 空闲的inode数量
    uint32_t s_free_blocks_count;  // 空闲的块数量
    uint32_t s_root;               // 根目录的inode编号
    uint32_t block_map[MAX_BLOCK_MAP];  // 块位图，记录块是否空闲
    uint32_t inode_map[MAX_INODE_MAP];  // inode位图，记录inode是否空闲
} super_block;


/**
 * @brief 初始化super block
 */
int init_spb();

/**
 * @brief 从磁盘中加载spb
 */
int load_spb();

/**
 * @brief 加载后核对spb中块上限数量和当前磁盘是否一致
 */
int check_spb();

/**
 * @brief 寻找并分配空闲块
 * @return 返回大于0代表成功（即块编号），-1代表失败
 */
int alloc_block();

/**
 * @brief 将已分配block还原
 * @param index 需要还原的block号
 * @return 返回0代表成功，-1代表失败，1代表本块本身已经空闲
 */
int free_block(uint16_t index);

extern super_block spb;

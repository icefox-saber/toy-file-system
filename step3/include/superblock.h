#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#include "tcp_utils.h"

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
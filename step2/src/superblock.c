#include "superblock.h"
#include "diskop.h"

#include <string.h>
super_block spb;
int init_spb()
{
    spb.s_magic = 0xE986; // 设置魔数
    spb.s_free_inodes_count = MAX_INODE_COUNT;
    int num_disk_block = get_disk_info(); // 获取磁盘信息
    if (num_disk_block > MAX_BLOCK_COUNT)
        num_disk_block = MAX_BLOCK_COUNT;
    if (num_disk_block <= RESERVE_BLOCK)
        return -1;                                            // 确保磁盘块数量足够
    spb.s_free_blocks_count = num_disk_block - RESERVE_BLOCK; // 计算空闲块数量
    spb.s_inodes_count = 0;
    spb.s_blocks_count = 131;                        // 预留的块数量
    memset(spb.inode_map, 0, sizeof(spb.inode_map)); // 初始化inode位图
    memset(spb.block_map, 0, sizeof(spb.block_map)); // 初始化块位图
    for (int i = 0; i <= 3; i++)
        spb.block_map[i] = ~0;       // 标记预留块为已使用32+32+32+32=128blocks=1024 i-node
    spb.block_map[4] = (0xe0000000); // 标记部分预留块为已使用3blocks，总共131blocks

    char buf[TCP_BUF_SIZE];
    memset(buf, 0, sizeof(buf));    // 清空缓冲区
    memcpy(buf, &spb, sizeof(spb)); // 将super_block拷贝到缓冲区
    write_block(0, buf, 3);         // 写入到磁盘的前3个块
    return 0;
}

int load_spb()
{
    char buf[TCP_BUF_SIZE]; // 缓冲区用于读取super_block
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < 3; i++)
    {
        if (read_block(i, &buf[i * BLOCK_SIZE]) < 0)
        {
            printf("Read from disk failed.\n");
            return -1;
        }
    }
    memcpy(&spb, buf, sizeof(super_block)); // 将读取的缓冲区内容拷贝到super_block
    return 0;
}

int check_spb()
{
    int num_disk_block = get_disk_info(); // 获取磁盘信息
    if (num_disk_block > MAX_BLOCK_COUNT)
        num_disk_block = MAX_BLOCK_COUNT;
    if (num_disk_block <= RESERVE_BLOCK)
        return -1;
    if ((spb.s_blocks_count + spb.s_free_blocks_count) != num_disk_block)
        return -1; // 校验块数量
    return 0;
}

int alloc_block()
{
    if (!spb.s_free_blocks_count)
        return -1; // 如果没有空闲块，返回-1
    for (int i = 4; i < MAX_BLOCK_MAP; i++)
    {
        uint32_t block = spb.block_map[i];
        for (int j = 0; j < 32; j++)
        {
            if ((block >> (31 - j)) & 1) // 判断左数j+1位
                continue;                // 跳过已使用的块
            else
            {
                spb.s_free_blocks_count--;
                spb.s_blocks_count++;
                spb.block_map[i] |= 1 << (31 - j); // 标记块为已使用,将左数第j+1位赋值位1

                char buf[TCP_BUF_SIZE]; // 将修改后的super_block写入磁盘
                memset(buf, 0, sizeof(buf));
                memcpy(buf, &spb, sizeof(spb));
                if (write_block(0, buf, 3) < 0)
                    return -1;
                return i * 32 + j;
            }
        }
    }
    return -1;
}

int free_block(uint16_t index)
{
    if (index < RESERVE_BLOCK)
        return -1; // 不允许释放预留块
    int i = index / 32;
    int j = index % 32;
    if (((spb.block_map[i] >> (31 - j)) & 1) == 0)
        return 1; // 块已空闲
    else
    {

        char buf[BLOCK_SIZE]; // 清空对应块
        memset(buf, 0, sizeof(buf));
        if (write_block(index, buf, 1) < 0)
            return -1;

        spb.block_map[i] ^= 1 << (31 - j); // 异或
        spb.s_free_blocks_count++;
        spb.s_blocks_count--;
        char buf_2[3 * BLOCK_SIZE]; // 将修改后的super_block写入磁盘
        memset(buf_2, 0, sizeof(buf_2));
        memcpy(buf_2, &spb, sizeof(spb));
        if (write_block(0, buf_2, 3) < 0)
            return -1;
        return 0;
    }
    return -1;
}

#include "superblock.h"
#include "diskop.h"

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
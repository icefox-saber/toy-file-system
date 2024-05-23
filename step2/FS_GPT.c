//FS.c
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "tcp_utils.h"
//./FS <BDSPort=10356> <FSPort=12356>

#define BLOCKSIZE 256
#define MAX_FILES 128
#define MAX_FILENAME 8
#define MAX_PATH 256

typedef enum {
    FILE_TYPE,
    DIRECTORY_TYPE
} FileType;

typedef struct Inode {
    int id;
    char name[MAX_FILENAME];
    FileType type;
    int size;
    int blocks[64]; // Maximum 64 blocks per file
    int num_blocks;
    time_t last_modified;
    struct Inode* parent;
    struct Inode* children[MAX_FILES];
    int num_children;
} Inode;

typedef struct FileSystem {
    Inode* root;
    Inode* cwd;
    int free_blocks;
    int total_blocks;
    char* block_map;
} FileSystem;

FileSystem fs;
static tcp_client client;

// File System initialization and formatting
void fs_init(int total_blocks) {
    fs.root = malloc(sizeof(Inode));
    fs.root->id = 0;
    strcpy(fs.root->name, "/");
    fs.root->type = DIRECTORY_TYPE;
    fs.root->size = 0;
    fs.root->num_blocks = 0;
    fs.root->parent = NULL;
    fs.root->num_children = 0;
    fs.cwd = fs.root;
    fs.free_blocks = total_blocks;
    fs.total_blocks = total_blocks;
    fs.block_map = calloc(total_blocks, sizeof(char));
}

Inode* create_inode(const char* name, FileType type, Inode* parent) {
    static int next_id = 1;
    Inode* inode = malloc(sizeof(Inode));
    inode->id = next_id++;
    strcpy(inode->name, name);
    inode->type = type;
    inode->size = 0;
    inode->num_blocks = 0;
    inode->last_modified = time(NULL);
    inode->parent = parent;
    inode->num_children = 0;
    if (parent) {
        parent->children[parent->num_children++] = inode;
    }
    return inode;
}

Inode* find_inode(Inode* parent, const char* name) {
    for (int i = 0; i < parent->num_children; ++i) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    return NULL;
}

void fs_format() {
    for (int i = 0; i < fs.total_blocks; ++i) {
        fs.block_map[i] = 0;
    }
    fs.free_blocks = fs.total_blocks;
    fs.cwd = fs.root;
    fs.root->num_children = 0;
}

// Command Handlers
int cmd_create(tcp_buffer* write_buf, char* args, int len) {
    char name[MAX_FILENAME];
    sscanf(args, "%s", name);
    Inode* inode = create_inode(name, FILE_TYPE, fs.cwd);
    if (!inode) {
        send_to_buffer(write_buf, "Error creating file", strlen("Error creating file") + 1);
        return 0;
    }
    send_to_buffer(write_buf, "File created", strlen("File created") + 1);
    return 0;
}

int cmd_delete(tcp_buffer* write_buf, char* args, int len) {
    char name[MAX_FILENAME];
    sscanf(args, "%s", name);
    Inode* inode = find_inode(fs.cwd, name);
    if (!inode) {
        send_to_buffer(write_buf, "File not found", strlen("File not found") + 1);
        return 0;
    }
    free(inode);
    send_to_buffer(write_buf, "File deleted", strlen("File deleted") + 1);
    return 0;
}

int cmd_ls(tcp_buffer* write_buf, char* args, int len) {
    char buf[4096];
    buf[0] = 0;
    for (int i = 0; i < fs.cwd->num_children; ++i) {
        strcat(buf, fs.cwd->children[i]->name);
        strcat(buf, "\n");
    }
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
    return 0;
}

int cmd_cd(tcp_buffer* write_buf, char* args, int len) {
    char path[MAX_PATH];
    sscanf(args, "%s", path);
    Inode* target = (strcmp(path, "..") == 0) ? fs.cwd->parent : find_inode(fs.cwd, path);
    if (!target || target->type != DIRECTORY_TYPE) {
        send_to_buffer(write_buf, "Directory not found", strlen("Directory not found") + 1);
        return 0;
    }
    fs.cwd = target;
    send_to_buffer(write_buf, "Changed directory", strlen("Changed directory") + 1);
    return 0;
}

int cmd_mkdir(tcp_buffer* write_buf, char* args, int len) {
    char name[MAX_FILENAME];
    sscanf(args, "%s", name);
    Inode* inode = create_inode(name, DIRECTORY_TYPE, fs.cwd);
    if (!inode) {
        send_to_buffer(write_buf, "Error creating directory", strlen("Error creating directory") + 1);
        return 0;
    }
    send_to_buffer(write_buf, "Directory created", strlen("Directory created") + 1);
    return 0;
}

// Command table
static struct {
    const char *name;
    int (*handler)(tcp_buffer *write_buf, char *, int);
} cmd_table[] = {
    {"CREATE", cmd_create},
    {"DELETE", cmd_delete},
    {"LS", cmd_ls},
    {"CD", cmd_cd},
    {"MKDIR", cmd_mkdir}
};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void add_client(int id) {}

int handle_client(int id, tcp_buffer *write_buf, char *msg, int len) {
    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++) {
        if (strcmp(p, cmd_table[i].name) == 0) {
            ret = cmd_table[i].handler(write_buf, p + strlen(p) + 1, len - strlen(p) - 1);
            break;
        }
    }
    if (ret == 1) {
        static char unk[] = "Unknown command";
        send_to_buffer(write_buf, unk, sizeof(unk));
    }
    return 0;
}

void clear_client(int id) {}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <BDSPort=10356> <FSPort=12356>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port_client = atoi(argv[1]);
    int port_server = atoi(argv[2]);

    // Initialize file system
    fs_init(1000);

    client = client_init("localhost", port_client);
    tcp_server server = server_init(port_server, 1, add_client, handle_client, clear_client);

    server_loop(server);

    client_destroy(client);
}

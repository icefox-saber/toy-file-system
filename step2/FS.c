#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"
//./FS localhost 10356 12356
static tcp_client client;
static struct
{
    const char *name;
    int (*handler)(tcp_buffer *write_buf, char *, int);
} cmd_table[] = {};
// command number
#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))
void add_client(int id)
{

    // some code that are executed when a new client is connected
    // you don't need this in step1
}
// msg为recv到的
int handle_client(int id, tcp_buffer *write_buf, char *msg, int len)
{
    static char buf[64];
    // have getten msg from FC
    // msg from BDC=>msg to BDS
    // recv msg from BDS
    // msg from BDS => msg from BDS
    client_send(client, msg, strlen(msg) + 1);
    int n = client_recv(client, buf, sizeof(buf));
    if (n==-1)
    {
        static char* msg1="error client_recv";
        send_to_buffer(write_buf, msg1, strlen(msg) + 1);
    }
    if (n==-1)
    {
        static char* msg2="nothing client_recv";
        send_to_buffer(write_buf, msg2, strlen(msg) + 1);
    }
    buf[n] = 0;
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
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

    int port_server = atoi(argv[2]);
    int port_client = atoi(argv[1]);
    client = client_init("localhost", port_client);
    tcp_server server = server_init(port_server, 1, add_client, handle_client, clear_client);
    static char buf[4096];


    server_loop(server);
}

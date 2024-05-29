// ./FC 8088
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"
//./FC 12356
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <Port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    tcp_client client = client_init("localhost", port);
    static char buf[4096];
    char ini[20] = "ini\n";
    printf("uid:");
    fgets(buf, sizeof(buf), stdin);
    strncat(ini, buf, 3);
    client_send(client, ini, strlen(ini) + 1);
    int n = client_recv(client, buf, sizeof(buf));
    buf[n] = 0;
    if (strcmp(buf, "Please first format.\n!format$") == 0)
    {
        client_send(client, "f\n", 2);
        n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        client_send(client, ini, strlen(ini) + 1);
        n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
    }

    printf("%s", buf);
    // int n = client_recv(client, buf, sizeof(buf));
    // buf[n] = 0;
    // printf("%s\n", buf);
    while (1)
    {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin))
            break;
        client_send(client, buf, strlen(buf) + 1);
        n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        printf("%s", buf);
        if (strcmp(buf, "Goodbye!\n") == 0)
            break;
    }
    client_destroy(client);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define TCP_PORT 8080
#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/* ---------------- BROADCAST ---------------- */

void broadcast(char *msg)
{
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++)
    {
        if(send(clients[i], msg, strlen(msg), 0) <= 0)
        {
            printf("Client %d disconnected\n", clients[i]);
            close(clients[i]);

            for(int j = i; j < client_count-1; j++)
                clients[j] = clients[j+1];

            client_count--;
            i--;
        }
    }

    pthread_mutex_unlock(&lock);
}


/* ---------------- CAN READER THREAD ---------------- */

void *can_reader_thread(void *arg)
{
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    char msg[256];

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        perror("CAN Socket");
        pthread_exit(NULL);
    }

    strcpy(ifr.ifr_name, "can0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("CAN Bind");
        pthread_exit(NULL);
    }

    printf("CAN reader started...\n");

    while(1)
    {
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if(nbytes < 0)
            continue;

        int pos = sprintf(msg, "CAN 0x%03X [%d] ",
                          frame.can_id, frame.can_dlc);

        for(int i=0; i<frame.can_dlc; i++)
            pos += sprintf(msg+pos, "%02X ", frame.data[i]);

        pos += sprintf(msg+pos, "\n");

        //printf("%s", msg);

        broadcast(msg);
    }

    close(s);

    return NULL;
}


/* ---------------- MAIN SERVER ---------------- */

int main()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

    listen(server_sock,10);

    printf("TCP Broadcast Server started on port %d\n", TCP_PORT);

    /* Start CAN thread */
    pthread_t can_tid;
    pthread_create(&can_tid, NULL, can_reader_thread, NULL);

    while(1)
    {
        int client_sock = accept(server_sock,
                                 (struct sockaddr*)&client_addr,
                                 &addr_len);

        printf("Client connected: %s:%d (fd %d)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_sock);

        pthread_mutex_lock(&lock);

        if(client_count < MAX_CLIENTS)
        {
            clients[client_count++] = client_sock;
        }
        else
        {
            printf("Max clients reached\n");
            close(client_sock);
        }

        pthread_mutex_unlock(&lock);
    }

    close(server_sock);

    return 0;
}

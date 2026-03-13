#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

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

/* ---------------- REMOVE CLIENT ---------------- */

void remove_client(int index)
{
    close(clients[index]);

    for(int j = index; j < client_count - 1; j++)
        clients[j] = clients[j+1];

    client_count--;
}

/* ---------------- BROADCAST ---------------- */

void broadcast(char *msg)
{
    for(int i = 0; i < client_count; i++)
    {
        int ret = send(clients[i], msg, strlen(msg), MSG_NOSIGNAL);

        if(ret <= 0)
        {
            printf("Client %d disconnected\n", clients[i]);
            remove_client(i);
            i--;
        }
    }
}

/* ---------------- CAN THREAD ---------------- */

void *can_reader_thread(void *arg)
{
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    char msg[256];

    /* Create CAN socket */
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(s < 0)
    {
        perror("CAN socket");
        pthread_exit(NULL);
    }

    strcpy(ifr.ifr_name, "can0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("CAN bind");
        pthread_exit(NULL);
    }

    printf("CAN reader started...\n");

    while(1)
    {
        nbytes = read(s, &frame, sizeof(frame));

        if(nbytes < 0)
            continue;

        int pos = sprintf(msg,
                          "CAN 0x%03X [%d] ",
                          frame.can_id,
                          frame.can_dlc);

        for(int i = 0; i < frame.can_dlc; i++)
            pos += sprintf(msg + pos, "%02X ", frame.data[i]);

        sprintf(msg + pos, "\n");

        broadcast(msg);
    }

    close(s);
    return NULL;
}

/* ---------------- TCP THREAD ---------------- */

void *tcp_accept_thread(void *arg)
{
    int server_sock;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t addr_len = sizeof(client_addr);

    /* Create TCP socket */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server_sock < 0)
    {
        perror("Socket");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0)
    {
        perror("Bind");
        pthread_exit(NULL);
    }

    if(listen(server_sock,10) < 0)
    {
        perror("Listen");
        pthread_exit(NULL);
    }

    printf("TCP server listening on port %d\n", TCP_PORT);

    while(1)
    {
        int client_sock = accept(server_sock,
                                 (struct sockaddr*)&client_addr,
                                 &addr_len);

        if(client_sock < 0)
        {
            perror("Accept");
            continue;
        }

        printf("Client connected: %s:%d (fd %d)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_sock);

        if(client_count < MAX_CLIENTS)
        {
            clients[client_count++] = client_sock;
        }
        else
        {
            printf("Max clients reached\n");
            close(client_sock);
        }
    }

    close(server_sock);
    return NULL;
}

/* ---------------- MAIN ---------------- */

int main()
{
    signal(SIGPIPE, SIG_IGN);

    pthread_t can_tid;
    pthread_t tcp_tid;

    /* Start CAN thread */
    if(pthread_create(&can_tid, NULL, can_reader_thread, NULL) != 0)
    {
        perror("CAN thread create");
        return -1;
    }

    /* Start TCP thread */
    if(pthread_create(&tcp_tid, NULL, tcp_accept_thread, NULL) != 0)
    {
        perror("TCP thread create");
        return -1;
    }

    pthread_detach(can_tid);
    pthread_detach(tcp_tid);

    pthread_exit(NULL);
}

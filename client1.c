#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int sock;

/* Receive server broadcast */
void *receive_thread(void *arg)
{
    char buffer[BUFFER_SIZE];

    while(1)
    {
        memset(buffer,0,BUFFER_SIZE);

        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

        if(bytes <= 0)
        {
            printf("Server disconnected\n");
            exit(0);
        }

        printf("Server Broadcast: %s", buffer);
    }
}

/* Send message to server */
void *send_thread(void *arg)
{
    char buffer[BUFFER_SIZE];

    while(1)
    {
        fgets(buffer, BUFFER_SIZE, stdin);

        send(sock, buffer, strlen(buffer), 0);
    }
}

int main()
{
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

    printf("Connected to server\n");

    pthread_t send_tid, recv_tid;

    pthread_create(&send_tid,NULL,send_thread,NULL);
    pthread_create(&recv_tid,NULL,receive_thread,NULL);

    pthread_join(send_tid,NULL);
    pthread_join(recv_tid,NULL);

    close(sock);
}

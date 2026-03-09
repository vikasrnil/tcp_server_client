#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* Remove client from list */
void remove_client(int sock)
{
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++)
    {
        if(clients[i] == sock)
        {
            for(int j = i; j < client_count - 1; j++)
                clients[j] = clients[j+1];

            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&lock);
}

/* Broadcast server message */
void broadcast(char *msg)
{
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++)
    {
        int ret = send(clients[i], msg, strlen(msg), 0);

        if(ret <= 0)
        {
            printf("Client send failed, removing socket %d\n", clients[i]);
            close(clients[i]);
            remove_client(clients[i]);
        }
    }

    pthread_mutex_unlock(&lock);
}

/* Client receive thread */
void *client_thread(void *arg)
{
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];

    while(1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);

        if(bytes <= 0)
        {
            printf("Client %d disconnected\n", client_sock);
            break;
        }

        buffer[strcspn(buffer,"\n")] = 0;

        printf("Client %d -> %s\n", client_sock, buffer);
    }

    close(client_sock);
    remove_client(client_sock);

    pthread_exit(NULL);
}

/* Server broadcast input thread */
void *server_broadcast_thread(void *arg)
{
    char message[BUFFER_SIZE];

    while(1)
    {
        printf("\nServer broadcast: ");
        fflush(stdout);

        if(fgets(message, BUFFER_SIZE, stdin) == NULL)
            continue;

        broadcast(message);
    }

    return NULL;
}

int main()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

    listen(server_sock,10);

    printf("\nServer started on port %d\n\n", PORT);

    pthread_t broadcast_tid;
    pthread_create(&broadcast_tid, NULL, server_broadcast_thread, NULL);

    while(1)
    {
        int client_sock = accept(server_sock,
                                 (struct sockaddr*)&client_addr,
                                 &addr_len);

        if(client_sock < 0)
            continue;

        printf("Client connected: %s:%d  (socket %d)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_sock);

        pthread_mutex_lock(&lock);

        if(client_count < MAX_CLIENTS)
            clients[client_count++] = client_sock;

        pthread_mutex_unlock(&lock);

        pthread_t tid;

        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;

        pthread_create(&tid,NULL,client_thread,pclient);
        pthread_detach(tid);
    }

    close(server_sock);
}

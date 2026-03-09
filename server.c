#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *handle_client(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes;

    printf("Client handler thread started\n");

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        bytes = recv(client_sock, buffer, BUFFER_SIZE, 0);

        if (bytes <= 0)
        {
            printf("Client disconnected\n");
            break;
        }

        printf("Client says: %s", buffer);

        send(client_sock, buffer, strlen(buffer), 0);
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    if (listen(server_sock, 10) < 0)
    {
        perror("Listen failed");
        close(server_sock);
        exit(1);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        int client_sock = accept(server_sock,
                                 (struct sockaddr *)&client_addr,
                                 &addr_len);

        if (client_sock < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        pthread_t tid;

        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;

        if (pthread_create(&tid, NULL, handle_client, pclient) != 0)
        {
            perror("Thread creation failed");
            close(client_sock);
            free(pclient);
        }

        pthread_detach(tid);
    }

    close(server_sock);

    return 0;
}

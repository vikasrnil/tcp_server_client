#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE + 1];

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("Socket");
        return -1;
    }

    /* Configure server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        return -1;
    }

    /* Connect to server */
    if(connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0)
    {
        perror("Connect");
        return -1;
    }

    printf("Connected to CAN broadcast server\n\n");

    /* Receive loop */
    while(1)
    {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

        if(bytes < 0)
        {
            perror("recv");
            break;
        }

        if(bytes == 0)
        {
            printf("Server disconnected\n");
            break;
        }

        buffer[bytes] = '\0';

        printf("%s", buffer);
        fflush(stdout);
    }

    close(sock);

    return 0;
}

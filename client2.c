#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr);

    connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

    printf("Connected to CAN server\n");

    while(1)
    {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

        if(bytes <= 0)
        {
            printf("Server disconnected\n");
            break;
        }

        buffer[bytes] = '\0';

        printf("%s", buffer);
    }

    close(sock);

    return 0;
}

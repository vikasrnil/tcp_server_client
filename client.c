#include <stdio.h>
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

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server\n");

    while (1)
    {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

        if (bytes <= 0)
        {
            printf("Server disconnected\n");
            break;
        }

        printf("Server: %s\n", buffer);
    }

    close(sock);

    return 0;
}

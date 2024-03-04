#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

void *listen_keyboard(void *arg)
{
    int client_fd = *((int *)arg);
    char buffer[1024];

    while (1)
    {
        printf("Enter message:\n");
        fgets(buffer, sizeof(buffer), stdin);
        send(client_fd, buffer, strlen(buffer), 0); // Send data to server
    }

    pthread_exit(NULL);
}

void *listen_server(void *arg)
{
    int client_fd = *((int *)arg);
    char buffer[1024];

    while (1)
    {
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
                printf("Server disconnected\n");
            else
                perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            buffer[bytes_received] = '\0';
            printf("Message: %s\n", buffer);
        }
    }

    pthread_exit(NULL);
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr;
    pthread_t server_tid, keyboard_tid;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    pthread_create(&server_tid, NULL, listen_server, (void *)&client_fd);
    pthread_create(&keyboard_tid, NULL, listen_keyboard, (void *)&client_fd);

    pthread_join(server_tid, NULL);
    pthread_join(keyboard_tid, NULL);

    close(client_fd);
    return 0;
}

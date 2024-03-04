#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proactor.h" // Include proactor header
#include "proactor.c"

#define PORT 8080
#define MAX_CLIENTS 100
#define SERVER_IP "127.0.0.1"

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int clients[MAX_CLIENTS];
Proactor *proactor;
int size=0;

void handle_client(void *arg)
{
    int sockfd = *(int *)arg;
    char buffer[1024];
    int read_size;

    while ((read_size = recv(sockfd, buffer, 1024, 0)) > 0)
    {
        buffer[read_size] = '\0';
        printf("Client %d sent a message: %s\n", sockfd, buffer);
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < size; i++)
        {
            if (clients[i] != sockfd)
            {
                printf("Forwarding message from %d to %d\n", sockfd, clients[i]);
                send(clients[i], buffer, strlen(buffer), 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < size; i++)
    {
        if (clients[i] == sockfd)
        {
            while (i < size - 1)
            {
                clients[i] = clients[i + 1];
                i++;
            }
            size--;
            printf("Client %d disconnected\n", sockfd);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(sockfd);
    
    pthread_exit(NULL);
}

// Function to accept clients using proactor
void *accept_clients(void *arg)
{
    int server_fd = *(int *)arg;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("New client connected: socket %d\n", new_socket);
        printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        
        clients[size++] = new_socket;
        // Submit the task of handling client to proactor
        proactor_submit_task(proactor, handle_client, &new_socket);
    }
    return NULL;
}

int main()
{
    int server_fd;
    struct sockaddr_in address;

    // Your server initialization code goes here

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Start the server and accept clients using proactor
    pthread_t accept_thread;
    proactor = proactor_create();
    if (pthread_create(&accept_thread, NULL, accept_clients, (void *)&server_fd) < 0)
    {
        perror("could not create accept thread");
        exit(EXIT_FAILURE);
    }
    // Your existing server logic ends here

    // Cleanup
    pthread_join(accept_thread, NULL);
    proactor_destroy(proactor);

    return 0;
}
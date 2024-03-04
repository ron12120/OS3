#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define SERVER_IP "127.0.0.1"

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int clients[MAX_CLIENTS];
int n_clients = 0;

void *handle_client(void *arg)
{
    int sockfd = *(int *)arg;
    char buffer[1024];
    int read_size;

    while ((read_size = recv(sockfd, buffer, 1024, 0)) > 0)
    {
        buffer[read_size] = '\0';
        printf("Client %d sent a message: %s\n", sockfd, buffer);
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < n_clients; i++)
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
    for (int i = 0; i < n_clients; i++)
    {
        if (clients[i] == sockfd)
        {
            while (i < n_clients - 1)
            {
                clients[i] = clients[i + 1];
                i++;
            }
            n_clients--;
            printf("Client %d disconnected\n", sockfd);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(sockfd);
    free(arg);
    pthread_exit(NULL);
}

void *accept_clients(void *server_fd)
{
    int new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int *new_sock;

    while (1)
    {
        if ((new_socket = accept(*(int *)server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New client connected: socket %d\n", new_socket);
        printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        pthread_mutex_lock(&clients_mutex);
        if (n_clients < MAX_CLIENTS)
        {
            clients[n_clients++] = new_socket;
            pthread_mutex_unlock(&clients_mutex);

            pthread_t tid;
            new_sock = malloc(1);
            *new_sock = new_socket;
            if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) < 0)
            {
                perror("could not create thread");
                return NULL;
            }
        }
        else
        {
            pthread_mutex_unlock(&clients_mutex);
            printf("Max clients reached.\n");
            close(new_socket);
        }
    }

    return NULL;
}

int main()
{
    int server_fd;
    struct sockaddr_in address;

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

    pthread_t accept_thread;
    if (pthread_create(&accept_thread, NULL, accept_clients, (void *)&server_fd) < 0)
    {
        perror("could not create accept thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(accept_thread, NULL);

    return 0;
}

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 128
#define MAX_CLIENTS 10

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int server_fd;
    struct sockaddr_in server_address;
    socklen_t server_address_lenght = sizeof(server_address);

    char buffer[BUFFER_SIZE];

    int clients_fds[MAX_CLIENTS] = {0};

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // create socket
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Could not create the socket");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind
    if(bind(server_fd, (struct sockaddr *)&server_address, server_address_lenght) < 0)
    {
        perror("Could not bind the address to socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // listen
    if(listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
   
    fd_set read_fds;
    int max_fd;
    while(true)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        // add clients to the read set read_fds
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(clients_fds[i] > 0)
            {
                FD_SET(clients_fds[i], &read_fds);
                if(clients_fds[i] > max_fd)
                {
                    max_fd = clients_fds[i];
                }
            }
        }

        // use select to monitor activities
        if(select(max_fd + 1, &read_fds, NULL, NULL, &timeout) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // new connection if the server_fd is ready for reading and there is activities in the read set
        if(FD_ISSET(server_fd, &read_fds))
        {
            int new_client_fd;
            if((new_client_fd = accept(server_fd, (struct sockaddr *)&server_address, &server_address_lenght)) < 0)
            {
                perror("accept");
                continue;
            }
            set_nonblocking(new_client_fd);

            // add the client to the clients_fds
            for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(clients_fds[i] == 0)
                {
                    clients_fds[i] = new_client_fd;
                    break;
                }
            }
        }   

        // manage the existing clients
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(FD_ISSET(clients_fds[i], &read_fds))
            {
                int bytes_received = recv(clients_fds[i], buffer, BUFFER_SIZE - 1, 0);
                if(bytes_received <= 0)
                {
                    close(clients_fds[i]);
                    clients_fds[i] = 0; // remove the client
                    
                }
                else
                {
                    buffer[bytes_received] = '\0'; // null terminate the string
                    printf("[CLIENT %d]: %s\n", clients_fds[i], buffer);
                }
            }
        }
    
    }

    return 0;
}
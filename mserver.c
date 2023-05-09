#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   

#define MAX_CLIENTS 10
#define BUFF_LENGTH 1024

int main(int argc, char* argv[])
{
    int listen_socket;
    int clients[MAX_CLIENTS] = { -1 };
    unsigned int port;

    struct sockaddr_in server_address, client_address;

    socklen_t addr_length = sizeof(struct sockaddr);

    fd_set read_fds;
    int maxfd = 0;
    int active_clients = 0, num_active = 0;
    int received, sent, recvd_from;


    char buffer[BUFF_LENGTH];

    printf("Validating arguments...\n");
    if (argc != 2)
    {
        printf("Missing arguments: %s <port>\n", argv[0]);
        exit(1);
    }
    printf("Arguments passed!\n");

    port = atoi(argv[1]);

    printf("Validating connection port...\n");
    if (port < 1 || port > 65535)
    {
        printf("Port number out of range. Allowed: [1,65535]");
        exit(1);
    }
    printf("Connection port validated!\n");

    printf("Creating socket...\n");
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0)
    {
        fprintf(stderr, "Failed while creating socket. Error: %d\n", errno);
    }
    printf("Successful!\n");

    // clear socket memory and fill with addr struct
    memset(&server_address, 0, sizeof(server_address));

    // set protocol, address(to all network interfaces), port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    printf("Binding to socket...\n");
    if (bind(listen_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        fprintf(stderr, "Failed while binding socket. Error: %d", errno);
        exit(1);
    }
    printf("Binded!\n");

    printf("Start listening...\n");
    listen(listen_socket, 5);

    
    for (;;)
    {
        // init and clear the socket set
        FD_ZERO(&read_fds);
        // add listen socket to the set
        FD_SET(listen_socket, &read_fds);
        if (listen_socket > maxfd)
            maxfd = listen_socket; // dont forget to check in case other are higher

        printf("Adding clients to read list...\n");
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] != -1)
            {   // add client to read list         
                FD_SET(clients[i], &read_fds);
                if (clients[i] > maxfd)
                    maxfd = clients[i];
            }
        }
        //printf("#0: %s\n", strerror(errno));

        printf("Starting to monitor socket activity...\n");
        // maxfd + 1 to make sure to monitor all file descriptors (sent data/disconnected)
        if (select(maxfd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            printf("Failed while trying to monitor socket activity\n");
            exit(1);
        }
        //printf("after select: %s\n", strerror(errno)); errno = 0;

        if (FD_ISSET(listen_socket, &read_fds))
        {
            printf("fdisset proc\n");
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                if (clients[i] == -1)
                {
                    // clear buffer and client address struct
                    memset(&client_address, 0, sizeof(client_address));
                    memset(&buffer, 0, BUFF_LENGTH);
                    clients[i] = accept(listen_socket, (struct sockaddr*)&client_address, &addr_length);
                    printf("Peer %s connected.\n", inet_ntoa(client_address.sin_addr));
                    break;
                }
            }
        }
        //printf("After isset: %s\n", strerror(errno));
        
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] != -1 && FD_ISSET(clients[i], &read_fds))
            {
                memset(buffer, 0, BUFF_LENGTH);
                received = recv(clients[i], &buffer, BUFF_LENGTH, 0);

                recvd_from = i;
                printf("Peer sent: %i %s\n", received, buffer);

                //printf("After 1st recv: %s\n", strerror(errno));
                for (int j = 0; j < MAX_CLIENTS; ++j)
                {
                    //  check if socket descriptor is set and msg is valid
                    if (clients[j] != -1 && (j != recvd_from || received <= 0) && buffer[0] != '\n' && buffer[0] != '\r')
                    {
                        sent = send(clients[j], buffer, received, 0);
                        if (sent <= 0) {
                            close(clients[j]);
                            clients[j] = -1;
                            //printf("Peer not connected.\n");
                        }
                    }
                }
            }
        }
        //printf("After msg check: %s\n", strerror(errno));
    }

    return 1;
}

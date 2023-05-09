#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFF_LENGTH 1024

int main(int argc, char* argv[])
{
    int client_socket;
    unsigned int port;

    struct sockaddr_in server_address;

    char buffer[1024];

    fd_set read_fds;

    struct timeval timeout;

    if (argc != 3)
    {
        fprintf(stderr, "Missing arguments: %s <ip> <port>", argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);

    if (port < 1 || port > 65535)
    {
        printf("Port number out of range. Allowed: [1,65535]");
        exit(1);
    }

    printf("Creating socket...\n");
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        fprintf(stderr, "Failed while creating socket. Error: %d\n", errno);
    }
    printf("Successful!\n");

    // clear socket memory and fill with addr struct
    memset(&server_address, 0, sizeof(server_address));
    // set protocol(IPv4) and port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_aton(argv[1], &server_address.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address.\n");
        exit(1);
    }

    printf("Connecting to server...\n");
    int i_result = connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (i_result < 0)
    {
        if (errno != EINPROGRESS)
        {
            fprintf(stderr, "Failed while connecting to server: %s\n", strerror(errno));
            exit(1);
        }
    }
    printf("Connected!\n");

    // set stdin as non-blocking so messages are received real time
    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NONBLOCK);
    for (;;)
    {
        // init read_fds descriptors and fill with 0
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(client_socket, &read_fds);


        // printf("Waiting for activity...\n");
        if (select(client_socket + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            fprintf(stderr, "Failed while selecting fd.\n");
            exit(1);
        }
        // printf("Select passed\n");

        if (FD_ISSET(client_socket, &read_fds))
        {
            // read message from server and print to console
            memset(buffer, 0, BUFF_LENGTH);
            if (recv(client_socket, buffer, BUFF_LENGTH, 0) == 0) {
                fprintf(stdin, "\n\nServer disconnected\n");
                break;
            }
            else if (buffer[0] != 13 || buffer[0] != 10)
            {
                printf("Received: %s", buffer);
            }
        }
        else if (FD_ISSET(0, &read_fds)) // check if event occured on stdin
        {
            memset(buffer, 0, BUFF_LENGTH);
            // read inputfrom user
            fgets(buffer, BUFF_LENGTH, stdin);
            // send input
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }

    close(client_socket);

    return 1;
}
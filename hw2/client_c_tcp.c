/*

Citations:

I copied anything socket related from the socket.h manpage.
This includes usage of socket(), AF_INET, SOCK_STREAM, send(), recv(), connect(), close().
socket.h manpage: https://man7.org/linux/man-pages/man2/socket.2.html

I copied anything address handling related from the arpa/inet.h manpage and netinet/in.h manpage. 
This includes usage of the sockaddr_in struct, inet_addr(), htons().
arpa/inet.h manpage: https://man7.org/linux/man-pages/man0/arpa_inet.h.0p.html
netinet/in.h manpage: https://man7.org/linux/man-pages/man0/netinet_in.h.0p.html

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Returns true if the client should stop awaiting responses from the server
int server_finish(char* response) {
    if (response[13] == 'S' || strlen(response) <= 14) {
        return 1;
    }
    return 0;
}

// ARGS: ip address of server, port number
int main(int argc, char** argv) {

    if (argc != 3) {
        printf("Invalid Arguments\n");
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    const int PORT = atoi(argv[2]);
    const int MAX_BUFFER_SIZE = 1024;
    char buffer[MAX_BUFFER_SIZE];

    // Configure client socket:
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address:
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_address.sin_port = htons(PORT);

    // Get user input:
    printf("Enter string: ");
    fgets(buffer, MAX_BUFFER_SIZE, stdin);
    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
    }

    // Handshake connect with server:
    int con_res = connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (con_res == -1) {
        printf("Connection failed\n");
        exit(EXIT_FAILURE);
    }
    // printf("Connected to server at %s:%d\n", SERVER_IP, PORT);

    // Send and receive from server:
    send(client_socket, buffer, strlen(buffer), 0);
    int received_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[received_bytes] = '\0';
    printf("%s\n", buffer);

    while (!server_finish(buffer)) {
        usleep(1000);
        memset(buffer, 0, sizeof(buffer));
        received_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        buffer[received_bytes] = '\0';
        printf("%s\n", buffer);
    }

    // Close client socket:
    close(client_socket);

    return 0;
}

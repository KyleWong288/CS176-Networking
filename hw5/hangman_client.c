#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

const int MAX_BUFFER_SIZE = 1024;


struct server_packet {
    char msg_flag;
    char word_length;
    char num_incorrect;
    char* payload;
};


struct client_packet {
    char msg_length;
    char* payload;
};


// Serializes a client_packet struct
void serialize(struct client_packet data, char* buffer) {
    memcpy(buffer, &data.msg_length, 1);
    strcpy(buffer + 1, data.payload);
}


// Deserializes a server_packet struct
struct server_packet deserialize(char* buffer) {
    struct server_packet res;
    res.payload = malloc(MAX_BUFFER_SIZE);
    memcpy(&res.msg_flag, buffer, 1);
    memcpy(&res.word_length, buffer + 1, 1);
    memcpy(&res.num_incorrect, buffer + 2, 1);
    strcpy(res.payload, buffer + 3);
    return res;
}


// ARGS: IP of host, port number
int main(int argc, char* argv[]) {
    
    // Parse arguments:
    if (argc != 3) {
        printf("ERROR: Invalid Arguments\n");
        exit(1);
    }
    const char* SERVER_IP = argv[1];
    const int PORT = atoi(argv[2]);

    // buffer is for sending messages between server and client
    char buffer[MAX_BUFFER_SIZE];
    // input is for reading user input from terminal
    char input[MAX_BUFFER_SIZE];

    // Configure client socket:
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address:
    struct sockaddr_in server_addr;
    struct hostent *server;
    server = gethostbyname(SERVER_IP);
    if (server == 0) {
        printf("ERROR: Invalid host name\n");
        exit(1);
    }
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr_list[0], (char*)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(PORT);

    // TCP connection:
    if (connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        printf("ERROR: TCP connection failure\n");
        exit(1);
    }

    // Check if the server is overloaded:
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    int over = select(sockfd + 1, &read_fds, 0, 0, &timeout);
    if (over) {
        bzero(buffer, MAX_BUFFER_SIZE);
        read(sockfd, buffer, MAX_BUFFER_SIZE);
        struct server_packet response = deserialize(buffer);
        printf(">>>%.*s", response.word_length, response.payload);
        close(sockfd);
        return 0;
    }

    // Send the ready up phase message:
    printf(">>>Ready to start game? (y/n): ");
    memset(input, 0, sizeof(input));
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("\n");
        close(sockfd);
        return 0;
    }
    if (strlen(input) > 0 && input[strlen(input)-1] == '\n') {
        input[strlen(input)-1] = '\0';
    }
    if (strcmp(input, "y") == 0) {
        struct client_packet message;
        message.msg_length = 0;
        message.payload = malloc(MAX_BUFFER_SIZE);
        strcpy(message.payload, "START");
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(message, buffer);
        write(sockfd, buffer, sizeof(buffer));
    }
    else {
        close(sockfd);
        return 0;
    }

    // Receive the ready up phase response:
    bzero(buffer, MAX_BUFFER_SIZE);
    int bytes_read = read(sockfd, buffer, MAX_BUFFER_SIZE);
    if (bytes_read <= 0) {
        close(sockfd);
        return 0;
    }
    struct server_packet response = deserialize(buffer);

    // Simulate the gameplay:
    while (response.msg_flag == 0) {

        // Print server response and get input for the next turn:
        printf(">>>");
        for (int i=0; i<response.word_length; ++i) {
            printf("%c", response.payload[i]);
            if (i < response.word_length-1) {
                printf(" ");
            }
        }
        printf("\n>>>Incorrect Guesses: ");
        for (int i=0; i<response.num_incorrect; ++i) {
            printf("%c", response.payload[response.word_length + i]);
            if (i < response.num_incorrect-1) {
                printf(" ");
            }
        }
        printf("\n>>>\n");
        printf(">>>Letter to guess: ");
        memset(input, 0, sizeof(input));
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            close(sockfd);
            return 0;
        }
        if (strlen(input) > 0 && input[strlen(input)-1] == '\n') {
            input[strlen(input)-1] = '\0';
        }
        while (strlen(input) != 1 || !isalpha(*input)) {
            printf(">>>Error! Please guess one letter.\n");
            printf(">>>Letter to guess: ");
            memset(input, 0, sizeof(input));
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("\n");
                close(sockfd);
                return 0;
            }
            if (strlen(input) > 0 && input[strlen(input)-1] == '\n') {
                input[strlen(input)-1] = '\0';
            }
        }
        input[0] = tolower(input[0]);

        // Send the client message:
        struct client_packet message;
        message.msg_length = strlen(input);
        message.payload = malloc(MAX_BUFFER_SIZE);
        strcpy(message.payload, input);
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(message, buffer);
        write(sockfd, buffer, sizeof(buffer));

        // Receive the server response:
        bzero(buffer, MAX_BUFFER_SIZE);
        bytes_read = read(sockfd, buffer, MAX_BUFFER_SIZE);
        if (bytes_read <= 0) {
            close(sockfd);
            return 0;
        }
        response = deserialize(buffer);
    }

    // Print and receive the finishing messages:
    printf(">>>%.*s\n", response.word_length, response.payload);
    bzero(buffer, MAX_BUFFER_SIZE);
    read(sockfd, buffer, MAX_BUFFER_SIZE);
    response = deserialize(buffer);
    printf(">>>%.*s\n", response.word_length, response.payload);
    bzero(buffer, MAX_BUFFER_SIZE);
    read(sockfd, buffer, MAX_BUFFER_SIZE);
    response = deserialize(buffer);
    printf(">>>%.*s\n", response.word_length, response.payload);

    // Close socket:
    close(sockfd);

    return 0;
}
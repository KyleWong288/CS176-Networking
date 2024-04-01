#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

const int MAX_BUFFER_SIZE = 1024;
const int MAX_GUESSES = 6;
const int MAX_CONNECTIONS = 3;
char** word_choices;
int num_choices;
int num_connections = 0;
pthread_mutex_t mutex;


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


// Serializes a server_packet struct
void serialize(struct server_packet data, char* buffer) {
    memcpy(buffer, &data.msg_flag, 1);
    memcpy(buffer + 1, &data.word_length, 1);
    memcpy(buffer + 2, &data.num_incorrect, 1);
    strcpy(buffer + 3, data.payload);
}


// Deserializes a client_packet struct
struct client_packet deserialize(char* buffer) {
    struct client_packet res;
    res.payload = malloc(MAX_BUFFER_SIZE);
    memcpy(&res.msg_length, buffer, 1);
    strcpy(res.payload, buffer + 1);
    return res;
}


// Reads the word choices from a txt file and saves the strings to memory
void get_word_choices(char*** word_choices, int* length) {
    // Count amount of lines:
    int num_lines = 0;
    char buffer[1024];
    FILE *file = fopen("hangman_words.txt", "r");
    while (fgets(buffer, sizeof(buffer), file)) {
        ++num_lines;
    }
    *length = num_lines;
    fclose(file);

    // Copy lines into array:
    *word_choices = (char**)malloc(num_lines * sizeof(char*));
    file = fopen("hangman_words.txt", "r");
    for (int i=0; i<num_lines; ++i) {
        fgets(buffer, sizeof(buffer), file);
        if (buffer[strlen(buffer)-1] == '\n') {
            buffer[strlen(buffer)-1] = '\0';
        }
        (*word_choices)[i] = strdup(buffer);
    }
    fclose(file);
}


// Closes the socket, decreases the number of active connections
void close_connection(int client_sockfd) {
    pthread_mutex_lock(&mutex);
    --num_connections;
    pthread_mutex_unlock(&mutex);
    close(client_sockfd);
}

// Simulates a hangman game with the given client address
void *play_hangman(void* arg) {

    int client_sockfd = *((int* )arg);
    char buffer[MAX_BUFFER_SIZE];

    // Check if the server has at most 3 connections:
    int over = 0;
    pthread_mutex_lock(&mutex);
    ++num_connections;
    if (num_connections > 3) {
        over = 1;
    }
    pthread_mutex_unlock(&mutex);
    if (over) {
        struct server_packet response;
        response.msg_flag = 1;
        response.word_length = 25;
        response.num_incorrect = 0;
        response.payload = malloc(MAX_BUFFER_SIZE);
        strcpy(response.payload, "server-overloaded\n");
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(response, buffer);
        write(client_sockfd, buffer, sizeof(buffer));
        close_connection(client_sockfd);
        return 0;
    }

    // Pick a fixed word:
    int rand_idx = rand() % num_choices;
    char game_word[MAX_BUFFER_SIZE];
    strcpy(game_word, word_choices[rand_idx]);
    int chars_left = strlen(game_word);

    // Receive and respond to the ready up phase message:
    bzero(buffer, MAX_BUFFER_SIZE);
    int bytes_read = read(client_sockfd, buffer, MAX_BUFFER_SIZE);
    if (bytes_read <= 0) {
        close_connection(client_sockfd);
        return 0;
    }
    struct client_packet message = deserialize(buffer);
    struct server_packet response;
    response.msg_flag = 0;
    response.word_length = strlen(game_word);
    response.num_incorrect = 0;
    response.payload = malloc(MAX_BUFFER_SIZE);
    memset(response.payload, '_', MAX_BUFFER_SIZE);
    bzero(buffer, MAX_BUFFER_SIZE);
    serialize(response, buffer);
    write(client_sockfd, buffer, sizeof(buffer));

    // Simulate the gameplay:
    while (1) {

        // Receive the client guess:
        bzero(buffer, MAX_BUFFER_SIZE);
        bytes_read = read(client_sockfd, buffer, MAX_BUFFER_SIZE);
        if (bytes_read <= 0) {
            close_connection(client_sockfd);
            return 0;
        }
        message = deserialize(buffer);
        
        // Update the state of the word:
        int marked = 0;
        char guess = message.payload[0];
        for (int i=0; i<response.word_length; ++i) {
            if (game_word[i] == guess && response.payload[i] == '_') {
                ++marked;
                response.payload[i] = guess;
            }
        }
        
        // If nothing was marked, update the state of wrong guesses
        chars_left -= marked;
        if (marked == 0) {
            int write_idx = response.word_length + response.num_incorrect;
            response.payload[write_idx] = guess;
            response.num_incorrect += 1;
            if (response.num_incorrect >= MAX_GUESSES) {
                break;
            }
        }

        if (chars_left <= 0 || response.num_incorrect >= MAX_GUESSES) {
            break;
        }

        // Send the server response:
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(response, buffer);
        write(client_sockfd, buffer, sizeof(buffer));

    }

    // Send the ground truth answer:
    response.msg_flag = 1;
    response.word_length += 14;
    sprintf(buffer, "The word was %s", game_word);
    strcpy(response.payload, buffer);
    bzero(buffer, MAX_BUFFER_SIZE);
    serialize(response, buffer);
    write(client_sockfd, buffer, sizeof(buffer));

    // Handle a win case:
    if (chars_left <= 0) {
        response.word_length = 9;
        strcpy(response.payload, "You Win!");
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(response, buffer);
        write(client_sockfd, buffer, sizeof(buffer));
    }
    // Handle a lose case:
    else {
        response.word_length = 10;
        strcpy(response.payload, "You Lose!");
        bzero(buffer, MAX_BUFFER_SIZE);
        serialize(response, buffer);
        write(client_sockfd, buffer, sizeof(buffer));
    }

    // Send the game over message:
    response.word_length = 11;
    strcpy(response.payload, "Game Over!");
    bzero(buffer, MAX_BUFFER_SIZE);
    serialize(response, buffer);
    write(client_sockfd, buffer, sizeof(buffer));

    // Close socket:
    close_connection(client_sockfd);
    return 0;
}


// ARGS: none for gradescope
int main(int argc, char* argv[]) {

    // Read in word choices:
    get_word_choices(&word_choices, &num_choices);

    // Configure server socket:
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address:
    struct sockaddr_in server_addr, client_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("ERROR: Failed to bind\n");
        exit(1);
    }

    // Listen for incoming connection requests:
    listen(sockfd, 3);

    while (1) {

        // Accept incoming connection requests:
        socklen_t client_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            printf("ERROR: Failed to create child socket\n");
            exit(1);
        }

        // Play hangman async:
        pthread_t thread;
        pthread_create(&thread, NULL, play_hangman, (void*)&client_sockfd);
    }

    // Close listening socket:
    close(sockfd);

    return 0;
}
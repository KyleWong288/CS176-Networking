/*

Citations:

I copied anything socket related from the socket.h manpage.
This includes usage of socket(), AF_INET, SOCK_DGRAM, sendto(), recvfrom(), close().
socket.h manpage: https://man7.org/linux/man-pages/man2/socket.2.html

I copied host name mapping and usage from the linuxhowtos page from the class videos.
This includes usage of the hostent struct and gethostbyname().
linuxhowto page for udp client: https://www.linuxhowtos.org/data/6/client_udp.c

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
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

// ARGS: name of host, port number
int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Invalid Arguments\n");
        exit(EXIT_FAILURE);
    }

    const char* HOST_NAME = argv[1];
    const int PORT = atoi(argv[2]);
    const int MAX_BUFFER_SIZE = 1024;
    char buffer[MAX_BUFFER_SIZE];

    // Configure client socket:
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // Configure timer for client socket:
    struct timeval timeout;
    timeout.tv_sec = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Configure server address:
    struct sockaddr_in server_address;
    struct hostent *hp;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    hp = gethostbyname(HOST_NAME);
    if (hp == 0) {
        printf("ERROR: Invalid host name\n");
        exit(EXIT_FAILURE);
    }
    bcopy((char *)hp->h_addr_list[0], (char *)&server_address.sin_addr, hp->h_length);
    server_address.sin_port = htons(PORT);

    // Run the 10 pings and track summary metrics:
    int received_packets = 0;
    double rtt_min = 696969.69;
    double rtt_max = 0.0;
    double rtt_sum = 0.0;

    for (int i=1; i<=10; ++i) {
        struct timeval time_start, time_end;
        gettimeofday(&time_start, NULL);
        sprintf(buffer, "PING %d %ld", i, time_start.tv_sec);
        sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&server_address, sizeof(server_address));
        int recv_bytes = recvfrom(client_socket, buffer, MAX_BUFFER_SIZE, 0, NULL, NULL);
        gettimeofday(&time_end, NULL);
        double time_elapsed = (time_end.tv_sec - time_start.tv_sec) * 1000;
        time_elapsed += (time_end.tv_usec - time_start.tv_usec) * 1.0 / 1000;
        
        if (recv_bytes < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Request timeout for seq#=%d\n", i); 
            }
            else {
                printf("ERROR (OTHER)\n");
            }
        }
        else {
            buffer[recv_bytes] = '\0';
            printf("PING received from %s: seq#=%d time=%.3f ms\n", HOST_NAME, i, time_elapsed);
            ++received_packets;
            if (time_elapsed < rtt_min) rtt_min = time_elapsed;
            if (time_elapsed > rtt_max) rtt_max = time_elapsed;
            rtt_sum += time_elapsed;
        }
    }

    // Print summary metrics:
    const char *ip_string = inet_ntoa(server_address.sin_addr);
    printf("--- %s ping statistics ---\n", ip_string);
    printf("10 packets transmitted, %d received, %d%% packet loss", 
        received_packets, 
        100 * (10 - received_packets) / 10);
    if (received_packets) {
        printf(" rtt min/avg/max = %.3f %.3f %.3f ms",
            rtt_min,
            rtt_sum / received_packets,
            rtt_max);
    }
    printf("\n");

    // Close client socket:
    close(client_socket);

    return 0;
}
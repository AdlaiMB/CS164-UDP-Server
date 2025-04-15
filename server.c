/* UCR CS164 Go-Back-N Project
 * UDP server
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_PKT_NUM 255

/* Define control flags */
#define SYN 1
#define SYN_ACK 2
#define ACK 3
#define RST 4

/* Packet data structure */
typedef struct {
  int seq;      /* Sequence number */
  int ack;      /* Acknowledgement number */
  int flag;     /* Control flag. Indicate type of packet */
  char payload; /* Data payload (1 character for this project) */
} Packet;

int min(int a, int b) {
    return (a < b) ? a : b;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <listen_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int port = atoi(argv[1]);

    /* Create socket file descriptor */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Error opening socket");
        exit(EXIT_FAILURE);
    }
    
    /* Setup server address structure */
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); 

    /* Bind the socket */ 
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    Packet send_packet, recv_packet;
    int cur_seq, cur_ack;
    int bytes_sent, bytes_recv;

    /* Step 1: Perform the three-way handshake */
    printf("----------Handshake----------\n\n");
    while (1) {
        bytes_recv = recvfrom(sockfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, &addrlen);
        if (bytes_recv < 0) {
            printf("Receive error\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (recv_packet.flag != SYN) {
            printf("Expected SYN packet! Received %d insted, ignoring\n", recv_packet.flag);
        }
        else {
            printf("Received SYN packet\n");
            break;
        }
    }

    cur_seq = recv_packet.seq;
    cur_ack = recv_packet.ack;
    send_packet.seq = cur_seq;
    send_packet.ack = cur_ack;
    send_packet.flag = SYN_ACK;
    send_packet.payload = 0;
    bytes_sent = sendto(sockfd, &send_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, addrlen);
    if (bytes_sent < 0) {
        printf("Send error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Sent SYN-ACK packet\n");

    while (1) {
        bytes_recv = recvfrom(sockfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, &addrlen);
        if (bytes_recv < 0) {
            printf("Receive error\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (recv_packet.flag != ACK) {
            printf("Expected ACK packet! Received %d insted, ignoring\n", recv_packet.flag);
        }
        else {
            if (recv_packet.ack != cur_ack) {
                printf("received incorrect ACK, ignoring\n");
            }
            else {
                printf("Received ACK packet, handshake complete\n");
                break;
            }
        }
    }

    /* Step 2: Transmission Setup */
    printf("\n----------Transmission Setup----------\n\n");
    int MAX_WINDOW_SIZE, BYTES_REQUEST;

    bytes_recv = recvfrom(sockfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, &addrlen);
    if (bytes_recv < 0) {
        printf("Receive error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 
    MAX_WINDOW_SIZE = recv_packet.payload;
    cur_ack = recv_packet.seq;
    printf("Window Size (N) = %d\n", MAX_WINDOW_SIZE);

    bytes_recv = recvfrom(sockfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, &addrlen);
    if (bytes_recv < 0) {
        printf("Receive error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 
    BYTES_REQUEST = recv_packet.payload;
    cur_ack = recv_packet.seq;
    printf("Byte Request (S) = %d\n", BYTES_REQUEST);

    /* Step 3: Transmission */
    printf("\n----------Transmission----------\n\n");
    int window_size = MAX_WINDOW_SIZE;
    int conn_alive = 1;
    int base = cur_seq + 1, next_seq_num = cur_seq + 1;
    int succesful_sends = 0; 
    int waiting = 1;

    // construct timer
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    struct timeval timer;
    timer.tv_sec = 2;
    timer.tv_usec = 0;

    while (conn_alive) {
        printf("Current window = %d\n", window_size);
        int window = base + min(window_size, BYTES_REQUEST - base + 1);

        for (int i = next_seq_num; i < window; i++) {
            send_packet.seq = i;
            send_packet.ack = cur_ack;
            send_packet.flag = ACK;
            send_packet.payload = 'A';
            bytes_sent = sendto(sockfd, &send_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, addrlen);
            if (bytes_sent < 0) {
                printf("Send error\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            printf("Sent packet seq=%d, ack=%d\n", send_packet.seq, send_packet.ack);
        }

        next_seq_num = window;

        while (waiting) {
            int ret = select(sockfd + 1, &readfds, NULL, NULL, &timer);
            // a packet has arrived 
            if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
                bytes_recv = recvfrom(sockfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, &addrlen);
                if (bytes_recv < 0) {
                    printf("Send error\n");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                // the correct packet ack has been received 
                if (recv_packet.ack >= base) {
                    printf("Received ACK for packet %d\n\n", recv_packet.ack);
                    base = recv_packet.ack + 1;
                    FD_ZERO(&readfds);
                    FD_SET(sockfd, &readfds);
                    timer.tv_sec = 2;
                    timer.tv_usec = 0;
                    succesful_sends++;
                    waiting = 0;
                }
                else if (recv_packet.ack < base) {
                    /* (corrupted) incorrect packet do nothing */
                }
                
                if (succesful_sends == 2 * window_size) {
                    window_size = MAX_WINDOW_SIZE;
                }
            }
            else {
                /* timeout */
                succesful_sends = 0;
                window_size = MAX_WINDOW_SIZE / 2;
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                timer.tv_sec = 2;
                timer.tv_usec = 0;
                next_seq_num = base;
                waiting = 0;
                printf("Timeout occurred, resending packets from %d\n\n", base);
            }
        }
        waiting = 1;

        if (base > BYTES_REQUEST)  {
            conn_alive = 0;
        }

        sleep(1);
    }

    /* Cleanup */
    send_packet.seq = next_seq_num;
    send_packet.ack = cur_ack;
    send_packet.flag = RST;
    send_packet.payload = 0;
    bytes_sent = sendto(sockfd, &send_packet, sizeof(Packet), 0, (struct sockaddr *) &client_addr, addrlen);
    if (bytes_sent < 0) {
        printf("Send error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Closing connection, RST packet sent\n");
    close(sockfd);

    return EXIT_SUCCESS;
}
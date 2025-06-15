#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "bin_array.h"
#include "cpong_packets.h"
#include "server.h"

int connect_to_server(char *name, char *port);

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server ip/name> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    server_t server = {.fd = connect_to_server(argv[1], argv[2])};
    printf("Connected to server\n");
    struct packet packet = {
        .type = PACKET_PING,
        .size = 1 + 4 + 1,
        .data = {.ping = {
            'p'
        }}
    };
    client_send(server, packet);
    printf("Sent ping packet\n");

    struct binarr barr = {0};
    size_t capacity = packet.size;
    binarr_new(&barr, capacity);
    recv(server.fd, barr.buf, capacity, 0);
    packet_deserialize(&packet, &barr);

    if (packet.type == PACKET_PING) {
        printf("recieved PACKET_PING: type %d; size %u; dummy char %c\n",
               packet.type, packet.size, packet.data.ping.dummy);
    } else {
        printf("recieved unknown packet: %d\n", packet.type);
    }

    return 0;
}

int connect_to_server(char *name, char *port) {
    int status;
    int serverfd;
    struct addrinfo hints;
    struct addrinfo *res;

    // get addrinfo for remote
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(name, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    // select the appropriate addrinfo linked list member
    if (res == NULL) {
        perror("couldn't find host addrinfo (list is empty)");
        return -1;
    }
    struct addrinfo server_ai = *res;
    freeaddrinfo(res);

    // make a socket and connect
    if ((serverfd = socket(server_ai.ai_family, server_ai.ai_socktype, server_ai.ai_protocol)) == -1) {
        perror("failed to create a socket");
        return -1;
    }

    if (connect(serverfd, server_ai.ai_addr, server_ai.ai_addrlen) == -1) {
        perror("failed to connect");
        return -1;
    }
    printf("connected to server\n");
    return serverfd;
}

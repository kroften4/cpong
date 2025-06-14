#include "server.h"
#include "cpong_packets.h"
#include "ts_queue.h"
#include <stdio.h>

void ping(client_t client) {
    for (;;) {
        struct packet packet = {
            .type = PACKET_PING,
            .size = 1 + 4 + 1,
            .data = {.ping = {
                .dummy = 'p'
            }}
        };
        if (server_send_packet(client.server, client, packet) == -1) {
            printf("ping: Connection %d closed\n", client.id);
            return;
        };
        // printf("Sent ping to %d\n", client.id);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>", argv[0]);
    }
    char *port = argv[1];
    server_t server = {0};
    server_set_fd(&server, port);
    server.clients = ts_queue_new();
    printf("Listening on %s\n", port);
    server_worker(&server, ping);
}


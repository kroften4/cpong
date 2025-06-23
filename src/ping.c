#include "cpong_packets.h"

void ping(client_t client);

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


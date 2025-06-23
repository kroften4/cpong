#include "cpong_packets.h"
#include "log.h"

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
            LOGF("ping: Connection %d closed", client.id);
            return;
        };
        // LOGF("Sent ping to %d", client.id);
    }
}


#ifndef __CPONG_PACKETS_H__
#define __CPONG_PACKETS_H__

#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include "ts_queue.h"
#include "bin_array.h"
#include "server.h"

enum cpong_packet {
    PACKET_PING
};

struct packet {
    uint8_t type;
    uint32_t size;
    union packet_data {
        struct {
            char dummy;
        } ping;
    } data;
};

struct binarr *packet_serialize(struct binarr *barr, struct packet packet) {
    binarr_append_i8(barr, packet.type);
    binarr_append_i32_n(barr, packet.size);
    switch (packet.type) {
    case PACKET_PING:
        binarr_append_i8(barr, packet.data.ping.dummy);
        break;
    default:
        return NULL;
    }
    return barr;
}

int server_send_packet_serialized(server_t *server, client_t client, struct binarr barr) {
    if (server_send(server, client, barr)) {
        return -1;
    }
    return 0;
}

int server_send_packet(server_t *server, client_t target, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = sizeof(packet.type) + sizeof(packet.data) + packet.size;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        fprintf(stderr, "unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    if (server_send_packet_serialized(server, target, barr) == -1) {
        binarr_destroy(barr);
        return -1;
    }
    binarr_destroy(barr);

    return 0;
}

int server_broadcast(server_t *server, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = sizeof(packet.type) + sizeof(packet.data) + packet.size;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        fprintf(stderr, "unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    for (struct ts_queue_node *p = server->clients->head; p != NULL; p = p->next) {
        client_t *client = p->data;
        if (server_send_packet_serialized(server, *client, barr) == -1) {
            // TODO: do i need to signal that some clients disconnected?
            continue;
        }
    }
    binarr_destroy(barr);
    return 0;
}

int client_send(server_t server, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = sizeof(packet.type) + sizeof(packet.data) + packet.size;
    binarr_new(&barr, capacity);

    if (packet_serialize(&barr, packet) == NULL) {
        fprintf(stderr, "unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    // TODO: write proper shit for client
    send(server.fd, barr.buf, barr.size, 0);
    binarr_destroy(barr);

    return 0;
}

#endif


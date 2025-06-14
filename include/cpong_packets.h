#ifndef __CPONG_PACKETS_H__
#define __CPONG_PACKETS_H__

#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include "ts_queue.h"
#include "bin_array.h"

enum cpong_packet {
    PACKET_PONG
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

typedef struct {
    int fd;
    struct ts_queue *clients;
} server_t;

typedef struct {
    int fd;
    int id;
} client_t;


struct binarr *packet_serialize(struct binarr *barr, struct packet packet) {
    binarr_append_i8(barr, packet.type);
    binarr_append_i32_n(barr, packet.size);
    switch (packet.type) {
    case PACKET_PONG:
    {
        binarr_append_i8(barr, packet.data.ping.dummy);
    }
    default:
        return NULL;
    }
    return barr;
}

int send_packet_serialized(int fd, struct binarr barr) {
    if (send(fd, barr.buf, barr.size, NULL) == -1) {
        return -1;
    }
    return 0;
}

int server_send(client_t target, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = sizeof(packet.type) + sizeof(packet.data) + packet.size;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        fprintf(stderr, "unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    send_packet_serialized(target.fd, barr);
    binarr_destroy(barr);

    return 0;
}

int server_broadcast(server_t server, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = sizeof(packet.type) + sizeof(packet.data) + packet.size;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        fprintf(stderr, "unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    for (struct ts_queue_node *p = server.clients->head; p != NULL; p = p->next) {
        client_t *client = p->data;
        send_packet_serialized(client->fd, barr);
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
    send_packet_serialized(server.fd, barr);
    binarr_destroy(barr);

    return 0;
}

#endif


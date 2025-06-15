#ifndef __CPONG_PACKETS_H__
#define __CPONG_PACKETS_H__

#include <inttypes.h>
#include "bin_array.h"
#include "server.h"

enum cpong_packet {
    PACKET_PING,
    PACKET_INPUT,
    PACKET_STATE
};

struct packet {
    uint8_t type;
    uint32_t size;
    union packet_data {
        struct ping {
            char dummy;
        } ping;
        struct input {
            int8_t left;
            int8_t right;
        } input;
        struct state {
            struct {
                int32_t id;
                int32_t y;
            } player1;
            struct {
                int32_t id;
                int32_t y;
            } player2;
            struct {
                int32_t x;
                int32_t y;
            } ball;
        } state;
    } data;
};

struct binarr *packet_serialize(struct binarr *barr, struct packet packet);

struct packet *packet_deserialize(struct packet *packet, struct binarr *barr);

int server_send_packet_serialized(server_t *server, client_t client, struct binarr barr);

int server_send_packet(server_t *server, client_t target, struct packet packet);

int server_broadcast(server_t *server, struct packet packet);

int client_send(server_t server, struct packet packet);

#endif


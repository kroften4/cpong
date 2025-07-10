#ifndef __CPONG_PACKETS_H__
#define __CPONG_PACKETS_H__

#include <inttypes.h>
#include "bin_array.h"
#include "server.h"
#include "cpong_logic.h"

#define MAX_PACKET_SIZE 200
#define PACKET_HEADER_SIZE 6

/*
 * CPONG protocol
 * Packet types:
 * PACKET_PING - unused
 * PACKET_INIT - Sent when a match is found and on scoring; contains initial information about game objects, field size, etc.
 * PACKET_STATE - Sent every server tick; contains info about game objects (position, velocity), etc.
 */
enum cpong_packet {
    PACKET_PING,
    PACKET_INPUT,
    PACKET_INIT,
    PACKET_STATE
};

struct packet {
    uint8_t type;
    uint8_t sync;
    uint32_t size;
    union packet_data {
        struct ping {
            char dummy;
        } ping;
        struct input {
            int32_t input_acc_ms;
        } input;
        struct pong_state state;
    } data;
};

struct binarr *packet_serialize(struct binarr *barr, struct packet packet);

struct packet *packet_deserialize(struct packet *packet, struct binarr *barr);

int server_send_packet(server_t *server, client_t target, struct packet packet);

int server_broadcast(server_t *server, struct packet packet);

int client_send(server_t server, struct packet packet);

int recv_packet(int fd, struct packet *result);

#endif


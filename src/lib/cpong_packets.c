#include <inttypes.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include "ts_queue.h"
#include "bin_array.h"
#include "server.h"
#include "cpong_packets.h"
#include "log.h"

struct binarr *packet_serialize(struct binarr *barr, struct packet packet) {
    binarr_append_i8(barr, packet.type);
    barr->index += sizeof(packet.size);
    barr->size = barr->index;
    switch (packet.type) {
    case PACKET_PING:
        binarr_append_i8(barr, packet.data.ping.dummy);
        break;
    case PACKET_INPUT:
        binarr_append_i32_n(barr, packet.data.input.input_acc_ms);
        break;
    case PACKET_INIT:
        binarr_append_i32_n(barr, packet.data.state.player_ids[0]);
        binarr_append_i32_n(barr, packet.data.state.player_ids[1]);

        binarr_append_i32_n(barr, packet.data.state.own_id_index);

        binarr_append_i32_n(barr, packet.data.state.player1.pos.x);
        binarr_append_i32_n(barr, packet.data.state.player1.pos.y);
        binarr_append_i32_n(barr, packet.data.state.player1.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.player1.velocity.y);
        binarr_append_i32_n(barr, packet.data.state.player1.size.x);
        binarr_append_i32_n(barr, packet.data.state.player1.size.y);

        binarr_append_i32_n(barr, packet.data.state.player2.pos.x);
        binarr_append_i32_n(barr, packet.data.state.player2.pos.y);
        binarr_append_i32_n(barr, packet.data.state.player2.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.player2.velocity.y);
        binarr_append_i32_n(barr, packet.data.state.player2.size.x);
        binarr_append_i32_n(barr, packet.data.state.player2.size.y);

        binarr_append_i32_n(barr, packet.data.state.ball.pos.x);
        binarr_append_i32_n(barr, packet.data.state.ball.pos.y);
        binarr_append_i32_n(barr, packet.data.state.ball.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.ball.velocity.y);
        binarr_append_i32_n(barr, packet.data.state.ball.size.x);
        binarr_append_i32_n(barr, packet.data.state.ball.size.y);

        binarr_append_i32_n(barr, packet.data.state.box_size.x);
        binarr_append_i32_n(barr, packet.data.state.box_size.y);
        break;
    case PACKET_STATE:
        binarr_append_i32_n(barr, packet.data.state.player1.pos.y);
        binarr_append_i32_n(barr, packet.data.state.player1.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.player1.velocity.y);

        binarr_append_i32_n(barr, packet.data.state.player2.pos.y);
        binarr_append_i32_n(barr, packet.data.state.player2.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.player2.velocity.y);

        binarr_append_i32_n(barr, packet.data.state.ball.pos.x);
        binarr_append_i32_n(barr, packet.data.state.ball.pos.y);
        binarr_append_i32_n(barr, packet.data.state.ball.velocity.x);
        binarr_append_i32_n(barr, packet.data.state.ball.velocity.y);
        break;
    default:
        return NULL;
    }
    packet.size = barr->index - PACKET_HEADER_SIZE;
    barr->index = sizeof(packet.type);
    binarr_write_i32_n(barr, packet.size);
    barr->index = barr->size;
    return barr;
}

struct packet *packet_deserialize(struct packet *packet, struct binarr *barr) {
    barr->index = 0;
    packet->type = binarr_read_i8(barr);
    packet->size = binarr_read_i32_n(barr);
    switch (packet->type) {
    case PACKET_PING:
        packet->data.ping.dummy = binarr_read_i8(barr);
        break;
    case PACKET_INPUT:
        packet->data.input.input_acc_ms = binarr_read_i32_n(barr);
        break;
    case PACKET_INIT:
        packet->data.state.player_ids[0] = binarr_read_i32_n(barr);
        packet->data.state.player_ids[1] = binarr_read_i32_n(barr);

        packet->data.state.own_id_index = binarr_read_i32_n(barr);

        packet->data.state.player1.pos.x = binarr_read_i32_n(barr);
        packet->data.state.player1.pos.y = binarr_read_i32_n(barr);
        packet->data.state.player1.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.player1.velocity.y = binarr_read_i32_n(barr);
        packet->data.state.player1.size.x = binarr_read_i32_n(barr);
        packet->data.state.player1.size.y = binarr_read_i32_n(barr);

        packet->data.state.player2.pos.x = binarr_read_i32_n(barr);
        packet->data.state.player2.pos.y = binarr_read_i32_n(barr);
        packet->data.state.player2.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.player2.velocity.y = binarr_read_i32_n(barr);
        packet->data.state.player2.size.x = binarr_read_i32_n(barr);
        packet->data.state.player2.size.y = binarr_read_i32_n(barr);

        packet->data.state.ball.pos.x = binarr_read_i32_n(barr);
        packet->data.state.ball.pos.y = binarr_read_i32_n(barr);
        packet->data.state.ball.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.ball.velocity.y = binarr_read_i32_n(barr);
        packet->data.state.ball.size.x = binarr_read_i32_n(barr);
        packet->data.state.ball.size.y = binarr_read_i32_n(barr);

        packet->data.state.box_size.x = binarr_read_i32_n(barr);
        packet->data.state.box_size.y = binarr_read_i32_n(barr);
        break;
    case PACKET_STATE:
        packet->data.state.player1.pos.y = binarr_read_i32_n(barr);
        packet->data.state.player1.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.player1.velocity.y = binarr_read_i32_n(barr);

        packet->data.state.player2.pos.y = binarr_read_i32_n(barr);
        packet->data.state.player2.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.player2.velocity.y = binarr_read_i32_n(barr);

        packet->data.state.ball.pos.x = binarr_read_i32_n(barr);
        packet->data.state.ball.pos.y = binarr_read_i32_n(barr);
        packet->data.state.ball.velocity.x = binarr_read_i32_n(barr);
        packet->data.state.ball.velocity.y = binarr_read_i32_n(barr);
        break;
    default:
        return NULL;
    }
    return packet;
}

int server_send_packet_serialized(server_t *server, client_t client, struct binarr barr) {
    if (server_send(server, client, barr) == -1) {
        return -1;
    }
    return 0;
}

int server_send_packet(server_t *server, client_t target, struct packet packet) {
    struct binarr barr = {0};
    uint64_t capacity = MAX_PACKET_SIZE;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        ERROR("unknown packet type");
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
    uint64_t capacity = MAX_PACKET_SIZE;
    binarr_new(&barr, capacity);
    if (packet_serialize(&barr, packet) == NULL) {
        ERROR("unknown packet type");
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
    uint64_t capacity = MAX_PACKET_SIZE;
    binarr_new(&barr, capacity);

    if (packet_serialize(&barr, packet) == NULL) {
        ERROR("unknown packet type");
        binarr_destroy(barr);
        return -1;
    }
    // TODO: write proper shit for client
    int b_sent = send(server.fd, barr.buf, barr.size, 0);
    if (b_sent == -1) {
        ERRORF("client_send: %d %s", server.fd, strerror(errno));
    };
    binarr_destroy(barr);

    return b_sent;
}

int recv_packet(int fd, struct packet *result) {
    struct binarr barr = {0};
    size_t capacity = MAX_PACKET_SIZE;
    binarr_new(&barr, capacity);
    ssize_t h_size = recv(fd, barr.buf, PACKET_HEADER_SIZE, 0);
    barr.size += h_size;
    if (h_size <= 0) {
        perror("header recv");
        return h_size;
    }
    binarr_read_i8(&barr);
    int32_t packet_size = binarr_read_i32_n(&barr);
    ssize_t d_size = recv(fd, barr.buf + barr.size, packet_size, 0);
    barr.index = 0;
    if (d_size <= 0) {
        perror("data recv");
        return d_size;
    }

    packet_deserialize(result, &barr);
    binarr_destroy(barr);
    return h_size + d_size;
}


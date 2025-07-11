#include "krft/server.h"
#include "cpong_logic.h"
#include "krft/matchmaking.h"
#include "cpong_packets.h"
#include "krft/ts_queue.h"
#include "krft/log.h"
#include "krft/run_every.h"
#include "utility.h"
#include <pthread.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>

#define TICK_CAP 40
#define MIN_TICK_DURATION_MS 1000 / TICK_CAP

struct server_state {
    struct pong_state state;
    int8_t sync;
    pthread_mutex_t sync_mtx;
    struct input input1;
    struct input input2;
    pthread_mutex_t input_mtx;
};

static struct pong_state state;

static int8_t sync = 0;
pthread_mutex_t sync_mtx = PTHREAD_MUTEX_INITIALIZER;

static struct input input1 = {0};
static struct input input2 = {0};
pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;

static void on_disband(struct room *room) {
    LOGF("disbanded room %d", room->id);
}

static void start_pong_game(struct room *room);

static bool send_state(int delta_time, void *room_p);

static void *input_receive(void *room_p);

int main(int argc, char **argv) {
    if (argc != 2) {
        ERRORF("Usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *port = argv[1];
    server_t server = {0};
    server_set_fd(&server, port);
    server.clients = ts_queue_new();
    LOGF("Listening on %s", port);

    matchmaking_server_worker(&server, NULL, start_pong_game, on_disband);

    return 0;
}

static bool send_state(int delta_time, void *room_p) {
    // LOG("send_state start");
    if (delta_time > 3 + MIN_TICK_DURATION_MS) {
        WARNF("Server running slow: %d ms behind", 
              delta_time - MIN_TICK_DURATION_MS);
    }
    struct room *room = room_p;
    server_t *server = room->clients[0]->server;

    pthread_mutex_lock(&input_mtx);
    state.player1.velocity.y = state.player1.speed;
    state.player1.velocity.y *= get_input_acc_direction(input1.input_acc_ms);
    struct game_obj player1_upd = linear_move(state.player1, abs(input1.input_acc_ms));
    state.player2.velocity.y = state.player2.speed;
    state.player2.velocity.y *= get_input_acc_direction(input2.input_acc_ms);
    struct game_obj player2_upd = linear_move(state.player2, abs(input2.input_acc_ms));
    input1.input_acc_ms = 0;
    input2.input_acc_ms = 0;
    pthread_mutex_unlock(&input_mtx);

    struct wall wall = {.up = state.box_size.y, .down = 0, .left = 0, .right = state.box_size.x};
    struct game_obj ball_upd = {0};
    int scored_index;
    ball_advance(wall, state.player1, player1_upd, state.player2, player2_upd, state.ball, &ball_upd, delta_time, &scored_index);

    state.ball = ball_upd;
    state.player1 = player1_upd;
    state.player2 = player2_upd;

    enum cpong_packet packet_type = PACKET_STATE;
    if (scored_index != -1) {
        init_game(&state);
        state.score[scored_index]++;
        pthread_mutex_lock(&sync_mtx);
        LOGF("sync change: %d -> %d", sync, sync + 1);
        sync++;
        pthread_mutex_unlock(&sync_mtx);
        packet_type = PACKET_INIT;
    }

    pthread_mutex_lock(&sync_mtx);
    struct packet packet = {
        .type = packet_type,
        .sync = sync,
        .data.state = state
    };
    if (packet.type == PACKET_INIT) {
        LOGF("sync %d", packet.sync);
    }
    pthread_mutex_unlock(&sync_mtx);

    if (room_broadcast(server, room, packet) == -1) {
        LOG("Someone disconnected");
        return false;
    };

    // LOG("pong: Broadcasting state");
    // print_state(packet.data.state);

    return !room->is_disbanded;
}

static void on_input_ready(client_t client) {
    struct packet packet;
    int res = recv_packet(client.fd, &packet);
    if (res == 0) {
        LOG("Someone disconnected");
        return;
    } else if (res == -1) {
        ERROR("recv_packet returned -1");
    }

    pthread_mutex_lock(&sync_mtx);
    if (packet.sync != sync) {
        LOGF("invalid sync: got %d, current %d", packet.sync, sync);
        pthread_mutex_unlock(&sync_mtx);
        return;
    }
    pthread_mutex_unlock(&sync_mtx);

    if (packet.type != PACKET_INPUT) {
        WARNF("Expected PACKET_INPUT, received %d", packet.type);
        return;
    }

    pthread_mutex_lock(&input_mtx);
    if (state.player_ids[0] == client.id) {
        input1.input_acc_ms += packet.data.input.input_acc_ms;
        // LOGF("p1: %d", input1.input_acc_ms);
    } else if (state.player_ids[1] == client.id) {
        input2.input_acc_ms += packet.data.input.input_acc_ms;
        // LOGF("p2: %d", input2.input_acc_ms);
    }
    pthread_mutex_unlock(&input_mtx);

}

static void *input_receive(void *room_p) {
    struct room *room = room_p;
    struct pollfd fds[ROOM_SIZE] = {0};
    for (int i = 0; i < ROOM_SIZE; i++) {
        fds[i].fd = room->clients[i]->fd;
        fds[i].events = POLLIN;
    }
    while (!room->is_disbanded) {
        poll(fds, ROOM_SIZE, -1);
        for (int i = 0; i < ROOM_SIZE; i++) {
            if (!(fds[i].revents & POLLIN))
                continue;
            on_input_ready(*room->clients[i]);
        }
    }
    return NULL;
}

static void start_pong_game(struct room *room) {
    LOG("Starting pong");
    server_t *server = room->clients[0]->server;

    state.player_ids[0] = room->clients[0]->id;
    state.player_ids[1] = room->clients[1]->id;
    state.score[0] = 0;
    state.score[1] = 0;
    init_game(&state);

    struct packet init_packet = {
        .type = PACKET_INIT,
        .sync = sync,
        .data.state = state
    };
    for (int i = 0; i < ROOM_SIZE; i++) {
        init_packet.data.state.own_id_index = i;
        if (server_send_packet(server, *room->clients[i], init_packet) == -1) {
            ERRORF("Failed to send init packet to %d", i);
            return;
        }
    }
    LOG("broadcasted init packet");
    print_state(init_packet.data.state);

    struct run_every_args state_sender_re_args = {
        .func = send_state,
        .args = room,
        .interval_ms = MIN_TICK_DURATION_MS
    };
    pthread_t state_sender = start_run_every_thread(&state_sender_re_args);

    pthread_t input_receiver;
    pthread_create(&input_receiver, NULL, input_receive, room);
    pthread_detach(input_receiver);

    pthread_join(state_sender, NULL);
}


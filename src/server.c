#include "server.h"
#include "cpong_logic.h"
#include "matchmaking.h"
#include "cpong_packets.h"
#include "ts_queue.h"
#include "log.h"
#include "run_every.h"
#include <pthread.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>

#define TICK_CAP 10
#define MIN_TICK_DURATION_MS 1000 / TICK_CAP

static struct pong_state state;
static struct input input1 = {0};
static struct input input2 = {0};
pthread_mutex_t input_mtx = PTHREAD_MUTEX_INITIALIZER;

void start_pong_game(struct room *room);

void on_disband(struct room *room) {
    LOGF("disbanded room %d", room->id);
}

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
}

bool send_state(int delta_time, void *room_p) {
    if (delta_time > 3 + MIN_TICK_DURATION_MS) {
        WARNF("Server running slow: %d ms behind", 
              delta_time - MIN_TICK_DURATION_MS);
    }
    struct room *room = room_p;
    server_t *server = room->clients[0]->server;

    pthread_mutex_lock(&input_mtx);
    struct game_obj player1_upd = linear_move(state.player1, input1.input_acc_ms);
    struct game_obj player2_upd = linear_move(state.player2, input2.input_acc_ms);
    input1.input_acc_ms = 0;
    input2.input_acc_ms = 0;
    pthread_mutex_unlock(&input_mtx);

    struct game_obj ball_upd = linear_move(state.ball, delta_time);
    bool coll1 = ball_paddle_collide(state.player1, state.ball, player1_upd, &ball_upd, delta_time);
    bool coll2 = ball_paddle_collide(state.player2, state.ball, player2_upd, &ball_upd, delta_time);
    struct wall wall = {.up = state.box_size.y, .down = 0, .left = 0, .right = state.box_size.x};
    bool coll_wall = ball_wall_collision(wall, state.ball, &ball_upd, delta_time);
    if (coll1 || coll2 || coll_wall)
        LOGF("collisions: %d %d %d", coll1, coll2, coll_wall);
    state.ball = ball_upd;
    state.player1 = player1_upd;
    state.player2 = player2_upd;

    struct packet packet = {
        .type = PACKET_STATE,
        .data.state = state
    };

    if (mm_room_broadcast(server, room, packet) == -1) {
        LOG("Someone disconnected");
        return false;
    };

    LOG("pong: Broadcasting state");
    print_state(packet.data.state);

    return !room->is_disbanded;
}

void *input_receive(void *room_p) {
    struct room *room = room_p;
    struct pollfd fds[ROOM_SIZE] = {0};
    for (int i = 0; i < ROOM_SIZE; i++) {
        fds[i].fd = room->clients[i]->fd;
        fds[i].events = POLLIN;
    }
    while (!room->is_disbanded) {
        poll(fds, ROOM_SIZE, -1);
        for (int i = 0; i < ROOM_SIZE; i++) {
            if (fds[i].revents & POLLIN) {
                struct packet packet;
                int res = recv_packet(fds[i].fd, &packet);
                if (res == 0) {
                    LOG("Someone disconnected");
                    return NULL;
                } else if (res == -1) {
                    ERROR("recv_packet returned -1");
                }
                if (packet.type == PACKET_INPUT) {
                    int id = room->clients[i]->id;
                    pthread_mutex_lock(&input_mtx);
                    if (state.player_ids[0] == id) {
                        input1.input_acc_ms += packet.data.input.input_acc_ms;
                        LOGF("p1: %d", input1.input_acc_ms);
                    } else if (state.player_ids[1] == id) {
                        input2.input_acc_ms += packet.data.input.input_acc_ms;
                        LOGF("p2: %d", input2.input_acc_ms);
                    }
                    pthread_mutex_unlock(&input_mtx);
                } else {
                    WARN("Unknown packet");
                }
            }
        }
    }
    return NULL;
}

void start_pong_game(struct room *room) {
    LOG("Starting pong");
    server_t *server = room->clients[0]->server;

    state.player_ids[0] = room->clients[0]->id;
    state.player_ids[1] = room->clients[1]->id;
    init_game(&state);

    struct packet init_packet = {
        .type = PACKET_INIT,
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


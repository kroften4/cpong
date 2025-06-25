#include "server.h"
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

static struct state state = {0};
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

void print_state(struct state state) {
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player1.id, state.player1.y,
           state.player2.id, state.player2.y,
           state.ball.x, state.ball.y);
}

void *send_state(void *room_p) {
    struct room *room = room_p;
    server_t *server = room->clients[0]->server;
    while (!room->is_disbanded) {
        float tick_start = get_curr_time_ms();

        int speed = 1;
        pthread_mutex_lock(&input_mtx);
        state.player1.y -= speed * input1.input_acc_ms;
        state.player2.y -= speed * input2.input_acc_ms;
        input1.input_acc_ms = 0;
        input2.input_acc_ms = 0;
        pthread_mutex_unlock(&input_mtx);

        struct packet packet = {
            .type = PACKET_STATE,
            .size = 4 * 3 * 3,
            .data = {.state = {
                .player1 = {.id = room->clients[0]->id, .y = state.player1.y},
                .player2 = {.id = room->clients[1]->id, .y = state.player2.y},
                .ball = {.x = rand() % 100, .y = rand() % 100}
            }}
        };

        if (mm_room_broadcast(server, room, packet) == -1) {
            LOG("pong: Someone disconnected");
            break;
        };

        LOG("pong: Broadcasting state");
        print_state(packet.data.state);

        int32_t tick_end = get_curr_time_ms();
        int32_t tick_duration = tick_end - tick_start;
        int32_t sleep_time_ms = MIN_TICK_DURATION_MS - tick_duration;
        if (sleep_time_ms > 0) {
            struct timespec rt = {
                .tv_sec = sleep_time_ms / 1000, 
                .tv_nsec = sleep_time_ms % 1000 * 1000000
            };
            nanosleep(&rt, NULL);
        }
    }
    return NULL;
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
                    if (state.player1.id == id) {
                        input1.input_acc_ms += packet.data.input.input_acc_ms;
                        LOGF("p1: %d", input1.input_acc_ms);
                    } else if (state.player2.id == id) {
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
    state.player1.id = room->clients[0]->id;
    state.player2.id = room->clients[1]->id;

    struct packet start_packet = {
        .type = PACKET_PING,
        .size = 1,
        .data = {.ping = {
            .dummy = 's'
        }}
    };
    if (mm_room_broadcast(room->clients[0]->server, room, start_packet) == -1) {
        ERROR("can't broadcast ping packet");
        return;
    }
    LOG("broadcasted ping packet");
    pthread_t state_sender;
    pthread_create(&state_sender, NULL, send_state, room);
    pthread_detach(state_sender);

    pthread_t input_receiver;
    pthread_create(&input_receiver, NULL, input_receive, room);
    pthread_detach(input_receiver);
}


#include "server.h"
#include "matchmaking.h"
#include "cpong_packets.h"
#include "ts_queue.h"
#include "log.h"
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>

#define TICK_CAP 10
#define MIN_TICK_DURATION 1.0f/10

struct state state = {0};
struct input input1 = {0, 0};
struct input input2 = {0, 0};

void ping(client_t client);
void pong(client_t *room[ROOM_SIZE]);

void send_q_cond(client_t client);

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

    start_matchmaking_worker(&server, pong);
    server_worker(&server, send_q_cond);
}

void send_q_cond(client_t client) {
    (void)client;
    pthread_cond_signal(&mm_q_has_match);
}

void print_state(struct state state) {
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player1.id,
           state.player1.y,
           state.player2.id,
           state.player2.y,
           state.ball.x,
           state.ball.y);
}

float get_curr_time(void) {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}

void *send_state(void *room_p) {
    client_t **room = room_p;
    server_t *server = room[0]->server;
    for (;;) {
        float tick_start = get_curr_time();

        int speed = 5;
        state.player1.y -= (input1.up - input1.down) * speed;
        state.player2.y -= (input2.up - input2.down) * speed;

        struct packet packet = {
            .type = PACKET_STATE,
            .size = 4 * 2 * 3,
            .data = {.state = {
                .player1 = {.id = room[0]->id, .y = state.player1.y},
                .player2 = {.id = room[1]->id, .y = state.player2.y},
                .ball = {.x = rand() % 100, .y = rand() % 100}
            }}
        };
        if (mm_room_broadcast(server, room, packet) == -1) {
            LOG("pong: Someone disconnected");
            return NULL;
        };
        LOG("pong: Broadcasting state");
        print_state(packet.data.state);

        float tick_end = get_curr_time();
        float tick_duration = tick_end - tick_start;
        float sleep_time = MIN_TICK_DURATION - tick_duration;
        if (sleep_time > 0) {
            struct timespec rt = {.tv_sec = 0, .tv_nsec = sleep_time * 1000000000};
            nanosleep(&rt, NULL);
        }
    }
}

void *input_receive(void *room_p) {
    client_t **room = room_p;
    struct pollfd fds[ROOM_SIZE] = {0};
    for (int i = 0; i < ROOM_SIZE; i++) {
        fds[i].fd = room[i]->fd;
        fds[i].events = POLLIN;
    }
    for (;;) {
        poll(fds, ROOM_SIZE, -1);
        for (int i = 0; i < ROOM_SIZE; i++) {
            if (fds[i].revents & POLLIN) {
                struct packet packet;
                recv_packet(fds[i].fd, &packet);
                if (packet.type == PACKET_INPUT) {
                    int id = room[i]->id;
                    if (state.player1.id == id) {
                        input1.up = packet.data.input.up;
                        input1.down = packet.data.input.down;
                        LOGF("p1: %d %d", input1.up, input1.down);
                    } else if (state.player2.id == id) {
                        input2.up = packet.data.input.up;
                        input2.down = packet.data.input.down;
                        LOGF("p2: %d %d", input2.up, input2.down);
                    }
                } else {
                    // LOG("Unknown packet");
                }
            }
        }
    }
    return NULL;
}

void pong(client_t *room[ROOM_SIZE]) {
    state.player1.id = room[0]->id;
    state.player2.id = room[1]->id;

    struct packet start_packet = {
        .type = PACKET_PING,
        .size = 1,
        .data = {.ping = {
            .dummy = 's'
        }}
    };
    if (mm_room_broadcast(room[0]->server, room, start_packet) == -1) {
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
            LOGF("Connection %d closed", client.id);
            return;
        };
        // LOGF("Sent ping to %d\n", client.id);
    }
}


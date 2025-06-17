#include "server.h"
#include "matchmaking.h"
#include "cpong_packets.h"
#include "ts_queue.h"
#include <pthread.h>
#include <stdio.h>
#include <poll.h>

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
        fprintf(stderr, "Usage: %s <port>", argv[0]);
    }
    char *port = argv[1];
    server_t server = {0};
    server_set_fd(&server, port);
    server.clients = ts_queue_new();
    printf("Listening on %s\n", port);

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

        state.player1.y += input1.left - input1.right;
        state.player2.y += input2.left - input2.right;

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
            printf("pong: Someone disconnected\n");
            return NULL;
        };
        printf("pong: Broadcasting state: ");
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
                        input1.left = packet.data.input.left;
                        input1.right = packet.data.input.right;
                        printf("input_receive: p1: %d %d\n", input1.left, input1.right);
                    } else if (state.player2.id == id) {
                        input2.left = packet.data.input.left;
                        input2.right = packet.data.input.right;
                        printf("input_receive: p2: %d %d\n", input2.left, input2.right);
                    }
                } else {
                    // puts("input_receive: unknown packet");
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
        fprintf(stderr, "pong: can't broadcast ping packet\n");
        return;
    }
    puts("pong: broadcast ping packet");
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
            printf("ping: Connection %d closed\n", client.id);
            return;
        };
        // printf("Sent ping to %d\n", client.id);
    }
}


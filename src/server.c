#include "server.h"
#include "matchmaking.h"
#include "cpong_packets.h"
#include "ts_queue.h"
#include <pthread.h>
#include <stdio.h>

#define TICK_CAP 10
#define MIN_TICK_DURATION 1.0f/10

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

void pong(client_t *room[ROOM_SIZE]) {
    server_t *server = room[0]->server;
    for (;;) {
        float tick_start = get_curr_time();

        struct packet packet = {
            .type = PACKET_STATE,
            .size = 4 * 2 * 3,
            .data = {.state = {
                .player1 = {.id = room[0]->id, .y = rand() % 100},
                .player2 = {.id = room[1]->id, .y = rand() % 100},
                .ball = {.x = rand() % 100, .y = rand() % 100}
            }}
        };
        if (mm_room_broadcast(server, room, packet) == -1) {
            printf("pong: Someone disconnected\n");
            return;
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


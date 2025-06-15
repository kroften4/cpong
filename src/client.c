#include <bits/time.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "bin_array.h"
#include "cpong_packets.h"
#include "server.h"

#define MAX_PACKET_SIZE 100
#define FPS_CAP 10
#define MIN_FRAME_DURATION 1.0f / FPS_CAP

#define PACKET_HEADER_SIZE 5

int connect_to_server(char *name, char *port);

struct state state = {0};
pthread_mutex_t state_mtx;

void print_state(struct state state) {
    pthread_mutex_lock(&state_mtx);
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player1.id,
           state.player1.y,
           state.player2.id,
           state.player2.y,
           state.ball.x,
           state.ball.y);
    pthread_mutex_unlock(&state_mtx);
}

int state_server_sync(server_t server) {
    struct binarr barr = {0};
    size_t capacity = MAX_PACKET_SIZE;
    binarr_new(&barr, capacity);
    ssize_t h_size = recv(server.fd, barr.buf, PACKET_HEADER_SIZE, 0);
    if (h_size == -1) {
        return -1;
    }
    binarr_read_i8(&barr);
    int32_t packet_size = binarr_read_i32_n(&barr);
    ssize_t d_size = recv(server.fd, barr.buf + PACKET_HEADER_SIZE, packet_size, 0);
    barr.index = 0;
    if (d_size == -1) {
        return -1;
    }

    struct packet packet;
    packet_deserialize(&packet, &barr);
    if (packet.type == PACKET_STATE) {
        pthread_mutex_lock(&state_mtx);
        state = packet.data.state;
        pthread_mutex_unlock(&state_mtx);
    } else {
        return -1;
    }
    return 0;
}
void *state_sync_worker(void *server_p) {
    server_t *server = server_p;
    for (;;) {
        state_server_sync(*server);
    }
    return NULL;
}

float get_curr_time(void) {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server ip/name> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    server_t server = {.fd = connect_to_server(argv[1], argv[2])};
    printf("Connected to server\n");

    pthread_t state_syncer;
    pthread_create(&state_syncer, NULL, state_sync_worker, &server);
    pthread_detach(state_syncer);

    // float last_frame = get_curr_time();
    for (;;) {
        float frame_start = get_curr_time();
        // float delta_time = last_frame - frame_start;

        print_state(state);

        float frame_end = get_curr_time();
        float frame_duration = frame_end - frame_start;
        float sleep_time = MIN_FRAME_DURATION - frame_duration;
        if (sleep_time > 0) {
            struct timespec rt = {.tv_sec = 0, .tv_nsec = sleep_time * 1000000000};
            nanosleep(&rt, NULL);
        }
    }

    // struct binarr barr = {0};
    // size_t capacity = packet.size;
    // binarr_new(&barr, capacity);
    // recv(server.fd, barr.buf, capacity, 0);
    // packet_deserialize(&packet, &barr);
    //
    // switch (packet.type) {
    // case PACKET_PING:
    //     printf("recieved PACKET_PING: type %d; size %u; dummy char %c\n",
    //            packet.type, packet.size, packet.data.ping.dummy);
    //     break;
    // case PACKET_STATE:
    //     printf("recieved PACKET_STATE: type %d; size %u; \n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
    //            packet.type, packet.size,
    //            packet.data.state.player1.id,
    //            packet.data.state.player1.y,
    //            packet.data.state.player2.id,
    //            packet.data.state.player2.y,
    //            packet.data.state.ball.x,
    //            packet.data.state.ball.y);
    //     break;
    // default:
    //     printf("recieved unknown packet: %d\n", packet.type);
    //     break;
    // }

    return 0;
}

int connect_to_server(char *name, char *port) {
    int status;
    int serverfd;
    struct addrinfo hints;
    struct addrinfo *res;

    // get addrinfo for remote
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(name, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    // select the appropriate addrinfo linked list member
    if (res == NULL) {
        perror("couldn't find host addrinfo (list is empty)");
        return -1;
    }
    struct addrinfo server_ai = *res;
    freeaddrinfo(res);

    // make a socket and connect
    if ((serverfd = socket(server_ai.ai_family, server_ai.ai_socktype, server_ai.ai_protocol)) == -1) {
        perror("failed to create a socket");
        return -1;
    }

    if (connect(serverfd, server_ai.ai_addr, server_ai.ai_addrlen) == -1) {
        perror("failed to connect");
        return -1;
    }
    printf("connected to server\n");
    return serverfd;
}

#include <SDL3/SDL.h>
#include <stdbool.h>
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
#include "log.h"

#define FPS_CAP 10
#define MIN_FRAME_DURATION 1.0f / FPS_CAP

int connect_to_server(char *name, char *port);

struct state state = {0};
pthread_mutex_t state_mtx;
server_t server;

void print_state(struct state state) {
    pthread_mutex_lock(&state_mtx);
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player1.id, state.player1.y,
           state.player2.id, state.player2.y,
           state.ball.x, state.ball.y);
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

struct input input;

void draw_server_state(struct state state, SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 50);
    SDL_FRect paddle1 = {0, state.player1.y, 10, 50};
    SDL_RenderFillRect(renderer, &paddle1);
    SDL_FRect paddle2 = {100, state.player2.y, 10, 50};
    SDL_RenderFillRect(renderer, &paddle2);
    SDL_FRect ball = {state.ball.x, state.ball.y, 10, 10};
    SDL_RenderFillRect(renderer, &ball);
    SDL_RenderPresent(renderer);
}

void clear_screen(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
}

bool update(SDL_Event *event, SDL_Renderer *renderer) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    const bool *keyboard = SDL_GetKeyboardState(NULL);
    input.up = keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W];
    input.down = keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S];

    clear_screen(renderer);
    draw_server_state(state, renderer);

    return true;
}

void fixed_update(void) {
    struct packet packet = {
        .type = PACKET_INPUT,
        .size = sizeof(input.up) * 2,
        .data = {.input = {
            .up = input.up,
            .down = input.down
        }}
    };
    int b_sent = client_send(server, packet);
    LOGF("sent %d, inputs %d %d", b_sent, input.up, input.down);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        ERRORF("Usage: %s <server ip/name> <port>", argv[0]);
        return EXIT_FAILURE;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer("Nukers", 640, 480, 0, &window, &renderer);
    SDL_Event event;

    server.fd = connect_to_server(argv[1], argv[2]);

    struct packet start_packet = {0};
    recv_packet(server.fd, &start_packet);
    if (start_packet.type != PACKET_PING) {
        ERRORF("did not receive ping packet: %d", start_packet.type);
        return 1;
    } else {
        LOGF("received ping packet: %c", start_packet.data.ping.dummy);
    }

    pthread_t state_syncer;
    pthread_create(&state_syncer, NULL, state_sync_worker, &server);
    pthread_detach(state_syncer);


    // float last_frame = get_curr_time();
    bool quit = false;
    while (!quit) {
        float frame_start = get_curr_time();
        // float delta_time = last_frame - frame_start;

        if (!update(&event, renderer)) {
            quit = true;
        }

        fixed_update();

        float frame_end = get_curr_time();
        float frame_duration = frame_end - frame_start;
        float sleep_time = MIN_FRAME_DURATION - frame_duration;
        if (sleep_time > 0) {
            struct timespec rt = {.tv_sec = 0, .tv_nsec = sleep_time * 1000000000};
            nanosleep(&rt, NULL);
        }
    }

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
        ERRORF("getaddrinfo: %s", gai_strerror(status));
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
    LOGF("connected to server: %d", serverfd);
    return serverfd;
}

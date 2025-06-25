#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpong_packets.h"
#include "server.h"
#include "log.h"
#include "run_every.h"

#define FPS_CAP 1000
#define MIN_FRAME_DURATION_MS 1000 / FPS_CAP

#define NET_TPS_CAP 10
#define MIN_NET_TICK_DURATION_MS 1000 / NET_TPS_CAP

struct state state = {0};
pthread_mutex_t state_mtx = PTHREAD_MUTEX_INITIALIZER;
server_t server;

static int32_t input_accumulator = 0;
pthread_mutex_t input_acc_mtx = PTHREAD_MUTEX_INITIALIZER;

void print_state(struct state state) {
    pthread_mutex_lock(&state_mtx);
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player1.id, state.player1.y,
           state.player2.id, state.player2.y,
           state.ball.x, state.ball.y);
    pthread_mutex_unlock(&state_mtx);
}

int server_state_receive(server_t server) {
    struct packet packet = {0};
    int res = recv_packet(server.fd, &packet);
    if (res <= 0) {
        LOG("Couldn't recv from the server");
        return -1;
    }
    if (packet.type == PACKET_STATE) {
        pthread_mutex_lock(&state_mtx);
        state = packet.data.state;
        pthread_mutex_unlock(&state_mtx);
    } else {
        WARNF("Unexpected packet: %d", packet.type);
        return -1;
    }
    return 0;
}

void *server_state_receiver(void *server_p) {
    server_t *server = server_p;
    for (;;) {
        server_state_receive(*server);
    }
    return NULL;
}

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

struct update_args {
    SDL_Event *event;
    SDL_Renderer *renderer;
};

bool update(int delta_time, void *update_args);

bool network_update(int delta_time, void *dummy) {
    (void)dummy;
    if (delta_time > 5 + MIN_NET_TICK_DURATION_MS) {
        WARNF("Network updates running slow: %d ms behind", 
              delta_time - MIN_NET_TICK_DURATION_MS);
    }

    pthread_mutex_lock(&input_acc_mtx);
    struct packet packet = {
        .type = PACKET_INPUT,
        .size = sizeof(input_accumulator),
        .data = {.input = {
            .input_acc_ms = input_accumulator
        }}
    };
    input_accumulator = 0;
    pthread_mutex_unlock(&input_acc_mtx);
    int b_sent = client_send(server, packet);
    LOGF("sent %d, input %d", b_sent, packet.data.input.input_acc_ms);
    return true;
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

    pthread_t server_state_recv_thread;
    pthread_create(&server_state_recv_thread, NULL, server_state_receiver, &server);
    pthread_detach(server_state_recv_thread);

    struct run_every_args net_re_args = {
        .func = network_update,
        .args = NULL,
        .interval_ms = MIN_NET_TICK_DURATION_MS
    };
    pthread_t network_thread = start_run_every_thread(&net_re_args);
    pthread_detach(network_thread);

    struct update_args update_args = {
        .event = &event,
        .renderer = renderer
    };
    struct run_every_args update_re_args = {
        .func = update,
        .args = &update_args,
        .interval_ms = MIN_FRAME_DURATION_MS
    };
    run_every(update_re_args);

    return 0;
}

bool update(int delta_time, void *update_args_p) {
    struct update_args *update_args = update_args_p;
    SDL_Event *event = update_args->event;
    SDL_Renderer *renderer = update_args->renderer;
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    const bool *keyboard = SDL_GetKeyboardState(NULL);
    bool up = keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W];
    bool down = keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S];

    pthread_mutex_lock(&input_acc_mtx);
    input_accumulator += (up - down) * delta_time;
    pthread_mutex_unlock(&input_acc_mtx);

    clear_screen(renderer);
    draw_server_state(state, renderer);

    return true;
}


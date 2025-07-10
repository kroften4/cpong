#include "krft/bin_array.h"
#include "krft/server.h"
#include "krft/ts_queue.h"
#include "krft/matchmaking.h"
#include "krft/log.h"
#include <pthread.h>

struct mm_worker_args {
    server_t *server;
    on_connection_t on_connection_fn;
    on_match_t on_match_fn;
    on_disband_t on_disband_fn;
};

typedef struct {
    struct mm_worker_args args;
    struct ts_queue *rooms;
} mm_server_t;

static mm_server_t mm_server;

pthread_cond_t mm_q_has_match = PTHREAD_COND_INITIALIZER;

void mm_on_connection(client_t client);

int matchmaking_server_worker(server_t *server,
                              on_connection_t on_connection_fn,
                              on_match_t on_match_fn,
                              on_disband_t on_disband_fn) {
    struct mm_worker_args *mm_args = malloc(sizeof(struct mm_worker_args));
    mm_args->server = server;
    mm_args->on_connection_fn = on_connection_fn;
    mm_args->on_match_fn = on_match_fn;
    mm_args->on_disband_fn = on_disband_fn;

    mm_server.rooms = ts_queue_new();
    mm_server.args = *mm_args;

    pthread_t matcher;
    pthread_create(&matcher, NULL, matchmaking_worker, mm_args);
    pthread_detach(matcher);
    server_worker(server, mm_on_connection);
    return 0;
}

void *mm_handle_match(void *room_data_p) {
    struct room *room = room_data_p;
    if (mm_server.args.on_match_fn != NULL) {
        LOG("calling on_match_fn");
        mm_server.args.on_match_fn(room);
    } else {
        LOG("null");
    }
    return NULL;
}

void *matchmaking_worker(void *mm_worker_args_p) {
    struct mm_worker_args *startup_args = mm_worker_args_p;
    struct ts_queue *clients_q = startup_args->server->clients;
    while (1) {
        pthread_mutex_lock(&clients_q->mutex);
        while (clients_q->size < ROOM_SIZE) {
            pthread_cond_wait(&mm_q_has_match, &clients_q->mutex);
        }

        while (clients_q->size >= ROOM_SIZE) {
            // if enough players in q to create a room, do so

            // create room
            struct room *room = malloc(sizeof(struct room));
            if (room == NULL) {
                perror("matchmake: malloc");
                return NULL;
            }

            // fill it up
            for (int i = 0; i < ROOM_SIZE; i++) {
                room->clients[i] = clients_q->head->data;
                __ts_queue_dequeue_nolock(clients_q);
            }
            room->id = room->clients[0]->id;
            room->is_disbanded = false;
            LOG("matchmake: Created a room");
            __print_queue(clients_q);

            // add to room list
            __ts_queue_enqueue_nolock(mm_server.rooms, room);

            pthread_t game_thread;
            pthread_create(&game_thread, NULL, mm_handle_match, room);
            pthread_detach(game_thread);
        }

        pthread_mutex_unlock(&clients_q->mutex);
    }
}

void mm_on_connection(client_t client) {
    pthread_cond_signal(&mm_q_has_match);
    if (mm_server.args.on_connection_fn != NULL)
        mm_server.args.on_connection_fn(client);
}

void handle_disband(struct room *room) {
    room->is_disbanded = true;
    if (mm_server.args.on_disband_fn != NULL)
        mm_server.args.on_disband_fn(room);
}


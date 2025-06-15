#include "bin_array.h"
#include "cpong_packets.h"
#include "server.h"
#include "ts_queue.h"
#include "matchmaking.h"
#include <pthread.h>

static struct ts_queue *rooms;

pthread_cond_t mm_q_has_match = PTHREAD_COND_INITIALIZER;

int start_matchmaking_worker(server_t *server, on_match_fn_t on_match_fn) {
    struct mm_args *mm_args = malloc(sizeof(struct mm_args));
    mm_args->on_match_fn = on_match_fn;
    mm_args->server = server;

    rooms = ts_queue_new();

    pthread_t matcher;
    pthread_create(&matcher, NULL, matchmaking_worker, mm_args);
    pthread_detach(matcher);
    return 0;
}


void *__handle_match(void *match_data_p) {
    struct room_data *match_data = match_data_p;
    match_data->on_match_fn(match_data->room);
    return NULL;
}
void *matchmaking_worker(void *matchmake_args_p) {
    struct mm_args *mm_args = matchmake_args_p;
    struct ts_queue *clients_q = mm_args->server->clients;
    while (1) {
        pthread_mutex_lock(&clients_q->mutex);
        while (clients_q->size < ROOM_SIZE) {
            pthread_cond_wait(&mm_q_has_match, &clients_q->mutex);
        }

        while (clients_q->size >= ROOM_SIZE) {
            // if enough players in q to create a room, do so

            // create room
            struct room_data *match_data = malloc(sizeof(struct room_data));
            if (match_data == NULL) {
                perror("matchmake: malloc");
                return NULL;
            }
            match_data->on_match_fn = mm_args->on_match_fn;

            // fill it up
            for (int i = 0; i < ROOM_SIZE; i++) {
                match_data->room[i] = clients_q->head->data;
                __ts_queue_dequeue_nolock(clients_q);
            }
            printf("matchmake: Created a room ");
            __print_queue(clients_q);

            // add to room list
            __ts_queue_enqueue_nolock(rooms, match_data);

            pthread_t game_thread;
            pthread_create(&game_thread, NULL, __handle_match,
                           match_data);
            pthread_detach(game_thread);
        }

        pthread_mutex_unlock(&clients_q->mutex);
    }
}

int mm_room_broadcast(server_t *server, client_t *room[ROOM_SIZE], struct packet packet) {
    struct binarr *barr = malloc(sizeof(struct binarr));
    binarr_new(barr, packet.size);
    packet_serialize(barr, packet);
    for (int i = 0; i < ROOM_SIZE; i++) {
        // TODO: handle disconnection
        if (server_send_packet_serialized(server, *room[i], *barr)) {
            return -1;
        };
    }
    binarr_destroy(*barr);
    return 0;
}


// TODO: make on_queue function

#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ts_queue.h"
#include "server.h"
#include "matchmaking_server.h"

struct ts_queue *player_q;
pthread_cond_t q_has_match = PTHREAD_COND_INITIALIZER;

struct ts_queue *matches;

int matchmaking_server(char *port, on_match_fn_t on_match) {
    struct matchmake_args *mm_args = malloc(sizeof(struct matchmake_args));
    mm_args->on_match_fn = on_match;
    player_q = ts_queue_new();
    mm_args->queue = player_q;

    matches = ts_queue_new();

    // player queue consumer
    pthread_t matcher;
    pthread_create(&matcher, NULL, __matchmake, mm_args);

    server(port, __enqueue_new_player);
    return 0;
}

void __enqueue_new_player(int connfd) {
    /*
    * Handle client communication during the game
    */
    struct client_data *cl_data = malloc(sizeof(struct client_data));
    if (cl_data == NULL) {
        perror("enqueue_new_player: malloc");
        return;
    }
    cl_data->connfd = connfd;
    ts_queue_enqueue(player_q, cl_data);
    printf("enqueue_new_player: Put %d in the queue ", connfd);
    __print_queue(player_q);
    pthread_cond_signal(&q_has_match);
}

void __print_queue(struct ts_queue *q) {
    printf("[ ");
    for (struct ts_queue_node *node = q->head; node != NULL;
         node = node->next) {
        struct client_data *data = node->data;
        printf("%d ", data->connfd);
    }
    printf("]\n");
}

void *__matchmake(void *matchmake_args_p) {
    struct matchmake_args *mm_args = matchmake_args_p;
    struct ts_queue *q = mm_args->queue;
    while (1) {
        pthread_mutex_lock(&q->mutex);
        while (q->size < ROOM_SIZE) {
            pthread_cond_wait(&q_has_match, &q->mutex);
        }

        while (q->size >= ROOM_SIZE) {
            // if enough players in q to create a room, do so

            // create room
            struct match_data *match_data = malloc(sizeof(struct match_data));
            if (match_data == NULL) {
                perror("matchmake: malloc");
                return NULL;
            }
            match_data->on_match_fn = mm_args->on_match_fn;

            // fill it up
            for (int i = 0; i < ROOM_SIZE; i++) {
                match_data->players_fd[i] = q->head->data;
                __ts_queue_dequeue_nolock(q);
            }
            printf("matchmake: Created a room ");
            __print_queue(q);

            // add to room list
            __ts_queue_enqueue_nolock(matches, match_data);

            pthread_t game_thread;
            pthread_create(&game_thread, NULL, __handle_match,
                           match_data);
            pthread_detach(game_thread);
        }

        pthread_mutex_unlock(&q->mutex);
    }
}

void *__handle_match(void *match_data_p) {
    struct match_data *match_data = match_data_p;
    match_data->on_match_fn(match_data->players_fd);
    return NULL;
}


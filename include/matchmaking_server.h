#include "ts_queue.h"
#include "server.h"

struct client_data {
    int connfd;
};

#define ROOM_SIZE 2
typedef void (*on_match_fn_t)(struct client_data *players_fd[ROOM_SIZE]);

struct match_data {
    struct client_data *players_fd[ROOM_SIZE];
    on_match_fn_t on_match_fn;
};

struct matchmake_args {
    struct ts_queue *queue;
    on_match_fn_t on_match_fn;
    void (*on_queue)(struct ts_queue *queue);
};

void __print_queue(struct ts_queue *q);

void __enqueue_new_player(int connfd);
void *__matchmake(void *matchmake_args);

void *__handle_match(void *room_data_p);

int matchmaking_server(char *port, on_match_fn_t on_match);

void __enqueue_new_player(int connfd);


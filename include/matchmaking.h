#ifndef __MATCHMAKING_H__
#define __MATCHMAKING_H__

#include "server.h"
#include "cpong_packets.h"

#define ROOM_SIZE 2

typedef void (*on_match_fn_t)(client_t *players_fd[ROOM_SIZE]);
struct mm_args {
    server_t *server;
    on_match_fn_t on_match_fn;
};
struct room_data {
    client_t *room[ROOM_SIZE];
    on_match_fn_t on_match_fn;
};

extern pthread_cond_t mm_q_has_match;

void *matchmaking_worker(void *matchmake_args_p);
int start_matchmaking_worker(server_t *server, on_match_fn_t on_match_fn);

int mm_room_broadcast(server_t *server, client_t *room[ROOM_SIZE], struct packet packet);

#endif


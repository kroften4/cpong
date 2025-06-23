#ifndef __MATCHMAKING_H__
#define __MATCHMAKING_H__

#include "server.h"
#include "cpong_packets.h"

#define ROOM_SIZE 2

typedef void (*on_match_t)(client_t *players_fd[ROOM_SIZE]);
typedef void (*on_disband_t)(client_t *players_fd[ROOM_SIZE]);

extern pthread_cond_t mm_q_has_match;

void *matchmaking_worker(void *mm_worker_args_p);
int start_matchmaking_worker(server_t *server, on_match_t on_match_fn);

int mm_room_broadcast(server_t *server, client_t *room[ROOM_SIZE],
                      struct packet packet);

#endif


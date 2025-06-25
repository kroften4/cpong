#ifndef __MATCHMAKING_H__
#define __MATCHMAKING_H__

#include "server.h"
#include "cpong_packets.h"
#include <stdbool.h>

#define ROOM_SIZE 2

struct room {
    int id;
    client_t *clients[ROOM_SIZE];
    bool is_disbanded;
};

typedef void (*on_match_t)(struct room *room);
typedef void (*on_disband_t)(struct room *room);

extern pthread_cond_t mm_q_has_match;

void *matchmaking_worker(void *mm_worker_args_p);

int matchmaking_server_worker(server_t *server,
                              on_connection_t on_connection_fn,
                              on_match_t on_match_fn,
                              on_disband_t on_disband_fn);

int mm_room_broadcast(server_t *server, struct room *room,
                      struct packet packet);

#endif


#ifndef _CPONG_LOGIC_H
#define _CPONG_LOGIC_H

#include "vector.h"
#include <stdbool.h>

struct game_obj {
    struct vector pos;
    struct vector velocity;
    struct vector size;
};

struct pong_state {
    int player_ids[2];
    int own_id_index;
    struct game_obj player1;
    struct game_obj player2;
    struct game_obj ball;
    struct vector box_size;
};

struct game_obj linear_move(struct game_obj obj, int delta_time);

void init_game(struct pong_state *state);

void print_state(struct pong_state state);

struct coll_info {
    float toi;
    struct vector normal;
    struct vector pos;
};

bool ball_paddle_collide(struct game_obj paddle, struct game_obj ball,
                         struct game_obj paddle_next,
                         struct game_obj ball_next,
                         struct coll_info *coll_info);

struct wall {
    int up;
    int down;
    int left;
    int right;
};

bool ball_wall_collide(struct wall wall, struct game_obj ball,
                       struct game_obj ball_next,
                       struct coll_info *coll_info);

void ball_advance(struct wall wall, struct game_obj paddle1,
                  struct game_obj paddle1_next, struct game_obj paddle2,
                  struct game_obj paddle2_next, struct game_obj ball,
                  struct game_obj *ball_upd, int delta_time);

struct AABB_boundaries {
    float up;
    float down;
    float left;
    float right;
};

struct AABB_boundaries AABB_get_boundaries(struct game_obj obj);

bool AABB_is_overlapping(struct game_obj first, struct game_obj second);

#endif


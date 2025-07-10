#ifndef _CPONG_LOGIC_H
#define _CPONG_LOGIC_H

#include "vector.h"
#include "engine.h"
#include <stdbool.h>

struct pong_state {
    int player_ids[2];
    int own_id_index;
    int score[2];
    struct game_obj player1;
    struct game_obj player2;
    struct game_obj ball;
    struct vector box_size;
};

void init_ball(struct game_obj *ball);

void init_paddle(struct game_obj *paddle);

void init_game(struct pong_state *state);

void print_state(struct pong_state state);

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
                  struct game_obj *ball_upd, int delta_time, int *scored_index);

int ball_score_collide(struct wall wall, struct game_obj ball,
                       int delta_time, struct coll_info *coll_info);

#endif


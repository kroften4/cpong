#ifndef _CPONG_LOGIC_H
#define _CPONG_LOGIC_H

#include "vector.h"

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

#endif


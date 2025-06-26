#include "cpong_logic.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct game_obj linear_move(struct game_obj obj, int delta_time) {
    struct vector corrected_vel = vector_multiply(obj.velocity, delta_time);
    obj.pos = vector_add(obj.pos, corrected_vel);
    return obj;
}

void print_state(struct pong_state state) {
    printf("STATE:\n\tp1: id %d y %d\n\tp2: id %d y %d\n\tball: x %d y %d\n",
           state.player_ids[0], state.player1.pos.y,
           state.player_ids[1], state.player2.pos.y,
           state.ball.pos.x, state.ball.pos.y);
}

void init_paddle(struct game_obj *paddle) {
    paddle->size.x = 10;
    paddle->size.y = 50;
    paddle->velocity.y = 1;
}

void init_ball(struct game_obj *ball) {
    ball->size.x = 10;
    ball->size.y = 10;
    srand(time(NULL));
    int sign = rand() % 2 * 2 - 1;
    ball->velocity.x = sign * 1;
    sign = rand() % 2 * 2 - 1;
    ball->velocity.y = sign * 1;
}

void init_game(struct pong_state *state) {
    state->box_size.x = 640;
    state->box_size.y = 480;

    init_paddle(&state->player1);
    state->player1.pos.x = state->box_size.x - state->player1.size.x / 2;
    state->player1.pos.y = state->box_size.y / 2;

    init_paddle(&state->player2);
    state->player2.pos.x = state->player1.size.x / 2;
    state->player2.pos.y = state->box_size.y / 2;

    init_ball(&state->ball);
    state->ball.pos.x = state->box_size.x / 2;
    state->ball.pos.y = state->box_size.y / 2;
}


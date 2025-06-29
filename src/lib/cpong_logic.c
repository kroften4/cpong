#include "cpong_logic.h"
#include "vector.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

struct game_obj linear_move(struct game_obj obj, int delta_time) {
    struct vector corrected_vel = vector_multiply(obj.velocity, delta_time);
    // LOGF("(%.2f %.2f)->(%.2f %.2f)", obj.velocity.x, obj.velocity.y,
    //      corrected_vel.x, corrected_vel.y);
    obj.pos = vector_add(obj.pos, corrected_vel);
    return obj;
}

void print_state(struct pong_state state) {
    printf("\tp1: id %d, pos (%.2f %.2f), vel (%.2f %.2f), size (%.2f %.2f)\n"
           "\tp2: id %d, pos (%.2f %.2f), vel (%.2f %.2f), size (%.2f %.2f)\n"
           "\tball: pos (%.2f %.2f), vel (%.2f %.2f), size (%.2f %.2f)\n",
           state.player_ids[0], state.player1.pos.x, state.player1.pos.y,
           state.player1.velocity.x, state.player1.velocity.y,
           state.player1.size.x, state.player1.size.y,
           state.player_ids[1], state.player2.pos.x, state.player2.pos.y,
           state.player2.velocity.x, state.player2.velocity.y,
           state.player2.size.x, state.player2.size.y,
           state.ball.pos.x, state.ball.pos.y, 
           state.ball.velocity.x, state.ball.velocity.y,
           state.ball.size.x, state.ball.size.y);
}

void init_paddle(struct game_obj *paddle) {
    paddle->size.x = 10;
    paddle->size.y = 50;
    paddle->velocity.y = 0.5;
}

void init_ball(struct game_obj *ball) {
    ball->size.x = 10;
    ball->size.y = 10;
    float speed = 0.25;
    srand(time(NULL));
    int sign = rand() % 2 * 2 - 1;
    ball->velocity.x = sign * speed;
    sign = rand() % 2 * 2 - 1;
    ball->velocity.y = sign * speed;
    // ball->velocity = (struct vector){-1, 0};
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

struct vector linear_interpolate(struct vector pos, struct vector pos_upd,
                                 float delta_time_normalized) {
    struct vector pos_interpolated = {0, 0};
    pos_interpolated.x = delta_time_normalized * (pos_upd.x - pos.x) + pos.x;
    pos_interpolated.y = delta_time_normalized * (pos_upd.y - pos.y) + pos.y;
    return pos_interpolated;
}

/*
 * When collision happens, right side of paddle touches left side of ball, so
 * `ball.pos.x` at `collision_time` = `paddle.pos.x + paddle.size.x / 2 + ball.size.x / 2`
 * And by linear interpolation, we get
 * `ball.pos.x` at `collision_time` = `collision_time * (ball_upd.pos.x - ball.pos.x) + ball.pos.x
 * Also if at `collision_time` paddle is completely above or below the ball, the collision didn't actually happen
 */
bool ball_paddle_collide(struct game_obj paddle, struct game_obj ball,
                         struct game_obj paddle_upd,
                         struct game_obj *ball_upd, int delta_time) {
    int dir = ball.velocity.x > 0 ? 1 : -1;
    float collision_time_normalized =
        (paddle.pos.x - dir * (paddle.size.x / 2.0f + ball.size.x / 2.0f) - ball.pos.x)
        / (ball_upd->pos.x - ball.pos.x);
    if (collision_time_normalized <= 0.0f || collision_time_normalized > 1.0f) {
        return 0;
    }
    int collision_time = delta_time * collision_time_normalized;
    struct vector ball_coll_pos = linear_interpolate(ball.pos, ball_upd->pos, collision_time_normalized);
    struct vector paddle_coll_pos = linear_interpolate(paddle.pos, paddle_upd.pos, collision_time_normalized);
    if (ball_coll_pos.y + ball.size.y / 2 < paddle_coll_pos.y - paddle.size.y / 2 ||
        ball_coll_pos.y - ball.size.y / 2 > paddle_coll_pos.y + paddle.size.y / 2)
        return false;
    LOGF("NOW->NOCOL: paddle (%.2f %.2f)->(%.2f %.2f), ball (%.2f %.2f)->(%.2f %.2f), coll time %d (%.2f)",
         paddle.pos.x, paddle.pos.y, paddle_upd.pos.x, paddle_upd.pos.y,
         ball.pos.x, ball.pos.y, ball_upd->pos.x, ball_upd->pos.y,
         collision_time, collision_time_normalized);
    ball_upd->pos = ball_coll_pos;
    ball_upd->velocity = vector_multiply(ball_upd->velocity, -1);
    LOGF("NOW->COL: paddle (%.2f %.2f)->(%.2f %.2f), ball (%.2f %.2f)->(%.2f %.2f)",
         paddle.pos.x, paddle.pos.y, paddle_coll_pos.x, paddle_coll_pos.y,
         ball.pos.x, ball.pos.y, ball_upd->pos.x, ball_upd->pos.y);
    *ball_upd = linear_move(*ball_upd, delta_time - collision_time);
    LOGF("COL->UPD: paddle (%.2f %.2f)->(%.2f %.2f), ball (%.2f %.2f)->(%.2f %.2f)",
         paddle_coll_pos.x, paddle_coll_pos.y, paddle_upd.pos.x, paddle_upd.pos.y,
         ball_coll_pos.x, ball_coll_pos.y, ball_upd->pos.x, ball_upd->pos.y);
    return true;
}

bool ball_wall_left_collision(struct wall wall, struct game_obj ball,
                              struct game_obj *ball_upd, int delta_time) {
    float collision_time_normalized =
        (wall.left + ball.size.x / 2.0f - ball.pos.x)
        / (ball_upd->pos.x - ball.pos.x);
    if (collision_time_normalized <= 0.0f || collision_time_normalized > 1.0f)
        return 0;
    int collision_time = delta_time * collision_time_normalized;
    struct vector ball_coll_pos = linear_interpolate(ball.pos, ball_upd->pos, collision_time_normalized);
    ball_upd->pos = ball_coll_pos;
    ball_upd->velocity.x *= -1;
    *ball_upd = linear_move(*ball_upd, delta_time - collision_time);
    return true;
}

bool ball_wall_right_collision(struct wall wall, struct game_obj ball,
                               struct game_obj *ball_upd, int delta_time) {
    float collision_time_normalized =
        (wall.right - ball.size.x / 2.0f - ball.pos.x)
        / (ball_upd->pos.x - ball.pos.x);
    if (collision_time_normalized <= 0.0f || collision_time_normalized > 1.0f)
        return 0;
    int collision_time = delta_time * collision_time_normalized;
    struct vector ball_coll_pos = linear_interpolate(ball.pos, ball_upd->pos, collision_time_normalized);
    ball_upd->pos = ball_coll_pos;
    ball_upd->velocity.x *= -1;
    *ball_upd = linear_move(*ball_upd, delta_time - collision_time);
    return true;
}

bool ball_wall_up_collision(struct wall wall, struct game_obj ball,
                            struct game_obj *ball_upd, int delta_time) {
    float collision_time_normalized =
        (wall.up - ball.size.y / 2.0f - ball.pos.y)
        / (ball_upd->pos.y - ball.pos.y);
    if (collision_time_normalized <= 0.0f || collision_time_normalized > 1.0f)
        return 0;
    int collision_time = delta_time * collision_time_normalized;
    struct vector ball_coll_pos = linear_interpolate(ball.pos, ball_upd->pos, collision_time_normalized);
    ball_upd->pos = ball_coll_pos;
    ball_upd->velocity.y *= -1;
    *ball_upd = linear_move(*ball_upd, delta_time - collision_time);
    return true;
}

bool ball_wall_down_collision(struct wall wall, struct game_obj ball,
                              struct game_obj *ball_upd, int delta_time) {
    float collision_time_normalized =
        (wall.down + ball.size.y / 2.0f - ball.pos.y)
        / (ball_upd->pos.y - ball.pos.y);
    if (collision_time_normalized <= 0.0f || collision_time_normalized > 1.0f)
        return 0;
    int collision_time = delta_time * collision_time_normalized;
    struct vector ball_coll_pos = linear_interpolate(ball.pos, ball_upd->pos, collision_time_normalized);
    ball_upd->pos = ball_coll_pos;
    ball_upd->velocity.y *= -1;
    *ball_upd = linear_move(*ball_upd, delta_time - collision_time);
    return true;
}

bool ball_wall_collision(struct wall wall, struct game_obj ball,
                         struct game_obj *ball_upd, int delta_time) {
        return ball_wall_up_collision(wall, ball, ball_upd, delta_time) ||
               ball_wall_down_collision(wall, ball, ball_upd, delta_time) ||
               ball_wall_left_collision(wall, ball, ball_upd, delta_time) ||
               ball_wall_right_collision(wall, ball, ball_upd, delta_time);
}

bool ball_collide(struct wall wall, struct game_obj paddle1,
                    struct game_obj paddle1_upd, struct game_obj paddle2,
                    struct game_obj paddle2_upd, struct game_obj ball,
                    struct game_obj *ball_upd, int delta_time) {
    // TODO: finish this shit
    while (ball_wall_collision(wall, ball, ball_upd, delta_time) ||
           ball_paddle_collide(paddle1, ball, paddle1_upd, ball_upd, delta_time) ||
           ball_paddle_collide(paddle2, ball, paddle2_upd, ball_upd, delta_time)) {
        ball = *ball_upd;
    }
    return true;
}


#include "cpong_logic.h"
#include "log.h"
#include "vector.h"
#include "engine.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#define PI 3.1415

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
    paddle->speed = 0.5;
    paddle->velocity.x = 0;
    paddle->velocity.y = 0;
}

void init_ball(struct game_obj *ball) {
    ball->size.x = 10;
    ball->size.y = 10;
    ball->speed = 0.25;
    srand(time(NULL));
    struct vector angle = vector_random_angle(PI/6, PI/3, 0.1);
    int dir_x = rand() % 2 * 2 - 1;
    int dir_y = rand() % 2 * 2 - 1;
    ball->velocity.x = angle.x * dir_x * ball->speed;
    ball->velocity.y = angle.y * dir_y * ball->speed;
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

/*
 * When collision happens, right side of paddle touches left side of ball, so
 * `ball.pos.x` at TOI = `paddle.pos.x + paddle.size.x / 2 + ball.size.x / 2`
 * And by linear interpolation, we get
 * `ball.pos.x` at TOI = `toi_ms * (ball_next.pos.x - ball.pos.x) + ball.pos.x
 * Also if at TOI paddle is completely above or below the ball, the collision didn't actually happen
 *
 * returns whether a collision has occured in the time frame between `ball` and `ball_next`
 * puts collision info into `coll_info`
 */
bool ball_x_collide(int x, struct game_obj ball, struct game_obj ball_next,
                    struct coll_info *coll_info) {
    int dir = ball.velocity.x > 0 ? 1 : -1;
    coll_info->toi = (x - dir * ball.size.x / 2 - ball.pos.x)
        / (ball_next.pos.x - ball.pos.x);
    if (!is_valid_toi(coll_info->toi))
        return false;
    coll_info->pos = linear_interpolate(ball.pos, ball_next.pos, coll_info->toi);
    coll_info->normal.x = -dir;
    coll_info->normal.y = 0;
    return true;
}

float ball_y_collide(int y, struct game_obj ball, struct game_obj ball_next,
                     struct coll_info *coll_info) {
    int dir = ball.velocity.y > 0 ? 1 : -1;
    coll_info->toi = (y - dir * ball.size.y / 2 - ball.pos.y)
        / (ball_next.pos.y - ball.pos.y);
    if (!is_valid_toi(coll_info->toi))
        return false;
    coll_info->pos = linear_interpolate(ball.pos, ball_next.pos, coll_info->toi);
    coll_info->normal.x = 0;
    coll_info->normal.y = -dir;
    return true;
}

bool ball_paddle_collide(struct game_obj paddle, struct game_obj ball,
                         struct game_obj paddle_next,
                         struct game_obj ball_next,
                         struct coll_info *coll_info) {
    struct AABB_boundaries paddle_bounds = AABB_get_boundaries(paddle);
    struct coll_info collisions[4] = {0};
    ball_x_collide(paddle_bounds.left, ball, ball_next, &collisions[0]);
    ball_x_collide(paddle_bounds.right, ball, ball_next, &collisions[1]);
    ball_y_collide(paddle_bounds.up, ball, ball_next, &collisions[2]);
    ball_y_collide(paddle_bounds.down, ball, ball_next, &collisions[3]);
    if (!get_first_collision(collisions, 4, coll_info))
        return false;

    struct game_obj ball_at_toi = ball;
    ball_at_toi.pos = coll_info->pos;
    struct AABB_boundaries ball_bounds_at_toi = AABB_get_boundaries(ball_at_toi);
    struct game_obj paddle_at_toi = paddle;
    paddle_at_toi.pos = linear_interpolate(paddle.pos, paddle_next.pos, coll_info->toi);
    struct AABB_boundaries paddle_bounds_at_toi = AABB_get_boundaries(paddle_at_toi);
    if (ball_bounds_at_toi.up < paddle_bounds_at_toi.down ||
        ball_bounds_at_toi.down > paddle_bounds_at_toi.up) {
        coll_info->toi = -1;
        return false;
    }
    if (ball_bounds_at_toi.right < paddle_bounds_at_toi.left ||
        ball_bounds_at_toi.left > paddle_bounds_at_toi.right) {
        coll_info->toi = -1;
        return false;
    }
    LOG("collision with paddle");
    return true;
}

bool ball_wall_collide(struct wall wall, struct game_obj ball,
                       struct game_obj ball_next,
                       struct coll_info *coll_info) {
    struct coll_info collisions[2] = {0};
    ball_y_collide(wall.up, ball, ball_next, &collisions[0]);
    ball_y_collide(wall.down, ball, ball_next, &collisions[1]);
    if (!get_first_collision(collisions, 2, coll_info))
        return false;
    LOG("collision with wall");
    return true;
}

int ball_score_collide(struct wall wall, struct game_obj ball,
                       int delta_time, struct coll_info *coll_info) {
    struct game_obj ball_next = linear_move(ball, delta_time);
    int scored_index = -1;
    if (ball_x_collide(wall.left, ball, ball_next, coll_info))
        scored_index = 0;
    if (ball_x_collide(wall.right, ball, ball_next, coll_info))
        scored_index = 1;
    return scored_index;
}

void ball_bounce_on_collision(struct game_obj *ball, struct vector normal) {
    ball->velocity = reflect_orthogonal(ball->velocity, normal);
}

void ball_advance(struct wall wall, struct game_obj paddle1,
                  struct game_obj paddle1_next, struct game_obj paddle2,
                  struct game_obj paddle2_next, struct game_obj ball,
                  struct game_obj *ball_upd, int delta_time, int *scored_index) {
    float passed_time = 0;
    while (passed_time < delta_time) {
        struct game_obj ball_next = linear_move(ball, delta_time - passed_time);
        struct coll_info coll_info = {0};
        struct coll_info collisions[4] = {0};
        ball_paddle_collide(paddle1, ball, paddle1_next, ball_next,
                            &collisions[0]);
        ball_paddle_collide(paddle2, ball, paddle2_next, ball_next,
                            &collisions[1]);
        ball_wall_collide(wall, ball, ball_next, &collisions[2]);
        *scored_index = ball_score_collide(wall, ball, delta_time - passed_time, &collisions[3]);
        if (!get_first_collision(collisions, 4, &coll_info)) {
            ball = ball_next;
            break;
        }
        if (coll_info.toi == collisions[3].toi) {
            return;
        }
        *scored_index = -1;
        ball.pos = coll_info.pos;
        passed_time += coll_info.toi;
        ball_bounce_on_collision(&ball, coll_info.normal);
    }
    *ball_upd = ball;
}


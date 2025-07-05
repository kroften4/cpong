#include "cpong_logic.h"
#include "vector.h"
#include <stddef.h>
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
 * `ball.pos.x` at TOI = `paddle.pos.x + paddle.size.x / 2 + ball.size.x / 2`
 * And by linear interpolation, we get
 * `ball.pos.x` at TOI = `toi_ms * (ball_upd.pos.x - ball.pos.x) + ball.pos.x
 * Also if at TOI paddle is completely above or below the ball, the collision didn't actually happen
 *
 * returns `toi` if collision happened, and -1.0f if it didn't;
 * changes `ball_upd.pos` to ball position at TOI;
 */
float ball_paddle_collide(struct game_obj paddle, struct game_obj ball,
                          struct game_obj paddle_upd,
                          struct game_obj *ball_upd, float *toi,
                          struct vector *normal) {
    int dir = ball.velocity.x > 0 ? 1 : -1;
    *toi = (paddle.pos.x - dir * (paddle.size.x / 2.0f + ball.size.x / 2.0f) - ball.pos.x)
        / (ball_upd->pos.x - ball.pos.x);
    if (*toi <= 0.0f || *toi > 1.0f)
        return -1;

    struct game_obj ball_at_toi = ball;
    ball_at_toi.pos = linear_interpolate(ball.pos, ball_upd->pos, *toi);
    struct AABB_boundaries ball_bounds_at_toi = AABB_get_boundaries(ball_at_toi);
    struct game_obj paddle_at_toi = paddle;
    paddle_at_toi.pos = linear_interpolate(paddle.pos, paddle_upd.pos, *toi);
    struct AABB_boundaries paddle_bounds_at_toi = AABB_get_boundaries(paddle_at_toi);
    if (ball_bounds_at_toi.up < paddle_bounds_at_toi.down ||
        ball_bounds_at_toi.down > paddle_bounds_at_toi.up)
        return -1;

    ball_upd->pos = ball_at_toi.pos;
    return *toi;
}

bool ball_x_collide(int x, struct game_obj ball, struct game_obj *ball_upd,
                    float *toi, struct vector *normal) {
    int dir = ball.velocity.x > 0 ? 1 : -1;
    *toi = (x - dir * ball.size.x / 2 - ball.pos.x)
        / (ball_upd->pos.x - ball.pos.x);
    if (*toi <= 0.0f || *toi > 1.0f)
        return false;
    ball_upd->pos = linear_interpolate(ball.pos, ball_upd->pos, *toi);
    normal->x = -dir;
    normal->y = 0;
    return true;
}

float ball_y_collide(int y, struct game_obj ball, struct game_obj *ball_upd,
                     float *toi, struct vector *normal) {
    int dir = ball.velocity.y > 0 ? 1 : -1;
    *toi = (y - dir * ball.size.y / 2 - ball.pos.y)
        / (ball_upd->pos.y - ball.pos.y);
    if (*toi <= 0.0f || *toi > 1.0f)
        return false;
    ball_upd->pos = linear_interpolate(ball.pos, ball_upd->pos, *toi);
    normal->x = 0;
    normal->y = -dir;
    return true;
}

bool ball_wall_collide(struct wall wall, struct game_obj ball,
                       struct game_obj *ball_upd, float *toi,
                       struct vector *normal) {
        return ball_y_collide(wall.up, ball, ball_upd, toi, normal) ||
               ball_y_collide(wall.down, ball, ball_upd, toi, normal) ||
               ball_x_collide(wall.left, ball, ball_upd, toi, normal) ||
               ball_x_collide(wall.right, ball, ball_upd, toi, normal);
}

int min_idx(float arr[], size_t size) {
    int res = 0;
    for (size_t i = 1; i <= size / sizeof(float); i++) {
        if (arr[res] < arr[i])
            res = i;
    }
    return res;
}

void ball_collision_respond(struct game_obj *ball, struct vector normal) {
    ball->velocity = reflect_orthogonal(ball->velocity, normal);
}

bool ball_advance(struct wall wall, struct game_obj paddle1,
                  struct game_obj paddle1_upd, struct game_obj paddle2,
                  struct game_obj paddle2_upd, struct game_obj ball,
                  struct game_obj *ball_upd, int delta_time) {
    float passed_time = 0;
    while (true) {
        bool has_collided = false;
        float wall_toi, paddle1_toi, paddle2_toi;
        struct vector wall_normal, paddle1_normal, paddle2_normal;
        has_collided += ball_wall_collide(wall, ball, ball_upd, &wall_toi, &wall_normal);
        has_collided += ball_paddle_collide(paddle1, ball, paddle1_upd, ball_upd, &paddle1_toi, &paddle1_normal);
        has_collided += ball_paddle_collide(paddle2, ball, paddle2_upd, ball_upd, &paddle2_toi, &paddle2_normal);
        if (!has_collided)
            return passed_time;
        float tois[] = {wall_toi, paddle1_toi, paddle2_toi};
        struct vector normals[] = {wall_normal, paddle1_normal, paddle2_normal};
        int coll_idx = min_idx(tois, sizeof(tois));
        float toi = tois[coll_idx];
        struct vector normal = normals[coll_idx];
        int collision_step_time = toi * (delta_time - passed_time);
        ball_collision_respond(&ball, normal);
        struct game_obj ball_coll = linear_move(ball, collision_step_time);
        ball = *ball_upd;
        *ball_upd = ball_coll;
        passed_time += collision_step_time;
    }
}

struct AABB_boundaries AABB_get_boundaries(struct game_obj obj) {
    struct AABB_boundaries boundaries = {
        .up = obj.pos.y + obj.size.y / 2,
        .down = obj.pos.y - obj.size.y / 2,
        .left = obj.pos.x - obj.size.x / 2,
        .right = obj.pos.x + obj.size.x / 2
    };
    return boundaries;
}

bool AABB_is_overlapping(struct game_obj first, struct game_obj second) {
    struct AABB_boundaries bounds1 = AABB_get_boundaries(first);
    struct AABB_boundaries bounds2 = AABB_get_boundaries(second);
    if (bounds1.down > bounds2.up)
        return true;
    if (bounds1.up < bounds2.down)
        return true;
    if (bounds1.left > bounds2.right)
        return true;
    if (bounds1.right < bounds2.left)
        return true;
    return false;
}


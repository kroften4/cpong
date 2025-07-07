#include "cpong_logic.h"
#include "log.h"
#include "vector.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#define PI 3.1415

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

struct AABB_boundaries AABB_get_boundaries(struct game_obj obj) {
    struct AABB_boundaries boundaries = {
        .up = obj.pos.y + obj.size.y / 2,
        .down = obj.pos.y - obj.size.y / 2,
        .left = obj.pos.x - obj.size.x / 2,
        .right = obj.pos.x + obj.size.x / 2
    };
    return boundaries;
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
    struct vector angle = vector_random_angle(PI/6, PI/3, 0.1);
    int dir_x = rand() % 2 * 2 - 1;
    int dir_y = rand() % 2 * 2 - 1;
    ball->velocity.x = angle.x * dir_x * speed;
    ball->velocity.y = angle.y * dir_y * speed;
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

struct vector linear_interpolate(struct vector pos, struct vector pos_next,
                                 float delta_time_normalized) {
    struct vector pos_interpolated = {0, 0};
    pos_interpolated.x = delta_time_normalized * (pos_next.x - pos.x) + pos.x;
    pos_interpolated.y = delta_time_normalized * (pos_next.y - pos.y) + pos.y;
    return pos_interpolated;
}

bool is_valid_toi(float toi) {
    return 0.0f < toi && toi <= 1.0f;
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

bool get_first_collision(struct coll_info *collisions, size_t amount, struct coll_info *result) {
    result->toi = 99;
    for (size_t i = 0; i < amount; i++) {
        if (is_valid_toi(collisions[i].toi) && collisions[i].toi < result->toi)
            *result = collisions[i];
    }
    return result->toi != 99;
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
    struct coll_info collisions[4] = {0};
    ball_y_collide(wall.up, ball, ball_next, &collisions[0]);
    ball_y_collide(wall.down, ball, ball_next, &collisions[1]);
    ball_x_collide(wall.left, ball, ball_next, &collisions[2]);
    ball_x_collide(wall.right, ball, ball_next, &collisions[3]);
    if (!get_first_collision(collisions, 4, coll_info))
        return false;
    LOG("collision with wall");
    return true;
}

int min_idx(float arr[], size_t size) {
    int res = 0;
    for (size_t i = 1; i <= size / sizeof(float); i++) {
        if (arr[res] < arr[i])
            res = i;
    }
    return res;
}

void ball_bounce_on_collision(struct game_obj *ball, struct vector normal) {
    ball->velocity = reflect_orthogonal(ball->velocity, normal);
}

void ball_advance(struct wall wall, struct game_obj paddle1,
                  struct game_obj paddle1_next, struct game_obj paddle2,
                  struct game_obj paddle2_next, struct game_obj ball,
                  struct game_obj *ball_upd, int delta_time) {
    float passed_time = 0;
    while (passed_time < delta_time) {
        struct game_obj ball_next = linear_move(ball, delta_time - passed_time);
        struct coll_info coll_info = {0};
        struct coll_info collisions[3] = {0};
        ball_paddle_collide(paddle1, ball, paddle1_next, ball_next,
                            &collisions[0]);
        ball_paddle_collide(paddle2, ball, paddle2_next, ball_next,
                            &collisions[1]);
        ball_wall_collide(wall, ball, ball_next, &collisions[2]);
        if (!get_first_collision(collisions, 3, &coll_info)) {
            ball = ball_next;
            break;
        }
        ball.pos = coll_info.pos;
        passed_time += coll_info.toi;
        ball_bounce_on_collision(&ball, coll_info.normal);
    }
    *ball_upd = ball;
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


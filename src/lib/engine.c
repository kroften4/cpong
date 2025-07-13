#include "engine.h"
#include <stddef.h>

struct game_obj linear_move(struct game_obj obj, int delta_time) {
    struct vector corrected_vel = vector_multiply(obj.velocity, delta_time);
    obj.pos = vector_add(obj.pos, corrected_vel);
    return obj;
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

bool get_first_collision(struct coll_info *collisions, size_t amount, struct coll_info *result) {
    result->toi = 99;
    for (size_t i = 0; i < amount; i++) {
        if (is_valid_toi(collisions[i].toi) && collisions[i].toi < result->toi)
            *result = collisions[i];
    }
    return result->toi != 99;
}


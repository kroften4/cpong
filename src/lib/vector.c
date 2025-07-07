#include "vector.h"
#include "log.h"
#include <math.h>
#include <stdlib.h>

struct vector vector_add(struct vector first, struct vector second) {
    first.x += second.x;
    first.y += second.y;
    return first;
}

struct vector vector_multiply(struct vector vec, float scalar) {
    vec.x *= scalar;
    vec.y *= scalar;
    return vec;
}

struct vector reflect_orthogonal(struct vector vec, struct vector normal_orthogonal) {
    if (normal_orthogonal.x == 0)
        vec.y = -vec.y;
    else
        vec.x = -vec.x;
    return vec;
}

struct vector vector_normalize(struct vector vec) {
    float len = sqrt(vec.x * vec.x + vec.y * vec.y);
    struct vector res = {vec.x / len, vec.y / len};
    return res;
}

struct vector vector_random_angle(float min, float max, float step) {
    int rnd = rand();
    int range = (max - min) / step;
    int rnd_amount = rnd % range;
    float rnd_angle = min + step * rnd_amount;
    struct vector result = {cos(rnd_angle), sin(rnd_angle)};
    LOGF("rnd: %d, range: %d, rnd_amount: %d, rnd_angle: %.2f, result: (%.2f %.2f)",
         rnd, range, rnd_amount, rnd_angle, result.x, result.y);
    return result;
}

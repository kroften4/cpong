#include "vector.h"

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
    if (normal_orthogonal.x != 0)
        vec.y = -vec.y;
    else
        vec.x = -vec.x;
    return vec;
}

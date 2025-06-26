#include "vector.h"

struct vector vector_add(struct vector first, struct vector second) {
    first.x += second.x;
    first.y += second.y;
    return first;
}

struct vector vector_multiply(struct vector vec, int scalar) {
    vec.x *= scalar;
    vec.y *= scalar;
    return vec;
}


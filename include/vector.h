#ifndef _VECTOR_H
#define _VECTOR_H

struct vector {
    int x;
    int y;
};

struct vector vector_add(struct vector first, struct vector second);

struct vector vector_multiply(struct vector vec, int scalar);

#endif


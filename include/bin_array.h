#ifndef __BIN_ARRAY_H__
#define __BIN_ARRAY_H__

#include <inttypes.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

struct binarr {
    int8_t *buf;
    size_t size;
};

struct binarr *binarr_new(struct binarr *arr, size_t capacity) {
    int8_t *buf = malloc(capacity);
    if (buf == NULL) {
        perror("malloc");
    }
    arr->buf = buf;
    arr->size = 0;
    return arr;
}

void binarr_destroy(struct binarr arr) {
    free(arr.buf);
}

void binarr_append_i8(struct binarr *arr, int8_t data) {
    memcpy(arr->buf + arr->size, &data, sizeof(data));
    arr->size += sizeof(data);
}

void binarr_append_i16(struct binarr *arr, int16_t data) {
    memcpy(arr->buf + arr->size, &data, sizeof(data));
    arr->size += sizeof(data);
}

void binarr_append_i16_n(struct binarr *arr, int16_t data) {
    data = htons(data);
    binarr_append_i16(arr, data);
}

void binarr_append_i32(struct binarr *arr, int32_t data) {
    memcpy(arr->buf + arr->size, &data, sizeof(data));
    arr->size += sizeof(data);
}

void binarr_append_i32_n(struct binarr *arr, int32_t data) {
    data = htons(data);
    binarr_append_i32(arr, data);
}

#endif


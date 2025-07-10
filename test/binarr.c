#include "krft/bin_array.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    struct binarr barr1 = {0};
    binarr_new(&barr1, 100);
    binarr_append_i8(&barr1, 12);
    barr1.index = 0;
    int8_t i8 = binarr_read_i8(&barr1);
    assert(i8 == 12 && "Invalid value after append and read i8");

    struct binarr barr2 = {0};
    binarr_new(&barr2, 100);
    binarr_append_i8(&barr2, 12);
    binarr_append_i32(&barr2, 50);
    barr2.index = 0;
    i8 = binarr_read_i8(&barr2);
    int32_t i32 = binarr_read_i32(&barr2);
    assert(i8 == 12 && "Invalid value of i8 after append and read i8 + i32");
    assert(i32 == 50 && "Invalid value of i32 after append and read i8 + i32");

    struct binarr barrn2 = {0};
    binarr_new(&barrn2, 100);
    binarr_append_i8(&barrn2, 8);
    unsigned int u32 = 10;
    binarr_append_i32_n(&barrn2, u32);
    i32 = 14;
    binarr_append_i32_n(&barrn2, i32);
    barrn2.index = 0;
    i8 = binarr_read_i8(&barrn2);
    u32 = binarr_read_i32_n(&barrn2);
    i32 = binarr_read_i32_n(&barrn2);
    assert(i8 == 8 && "Invalid value of i8 after network append and read i8 + u32 + i32");
    assert(u32 == 10 && "Invalid value of u32 after network append and read i8 + u32 + i32");
    assert(i32 == 14 && "Invalid value of i32 after network append and read i8 + u32 + i32");

    puts("binarr: All tests passed");
    return 0;
}

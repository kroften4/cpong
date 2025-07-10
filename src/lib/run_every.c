#include "krft/run_every.h"
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

int get_curr_time_ms(void) {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct timespec ms_to_timespec(int time_ms) {
    struct timespec rt = {
        .tv_sec = time_ms / 1000,
        .tv_nsec = time_ms % 1000 * 1000000
    };
    return rt;
}

void sleep(int time_ms) {
    if (time_ms > 0) {
        struct timespec rt = ms_to_timespec(time_ms);
        nanosleep(&rt, NULL);
    }
}

void run_every(struct run_every_args re_args) {
    int last = get_curr_time_ms() - re_args.interval_ms;
    bool run = true;
    while (run) {
        int start = get_curr_time_ms();
        int delta_time = start - last;
        run = re_args.func(delta_time, re_args.args);
        int sleep_time = re_args.interval_ms - (get_curr_time_ms() - start);
        sleep(sleep_time);
        last = start;
    }
}

void *start_run_every_routine(void *re_args_p) {
    struct run_every_args *re_args = re_args_p;
    run_every(*re_args);
    return NULL;
}

pthread_t start_run_every_thread(struct run_every_args *re_args_p) {
    pthread_t run_every_thread;
    pthread_create(&run_every_thread, NULL, start_run_every_routine, re_args_p);
    return run_every_thread;
}


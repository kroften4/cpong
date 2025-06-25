#ifndef _RUN_EVERY_H
#define _RUN_EVERY_H

#include <time.h>
#include <stdbool.h>
#include <pthread.h>

int get_curr_time_ms(void);

struct timespec ms_to_timespec(int time_ms);

void sleep(int time_ms);

struct run_every_args {
    bool (*func)(int delta_time, void *args);
    void *args;
    long interval_ms;
};

void run_every(struct run_every_args re_args);

void *start_run_every_routine(void *re_args_p);

pthread_t start_run_every_thread(struct run_every_args *re_args_p);

#endif


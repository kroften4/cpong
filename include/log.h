#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>

#define LOG_PREFIX "L"
#define WARN_PREFIX "W"
#define ERROR_PREFIX "E"

#define __RED_ANSI "\x1B[0;31m"
#define __YELLOW_ANSI "\x1B[0;33m"
#define __RESET_ANSI "\x1B[0m"

#define LOG_INFO(prefix) printf("[" prefix "][" __FILE__ ":%d][%s] ", __LINE__, __func__)

#define __LOG_ARGS(level, message) "[" level "][" __FILE__ ":%d][%s][" message "]\n" __RESET_ANSI, __LINE__, __func__
#define LOGF(message, ...) printf(__LOG_ARGS(LOG_PREFIX, message) __VA_OPT__(,) __VA_ARGS__)
#define WARNF(message, ...) fprintf(stderr, __YELLOW_ANSI __LOG_ARGS(WARN_PREFIX, message) __VA_OPT__(,)  __VA_ARGS__)
#define ERRORF(message, ...) fprintf(stderr, __RED_ANSI __LOG_ARGS(ERROR_PREFIX, message) __VA_OPT__(,)  __VA_ARGS__)

#define LOG(message) printf(__LOG_ARGS(LOG_PREFIX, message))
#define WARN(message) fprintf(stderr, __LOG_ARGS(WARN_PREFIX, message))
#define ERROR(message) fprintf(stderr, __LOG_ARGS(ERROR_PREFIX, message))

#endif // _UTILS_H


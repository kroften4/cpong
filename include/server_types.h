#ifndef __SERVER_TYPES_H__
#define __SERVER_TYPES_H__

typedef struct {
    int fd;
    struct ts_queue *clients;
} server_t;

typedef struct {
    int fd;
    int id;
    server_t *server;
} client_t;

#endif


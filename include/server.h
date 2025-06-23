#ifndef __SERVER_H__
#define __SERVER_H__

#include "bin_array.h"

/*
 * Backlog for `listen` call
 */
#define SERVER_BACKLOG 10

typedef struct {
    int fd;
    struct ts_queue *clients;
} server_t;

typedef struct {
    int fd;
    int id;
    server_t *server;
} client_t;

void __print_queue(struct ts_queue *q);

int server_send(server_t *server, client_t client, struct binarr barr);

/*
 * Start a TCP server
 *
 * Returns server socket fd on success, and -1 on error
 *
 * Also spams errors if any occur into console
 */
server_t *server_set_fd(server_t *server, char *port);

typedef void (*on_connection_t)(client_t client);

/*
 * Start a TCP server and call `on_connection` in a new thread on connection
 * 
 * Returns -1 on error
 */
void *server_worker(server_t *server, on_connection_t);

#endif

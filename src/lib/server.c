#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "ts_queue.h"
#include "server.h"
#include "log.h"

server_t *server_set_fd(server_t *server, char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *server_ai;
    int ai_status = getaddrinfo(NULL, port, &hints, &server_ai);
    if (ai_status != 0) {
        freeaddrinfo(server_ai);
        ERRORF("addrinfo: %s", gai_strerror(ai_status));
        return NULL;
    }

    int sockfd;
    for ( ; server_ai != NULL; server_ai = server_ai->ai_next) {
        sockfd = socket(server_ai->ai_family, server_ai->ai_socktype, 0);
        if (sockfd == -1) {
            perror("server start: socket");
            continue;
        }

        if (bind(sockfd, server_ai->ai_addr, server_ai->ai_addrlen) == -1) {
            perror("server start: bind");
            close(sockfd);
            continue;
        }

        break;
    }

    if (server_ai == NULL) {
        ERROR("Failed to bind");
        return NULL;
    }

    freeaddrinfo(server_ai);

    if (listen(sockfd, SERVER_BACKLOG) == -1) {
        perror("server start: listen");
        close(sockfd);
        return NULL;
    }

    server->fd = sockfd;
    return server;
}

struct conn_data {
    client_t cl_data;
    void (*on_connection_fn)(client_t cl_data);
};

void server_handle_disconnection(server_t *server, client_t client) {
    pthread_mutex_lock(&server->clients->mutex);
    struct ts_queue_node *client_node = NULL;
    for (struct ts_queue_node *p = server->clients->head; p != NULL; p = p->next) {
        client_t *client_p = p->data;
        if (client_p->id == client.id) {
            client_node = p;
            break;
        }
    }
    if (client_node == NULL) {
        ERRORF("Cannot find client id %d", client.id);
    }
    else {
        close(client.fd);
        free(client_node->data);
        __ts_queue_remove_nolock(server->clients, client_node->prev, client_node->next);
    }
    pthread_mutex_unlock(&server->clients->mutex);
}

int server_send(server_t *server, client_t client, struct binarr barr) {
    if (send(client.fd, barr.buf, barr.size, MSG_NOSIGNAL) == -1) {
        server_handle_disconnection(server, client);
        return -1;
    }
    return 0;
}

void *handle_connection(void *conn_data) {
    struct conn_data *c_data = (struct conn_data *) conn_data;
    c_data->on_connection_fn(c_data->cl_data);
    free(c_data);
    return NULL;
}

void __print_queue(struct ts_queue *q) {
    printf("[ ");
    for (struct ts_queue_node *node = q->head; node != NULL;
         node = node->next) {
        client_t *client = node->data;
        printf("%d ", client->id);
    }
    printf("]\n");
}

void *server_worker(server_t *server, void on_connection(client_t)) {
    for (;;) {
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int connfd = accept(server->fd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd == -1) {
            perror("server: accept");
            continue;
        }
        LOGF("New connection: %d ", connfd);

        client_t *client = malloc(sizeof(client_t));
        client->fd = connfd;
        client->server = server;
        client->id = connfd;
        ts_queue_enqueue(server->clients, client);
        pthread_mutex_lock(&server->clients->mutex);
        __print_queue(server->clients);
        pthread_mutex_unlock(&server->clients->mutex);

        if (on_connection != NULL) {
            struct conn_data *conn_data = malloc(sizeof(struct conn_data));
            if (conn_data == NULL) {
                perror("server: malloc");
                continue;
            }
            conn_data->cl_data = *client;
            conn_data->on_connection_fn = on_connection;
            pthread_t conn_thread;
            pthread_create(&conn_thread, NULL, handle_connection, conn_data);
            pthread_detach(conn_thread);
        }
    }
}


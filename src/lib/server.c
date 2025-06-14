#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "server.h"

int start_server(char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *server_ai;
    int ai_status = getaddrinfo(NULL, port, &hints, &server_ai);
    if (ai_status != 0) {
        freeaddrinfo(server_ai);
        fprintf(stderr, "server start: addrinfo: %s\n", gai_strerror(ai_status));
        return -1;
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
        fprintf(stderr, "server start: Failed to bind\n");
        return -1;
    }

    freeaddrinfo(server_ai);

    if (listen(sockfd, SERVER_BACKLOG) == -1) {
        perror("server start: listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

struct conn_data {
    int connfd;
    void (*on_connection_fn)(int);
};

void *handle_connection(void *conn_data) {
    struct conn_data *c_data = (struct conn_data *) conn_data;
    c_data->on_connection_fn(c_data->connfd);
    free(c_data);
    return NULL;
}

int server(char *port, void on_connection(int connfd)) {
    int servfd = start_server(port);
    if (servfd == -1) {
        return -1;
    }
    printf("server: Listening on %s\n", port);

    for (;;) {
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int connfd = accept(servfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd == -1) {
            perror("server: accept");
            continue;
        }

        printf("server: New connection: %d\n", connfd);

        struct conn_data *conn_data = malloc(sizeof(struct conn_data));
        if (conn_data == NULL) {
            perror("server: malloc");
            continue;
        }
        conn_data->connfd = connfd;
        conn_data->on_connection_fn = on_connection;
        pthread_t conn_thread;
        pthread_create(&conn_thread, NULL, (void *(*)(void *))&handle_connection, conn_data);
        pthread_detach(conn_thread);
    }
}


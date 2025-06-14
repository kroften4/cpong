#include "matchmaking_server.h"
#include "cpong_packets.h"
#include <stdio.h>

void play_pong(struct client_data **cl_data);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>", argv[0]);
    }
    char *port = argv[1];
    matchmaking_server(port, play_pong);
}

void play_pong(struct client_data **cl_data) {
    
}


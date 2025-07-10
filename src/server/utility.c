#include "cpong_packets.h"
#include "krft/matchmaking.h"

int room_broadcast(server_t *server, struct room *room, struct packet packet) {
    struct binarr barr = {0};
    binarr_new(&barr, MAX_PACKET_SIZE);
    packet_serialize(&barr, packet);
    for (int i = 0; i < ROOM_SIZE; i++) {
        // TODO: handle disconnection
        client_t *receiver = room->clients[i];
        if (server_send(server, *receiver, barr) == -1) {
            handle_disband(room);
            return -1;
        };
    }
    binarr_destroy(barr);
    return 0;
}

short get_input_acc_direction(int input_acc) {
    if (input_acc > 0)
        return 1;
    if (input_acc == 0)
        return 0;
    return -1;
}


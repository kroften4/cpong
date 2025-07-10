#include "krft/server.h"
#include "krft/matchmaking.h"
#include "cpong_packets.h"

int room_broadcast(server_t *server, struct room *room, struct packet packet);
short get_input_acc_direction(int input_acc);


#ifndef UTILS_H
#define UTILS_H

#include "server.h"

void update_room_txtfile();
int return_room_num(int client_sock);
void remove_client(int client_sock);

#endif
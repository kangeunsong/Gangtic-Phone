#ifndef UTILS_H
#define UTILS_H

#include "server.h"

void update_room_txtfile();
int return_room_num(int client_sock);
void remove_client(int client_sock);
void writing_result(int Rnumber);
void init_game_log(int Rnumber);
void write_round_start(int Rnumber, int round_num);
void write_answer_set(int Rnumber, const char* answer);
void write_guess_result(int Rnumber, const char* nickname, const char* guess, int is_correct);
void write_point_gained(int Rnumber, const char* nickname, int points);

#endif
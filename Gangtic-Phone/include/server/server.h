#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_ROOMS 4
#define MAX_USERS_PER_ROOM 4
#define MAX_ROUND 5
#define MAXLINE 1000
#define PORT 8080

typedef struct {
    int is_existing;
    int users_num;
    int users_sknum[4];
    int users_score[4];
    char users_nickname[4][MAXLINE];
    char *answer;
    int round;
    int drawing_sknum;
} Game;

extern Game games[MAX_ROOMS];
extern int rooms_num;
extern pthread_mutex_t rooms_mutex;

#endif
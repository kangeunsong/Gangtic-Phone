#ifndef CLIENT_H
#define CLIENT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

typedef enum
{
    HOME_SCREEN,
    WAITING_ROOM,
    GAME_PAINTER_SCREEN,
    GAME_PLAYER_SCREEN,
    RESULT_SCREEN,
    SETTING_SCREEN,
    WAITING_FOR_PLAYERS
} GameState;

extern int render_update_needed;
extern GameState current_state;
extern char *nickname;
extern int sockfd;
extern int sockID;
extern int answer_num;

void send_command(const char* command);

#endif // CLIENT_H
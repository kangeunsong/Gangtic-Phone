#ifndef WAITING_H
#define WAITING_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

extern int current_room_num;

void render_waiting_room(SDL_Renderer *renderer);
void close_waiting_room();

#endif // WAITING_H
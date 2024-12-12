#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

void render_game_screen(SDL_Renderer *renderer, TTF_Font *font);
void close_game_screen();

#endif // GAME_H
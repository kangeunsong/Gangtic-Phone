#ifndef RESULT_H
#define RESULT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

void render_result_screen(SDL_Renderer *renderer, TTF_Font *font);
void close_result_screen();

#endif // RESULT_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

SDL_Texture *gangtic_texture = NULL;
SDL_Texture *phone_texture = NULL;
SDL_Texture *home_howto_texture = NULL;
SDL_Texture *home_mountain_background_texture = NULL;
SDL_Texture *setting_button_texture = NULL;
SDL_Texture *start_button_texture = NULL;

int init_home_screen_images(SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load("assets/images/GANGTIC.png");
    gangtic_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/PHONE.png");
    phone_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/home_howto.png");
    home_howto_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/mountain_background.png");
    home_mountain_background_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/setting_button.png");
    setting_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/start_button.png");
    start_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return 1;
}

void close_home_screen(){
    SDL_DestroyTexture(gangtic_texture);
    SDL_DestroyTexture(phone_texture);
    SDL_DestroyTexture(home_howto_texture);
    SDL_DestroyTexture(home_mountain_background_texture);
    SDL_DestroyTexture(setting_button_texture);
    SDL_DestroyTexture(start_button_texture);
}

void render_home_screen(SDL_Renderer *renderer)
{
    if (!init_home_screen_images(renderer))
    {
        printf("홈 화면 이미지 로드 실패\n");
        return;
    }

    SDL_RenderCopy(renderer, home_mountain_background_texture, NULL, NULL);

    SDL_Rect gangtic_rect = {195, 92, 1000, 210};
    SDL_RenderCopy(renderer, gangtic_texture, NULL, &gangtic_rect);

    SDL_Rect phone_rect = {370, 310, 700, 182};
    SDL_RenderCopy(renderer, phone_texture, NULL, &phone_rect);

    SDL_Rect start_button_rect = {58, 580, 479, 197};
    SDL_RenderCopy(renderer, start_button_texture, NULL, &start_button_rect);

    SDL_Rect setting_button_rect = {58, 822, 368, 141};
    SDL_RenderCopy(renderer, setting_button_texture, NULL, &setting_button_rect);

    SDL_Rect home_howto_rect = {661, 663, 779, 394};
    SDL_RenderCopy(renderer, home_howto_texture, NULL, &home_howto_rect);


    SDL_RenderPresent(renderer);
}
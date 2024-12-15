#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "client.h"
#include "waiting.h"

SDL_Texture *exit_button_texture = NULL;
SDL_Texture *tohome_button_texture = NULL;
SDL_Texture *result_background_texture = NULL;
SDL_Texture *result_texture = NULL;

TTF_Font *small_font = NULL; // 작은 폰트를 위한 변수

int init_result_screen_images(SDL_Renderer *renderer);
void render_result_screen(SDL_Renderer *renderer, TTF_Font *font);
void display_leaderboard(SDL_Renderer *renderer);
void close_result_screen();
void init_small_font();

int init_result_screen_images(SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load("assets/images/result_background.png");
    result_background_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/exit_button.png");
    exit_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/tohome_button.png");
    tohome_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/result.png");
    result_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return 1;
}

// 작은 폰트 초기화 함수
void init_small_font()
{
    small_font = TTF_OpenFont("assets/fonts/font.ttf", 45); // 45 사이즈로 폰트 열기
    if (!small_font)
    {
        printf("Failed to load small font: %s\n", TTF_GetError());
        exit(1);
    }
}

// 방 번호에 맞는 room_n.txt 파일을 읽고 순위를 출력
void display_leaderboard(SDL_Renderer *renderer)
{
    FILE *file;
    char filename[30];
    snprintf(filename, sizeof(filename), "running_game/result_%d.txt", current_room_num);

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open leaderboard file: %s\n", filename);
        return;
    }

    char nickname_in_file[100];
    int score;

    // 리더보드 텍스트 출력 시작 위치
    int x_pos = 330; // x 좌표
    int y_pos = 315; // y 좌표 (시작 위치)

    while (fscanf(file, "%s %d", nickname_in_file, &score) != EOF) {
        char leaderboard_entry[128];
        printf("%s - %d points\n", nickname_in_file, score);
        snprintf(leaderboard_entry, sizeof(leaderboard_entry), "%-10s %5d P", nickname_in_file, score);

        SDL_Color text_color = {0, 0, 0, 255}; // 검정색
        SDL_Surface *text_surface = TTF_RenderText_Blended(small_font, leaderboard_entry, text_color); // 작은 폰트 사용
        if (!text_surface) {
            printf("TTF_RenderText_Blended failed: %s\n", TTF_GetError());
            continue;
        }
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

        SDL_Rect text_rect = {x_pos, y_pos, text_surface->w, text_surface->h};
        SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);


        y_pos += text_surface->h + 80; // 다음 줄로 이동
        SDL_RenderPresent(renderer);

        
        SDL_FreeSurface(text_surface);
        SDL_DestroyTexture(text_texture);
    }

    fclose(file);
    
}

void close_result_screen(){
    SDL_DestroyTexture(result_texture);
    SDL_DestroyTexture(exit_button_texture);
    SDL_DestroyTexture(tohome_button_texture);
    SDL_DestroyTexture(result_background_texture);
    if (small_font)
        TTF_CloseFont(small_font);
}

void render_result_screen(SDL_Renderer *renderer, TTF_Font *font)
{
    if (!init_result_screen_images(renderer))
    {
        return;
    }

    init_small_font();

    SDL_RenderCopy(renderer, result_background_texture, NULL, NULL);

    SDL_Rect result_rect = {162, 83, 658, 846};
    SDL_RenderCopy(renderer, result_texture, NULL, &result_rect);

    SDL_Rect exit_button_rect = {953, 568, 325, 133};
    SDL_RenderCopy(renderer, exit_button_texture, NULL, &exit_button_rect);

    SDL_Rect tohome_button_rect = {953, 362, 325, 133};
    SDL_RenderCopy(renderer, tohome_button_texture, NULL, &tohome_button_rect);

    display_leaderboard(renderer);
    SDL_RenderPresent(renderer);

    render_update_needed = 0;

    SDL_Event event;
    while(1){
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                return;

            case SDL_MOUSEBUTTONDOWN:
                int x = event.button.x;
                int y = event.button.y;
                if (x >= exit_button_rect.x && x <= exit_button_rect.x + exit_button_rect.w && y >= exit_button_rect.y && y <= exit_button_rect.y + exit_button_rect.h) {
                    send_command("EXIT");
                    return;
                }

                if (x >= tohome_button_rect.x && x <= tohome_button_rect.x + tohome_button_rect.w && y >= tohome_button_rect.y && y <= tohome_button_rect.y + tohome_button_rect.h) {
                    send_command("GO_TO_HOME");
                    current_state = HOME_SCREEN;
                    render_update_needed = 1;
                    return;
                }
            }
        }
    }
    return;
}

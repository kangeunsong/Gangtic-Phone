#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "client.h"

#define BLACK 0, 0, 0, 255
extern TTF_Font *font;

SDL_Texture *setting_background_texture = NULL;
SDL_Texture *setting_title_texture = NULL;
SDL_Texture *back_texture = NULL;
SDL_Texture *enter_nick_texture = NULL;
SDL_Texture *ok_button_texture=NULL;
SDL_Texture *setting_white_back_texture = NULL;
Mix_Music *background_music = NULL; // 배경음악 데이터
int music_on = 1; // 음악 상태 (1: ON, 0: OFF)
SDL_Texture *volume_texture=NULL;

void init_music()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer 초기화 실패: %s\n", Mix_GetError());
        return;
    }

    // 배경음악 로드
    background_music = Mix_LoadMUS("assets/audio/background_music.mp3");
    if (!background_music) {
        printf("배경음악 로드 실패: %s\n", Mix_GetError());
        return;
    }

    Mix_VolumeMusic(MIX_MAX_VOLUME / 4); // 음량을 중간으로 설정

    // 배경음악 재생
    Mix_PlayMusic(background_music, -1); // -1: 무한 반복
}

void toggle_music()
{
    if (music_on) {
        Mix_PauseMusic();
        music_on = 0;
    } else {
        Mix_ResumeMusic(); 
        music_on = 1;
    }
}

void handle_music_button_click(int x, int y)
{
    SDL_Rect music_button_rect = {660, 500, 200, 90};
    if (x >= music_button_rect.x && x <= music_button_rect.x + music_button_rect.w &&
        y >= music_button_rect.y && y <= music_button_rect.y + music_button_rect.h) {
        toggle_music(); // 음악 상태 변경
    }
}

void close_music()
{
    Mix_FreeMusic(background_music);
    background_music = NULL;
    Mix_CloseAudio();
}

// 설정 화면에 필요한 이미지 로드 함수
int init_setting_screen_images(SDL_Renderer *renderer)
{
    // 설정 배경 로드
    SDL_Surface *surface = IMG_Load("assets/images/setting_background.png");
    setting_background_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/white_back.png");
    setting_white_back_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // 설정 타이틀 로드
    surface = IMG_Load("assets/images/setting_title.png");
    setting_title_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // 뒤로가기 버튼 로드
    surface = IMG_Load("assets/images/back.png");
    back_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // "닉네임 입력" 로드
    surface = IMG_Load("assets/images/enter_nick.png");
    enter_nick_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // "ok 버튼" 로드
    surface = IMG_Load("assets/images/ok_button.png");
    ok_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // "볼륨 이미지" 로드
    surface = IMG_Load("assets/images/volume.png");
    volume_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return 1; // 모든 이미지 로드 성공 시
}

void close_setting_screen(){
    SDL_DestroyTexture(setting_background_texture);
    SDL_DestroyTexture(setting_title_texture);
    SDL_DestroyTexture(back_texture);
    SDL_DestroyTexture(enter_nick_texture);
    SDL_DestroyTexture(ok_button_texture);
    SDL_DestroyTexture(volume_texture);
    close_music(); 
}

// 홈 화면 렌더링 함수
void render_setting_screen(SDL_Renderer *renderer)
{
    if (!init_setting_screen_images(renderer))
    {
        printf("홈 화면 이미지 로드 실패\n");
        return;
    }
    

    // 배경 이미지 렌더링
    SDL_RenderCopy(renderer, setting_background_texture, NULL, NULL);

    // 설정 타이틀 렌더링
    SDL_Rect setting_title_rect = {530, 165, 424, 109};
    SDL_RenderCopy(renderer, setting_title_texture, NULL, &setting_title_rect);

    SDL_Rect white_back_rect = {73, 80, 179, 58};
    SDL_RenderCopy(renderer, setting_white_back_texture, NULL, &white_back_rect);


    // 버튼 렌더링
    SDL_RenderCopy(renderer, setting_white_back_texture, NULL, &white_back_rect);

    // 볼륨이미지 렌더링
    SDL_Rect volume_rect={545,460,167,167};
    SDL_RenderCopy(renderer, volume_texture, NULL, &volume_rect);

    
    

    SDL_Event event;
    int running = 1;

    while(running){
        SDL_Rect music_button_rect = {720, 500, 200, 90}; // "음악 켜기/끄기" 버튼
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // 버튼 색상
        SDL_RenderFillRect(renderer, &music_button_rect);
         // 텍스트 중앙 정렬 계산
        SDL_Color text_color = {0, 0, 0, 255};
        SDL_Surface *text_surface = TTF_RenderText_Blended(font, music_on ? "OFF" : "ON", text_color);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        int text_x = music_button_rect.x + (music_button_rect.w - text_surface->w) / 2;
        int text_y = music_button_rect.y + (music_button_rect.h - text_surface->h) / 2;
    
        
        
        SDL_Rect text_rect = {text_x, text_y, text_surface->w, text_surface->h};
        SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

        SDL_FreeSurface(text_surface);
        SDL_DestroyTexture(text_texture);
        SDL_RenderPresent(renderer); // 화면 업데이트

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = 0;
                break;

            case SDL_MOUSEBUTTONDOWN:
                int x = event.button.x;
                int y = event.button.y;
                
                // "뒤로가기" 버튼 클릭 확인
                SDL_Rect white_back_rect = {73, 80, 179, 58};
                if (x >= white_back_rect.x && x <= white_back_rect.x + white_back_rect.w && y >= white_back_rect.y && y <= white_back_rect.y + white_back_rect.h)
                {
                    current_state = HOME_SCREEN;
                    return;
                }
                handle_music_button_click(x, y); // 음악 버튼 클릭 처리
                break;
            }
        }
    }
}
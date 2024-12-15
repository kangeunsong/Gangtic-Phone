#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL_mixer.h>
#include "client.h"
#include "waiting.h"

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define LINE_THICKNESS 5
#define MAXLINE 1000
#define ANSWER_NUM 20

int init_game_screen_images(SDL_Renderer *renderer);
void drawing_loop(SDL_Renderer *renderer);
void draw_thick_line(int x1, int y1, int x2, int y2, SDL_Renderer *renderer);
void close_game_screen();

SDL_Texture *round_texture = NULL;
SDL_Texture *total_round_texture = NULL;
SDL_Texture *round1_texture = NULL;
SDL_Texture *round2_texture = NULL;
SDL_Texture *round3_texture = NULL;
SDL_Texture *round4_texture = NULL;
SDL_Texture *round5_texture = NULL;
SDL_Texture *score_texture = NULL;
SDL_Texture *game_mountain_background_texture = NULL;
SDL_Texture *sketchbook_texture = NULL;
SDL_Texture *right_answer_texture = NULL;
SDL_Texture *wrong_answer_texture = NULL;
SDL_Texture *other_correct_texture = NULL;
SDL_Texture *draw_this_texture = NULL;

SDL_Texture *clock1_texture = NULL;
static double rotation_angle = 0.0;
static Uint32 last_frame_time = 0;

TTF_Font *small_font = NULL;

int prev_x = -1, prev_y = -1;

Mix_Chunk *correct_sound = NULL;
Mix_Chunk *wrong_sound = NULL;

int current_round = 0;

void init_correct_sound() {
    correct_sound = Mix_LoadWAV("assets/audio/correct_music.mp3");
    if (!correct_sound) {
        printf("Failed to load correct sound: %s\n", Mix_GetError());
    }
}
void init_wrong_sound() {
    wrong_sound = Mix_LoadWAV("assets/audio/wrong_music.mp3");
    if (!wrong_sound) {
        printf("Failed to load wrong sound: %s\n", Mix_GetError());
    }
}

void init_small_font()
{
    small_font = TTF_OpenFont("assets/fonts/font.ttf", 45); // 45 사이즈로 폰트 열기
    if (!small_font)
    {
        printf("Failed to load small font: %s\n", TTF_GetError());
        exit(1);
    }
}

void render_clock(SDL_Renderer *renderer) {
    SDL_Rect clock1_rect = {1440 - 73 - 180, 160, 180, 180};

    Uint32 current_time = SDL_GetTicks();
    // 애니메이션 업데이트 (1초마다 회전)
    if (current_time > last_frame_time + 100) {
        rotation_angle += 45.0;
        if (rotation_angle >= 360.0) {
            rotation_angle -= 360.0;
        }
        last_frame_time = current_time;
    }

    if (clock1_texture) {
        SDL_RenderCopyEx(renderer, clock1_texture, NULL, &clock1_rect, rotation_angle, NULL, SDL_FLIP_NONE);
    }
    else{
        printf("Failed to load clock texture.\n");
    }
}

int init_game_screen_images(SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load("assets/images/round.png");
    round_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/total_round.png");
    total_round_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/round1.png");
    round1_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/round2.png");
    round2_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/round3.png");
    round3_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/round4.png");
    round4_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/round5.png");
    round5_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/score.png");
    score_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/sketchbook.png");
    sketchbook_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/mountain_background.png");
    game_mountain_background_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/clock1.png");
    clock1_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/right_answer.png");
    right_answer_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/wrong_answer.png");
    wrong_answer_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    surface = IMG_Load("assets/images/other_correct.png");
    other_correct_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    surface = IMG_Load("assets/images/draw_this.png");
    draw_this_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return 1;
}

void check_round(SDL_Renderer *renderer){
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "CHECK_ROUND_%d\n", (current_room_num));
    send(sockfd, buffer, strlen(buffer), 0);

    char round_buffer[MAXLINE];
    int nbyte = recv(sockfd, round_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        round_buffer[nbyte] = '\0';
        if (strncmp(round_buffer, "ROUND_", 6) == 0) {
            current_round = atoi(&round_buffer[6]);

            SDL_Rect background_portion = {570, 85, 100, 138};
            SDL_Rect src_rect = {570, 85, 100, 138};
            SDL_RenderCopy(renderer, game_mountain_background_texture, &src_rect, &background_portion);
            SDL_RenderPresent(renderer);

            if(current_round == 1){
                SDL_Rect round_1_rect = {603, 83, 67, 140};
                SDL_RenderCopy(renderer, round1_texture, NULL, &round_1_rect);
            }
            else if(current_round == 2){
                SDL_Rect round_2_rect = {587, 79, 83, 151};
                SDL_RenderCopy(renderer, round2_texture, NULL, &round_2_rect);
            }
            else if(current_round == 3){
                SDL_Rect round_3_rect = {585, 87, 85, 136};
                SDL_RenderCopy(renderer, round3_texture, NULL, &round_3_rect);
            }
            else if(current_round == 4){
                SDL_Rect round_4_rect = {578, 86, 92, 137};
                SDL_RenderCopy(renderer, round4_texture, NULL, &round_4_rect);
            }
            else if(current_round == 5){
                SDL_Rect round_5_rect = {585, 84, 84, 138};
                SDL_RenderCopy(renderer, round5_texture, NULL, &round_5_rect);
            }
            return;
        }
        else { 
            printf("Can't get the round!\n");
            return;
        }
    }
}

// 출제자가 그림 그리는 루프
void drawing_loop(SDL_Renderer *renderer)
{
    check_round(renderer);
    if(current_round > 1){
        SDL_Rect right_answer_rect = {1026, 771, 269, 96};
        SDL_RenderCopy(renderer, right_answer_texture, NULL, &right_answer_rect);
        SDL_RenderPresent(renderer);

        Uint32 start_time = SDL_GetTicks();
        while (SDL_GetTicks() - start_time < 1000) {
            SDL_Delay(16);
        }
        SDL_Rect background_portion = {1026, 771, 269, 96};
        SDL_Rect src_rect = {1026, 771, 269, 96};
        SDL_RenderCopy(renderer, game_mountain_background_texture, &src_rect, &background_portion);
        SDL_RenderPresent(renderer);
    }

    SDL_Rect text_input_area = {937, 898, 447, 79}; // 정답입력칸 덮어씌움
    SDL_RenderCopy(renderer, game_mountain_background_texture, &text_input_area, &text_input_area);
    SDL_Rect draw_this_rect = {937, 400, 447, 193}; // 출제할 문제 정답 칸
    SDL_RenderCopy(renderer, draw_this_texture, NULL, &draw_this_rect);
    SDL_RenderPresent(renderer);

    char* answer = NULL;
    SDL_Event event;
    SDL_Rect canvas_rect = {100, 360, 690, 540};
    int drawing = 0;

    const char *filename = "data/answers.txt";
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Unable to open answers.txt\n");
        return;
    }
    
    char words[ANSWER_NUM][100];
    int word_count = 0;

    while (word_count < ANSWER_NUM && fgets(words[word_count], sizeof(words[word_count]), file)) {
        words[word_count][strcspn(words[word_count], "\n")] = 0;
        word_count++;
    }
    fclose(file);

    if (word_count == 0) {
        return;
    }

    srand(time(NULL));
    int random_index = rand() % word_count;
    answer = words[random_index];

    // 정답 화면에 표시
    int x_pos = 1110;
    int y_pos = 460;
    SDL_Color text_color = {0, 0, 0, 255};
    SDL_Surface *text_surface = TTF_RenderText_Blended(small_font, answer, text_color);
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

    SDL_Rect text_rect = {x_pos, y_pos, text_surface->w, text_surface->h};
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

    printf("Draw this word!: %s\n", answer);
    char answer_command[MAXLINE];
    snprintf(answer_command, sizeof(answer_command), "SET_ANSWER_%d_%s\n", current_room_num, answer);
    send(sockfd, answer_command, strlen(answer_command), 0);


    int stop_drawing = 0;
    while (!stop_drawing) {
        render_clock(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        char buffer[MAXLINE];
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            int nbyte = recv(sockfd, buffer, MAXLINE, 0);
            if (nbyte > 0) {
                buffer[nbyte] = '\0';                
                if (strcmp(buffer, "STOP_DRAWING") == 0) {
                    printf("Received STOP_DRAWING from server.\n");
                    stop_drawing = 1;
                    current_state = GAME_PLAYER_SCREEN;      
                    render_update_needed = 1;

                    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
                    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
                    SDL_FreeSurface(text_surface);
                    return;
                }
                else if (strcmp(buffer, "GAME_OVER") == 0) {
                    stop_drawing = 1;
                    current_state = RESULT_SCREEN;       
                    render_update_needed = 1;
                    return;
                }
            }
        }

        while (SDL_PollEvent(&event)) {
            int x = event.button.x;
            int y = event.button.y;
            switch (event.type)
            {
            case SDL_QUIT:
                stop_drawing = 1;
                break;
                
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                if (x >= canvas_rect.x && x <= canvas_rect.x + canvas_rect.w && 
                    y >= canvas_rect.y && y <= canvas_rect.y + canvas_rect.h) {
                    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                        drawing = 1;
                        prev_x = event.button.x;
                        prev_y = event.button.y;
                    }
                    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                        drawing = 0;
                        prev_x = -1;
                        prev_y = -1;
                    }
                    else if (event.type == SDL_MOUSEMOTION && drawing) {
                        if (prev_x != -1 && prev_y != -1) {
                            SDL_SetRenderDrawColor(renderer, BLACK);
                            draw_thick_line(prev_x, prev_y, event.motion.x, event.motion.y, renderer);
                        }
                        prev_x = event.motion.x;
                        prev_y = event.motion.y;
                    }
                }
                else {
                    if (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
                        drawing = 0;
                    }
                }
                break;
            }
        }
    }
    return;
}

// 화면에 그림을 그리는 함수
void draw_thick_line(int x1, int y1, int x2, int y2, SDL_Renderer *renderer)
{
    char draw_cmd[MAXLINE];
    snprintf(draw_cmd, MAXLINE, "SEND_DRAWING_%d_%d_%d_%d_%d\n", current_room_num, x1, y1, x2, y2);
    send(sockfd, draw_cmd, strlen(draw_cmd), 0);

    for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++)  
    {
        SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);
        SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);
    }
    SDL_RenderPresent(renderer);
}

// 출제자를 제외한 나머지 플레이어들이 정답을 맞추는 루프
void getting_answer_loop(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_Rect text_box_rect = {937, 898, 447, 79}; // 텍스트박스 위치와 크기
    SDL_Color box_color = {200, 200, 200, 255};   // 텍스트박스 색상
    SDL_Color text_color = {0, 0, 0, 255};        // 텍스트 색상
    char input_buffer[100] = "";
    int cursor_visible = 1;
    Uint32 last_blink_time = SDL_GetTicks();

    SDL_Event event;
    int stop_answering = 0;
    SDL_StartTextInput();
    check_round(renderer);

    SDL_Rect answer_area = {937, 400, 447, 193}; // 정답입력칸 덮어씌움
    SDL_RenderCopy(renderer, game_mountain_background_texture, &answer_area, &answer_area);
    SDL_RenderPresent(renderer);

    while (!stop_answering) {
        render_clock(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);

        SDL_SetRenderDrawColor(renderer, box_color.r, box_color.g, box_color.b, box_color.a);
        SDL_RenderFillRect(renderer, &text_box_rect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &text_box_rect);

        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        char buffer[MAXLINE];
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            int nbyte = recv(sockfd, buffer, MAXLINE, 0);
            if (nbyte > 0) {
                buffer[nbyte] = '\0';
                
                if (strcmp(buffer, "NEXT_ROUND") == 0) {
                    check_round(renderer);

                    SDL_Rect other_correct_rect = {937, 702, 447, 165};
                    SDL_RenderCopy(renderer, other_correct_texture, NULL, &other_correct_rect);
                    SDL_RenderPresent(renderer);
                    Uint32 start_time = SDL_GetTicks();
                    while (SDL_GetTicks() - start_time < 1000) {
                        SDL_Delay(16);
                    }

                    SDL_Rect background_portion = {937, 702, 447, 165};
                    SDL_Rect src_rect = {937, 702, 447, 165};
                    SDL_RenderCopy(renderer, game_mountain_background_texture, &src_rect, &background_portion);
                    SDL_RenderPresent(renderer);

                    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
                    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
                }
                else if(strcmp(buffer, "GAME_OVER") == 0) {
                    stop_answering = 1;
                    current_state = RESULT_SCREEN;       
                    render_update_needed = 1;
                    return;
                }
                else if (strncmp(buffer, "DRAW_", 5) == 0) {
                    char *msg = buffer;
                    char *next;
                    
                    while ((next = strstr(msg, "DRAW_")) != NULL) {
                        int x1, y1, x2, y2;
                        sscanf(next + 5, "%d_%d_%d_%d", &x1, &y1, &x2, &y2);
                        
                        SDL_SetRenderDrawColor(renderer, BLACK);
                        for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++) {
                            SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);
                            SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);
                        }
                        SDL_RenderPresent(renderer);
                        msg = next + 5;
                    }
                }
            }
        }

        if (strlen(input_buffer) > 0)
        {
            SDL_Surface *text_surface = TTF_RenderText_Blended(font, input_buffer, text_color);
            if (text_surface)
            {
                SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
                SDL_Rect text_rect = {text_box_rect.x + 10, text_box_rect.y + (text_box_rect.h - text_surface->h) / 2, text_surface->w, text_surface->h};
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

                SDL_FreeSurface(text_surface);
                SDL_DestroyTexture(text_texture);
            }
        }

        Uint32 current_time = SDL_GetTicks();
        if (current_time > last_blink_time + 500)
        {
            cursor_visible = !cursor_visible;
            last_blink_time = current_time;
        }
        if (cursor_visible)
        {
            int text_width = 0, text_height = 0;
            TTF_SizeText(font, input_buffer, &text_width, &text_height);
            SDL_Rect cursor_rect = {text_box_rect.x + 10 + text_width, text_box_rect.y + 5, 2, text_box_rect.h - 10};
            SDL_SetRenderDrawColor(renderer, text_color.r, text_color.g, text_color.b, text_color.a);
            SDL_RenderFillRect(renderer, &cursor_rect);
        }

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_QUIT:
                stop_answering = 1;  
                break;

            case SDL_TEXTINPUT:
                if (strlen(input_buffer) < sizeof(input_buffer) - 1)
                {
                    strcat(input_buffer, event.text.text);
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(input_buffer) > 0)
                {
                    input_buffer[strlen(input_buffer) - 1] = '\0';
                }
                else if (event.key.keysym.sym == SDLK_RETURN)
                {
                    input_buffer[strcspn(input_buffer, "\n")] = '\0';
                    char send_buffer[MAXLINE];
                    snprintf(send_buffer, sizeof(send_buffer), "RECV_ANSWER_%d_%s", current_room_num, input_buffer);
                    send(sockfd, send_buffer, strlen(send_buffer), 0);

                    char recv_buffer[MAXLINE];
                    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
                    recv_buffer[nbyte] = '\0';
                    if (strcmp(recv_buffer, "RIGHT") == 0) // 정답을 맞추면
                    {
                        if (correct_sound)
                        {
                            Mix_PlayChannel(-1, correct_sound, 0);
                        }
                        char G_buffer[MAXLINE];
                        snprintf(G_buffer, sizeof(G_buffer), "GET_CORRECT_%d_%d\n", current_room_num, sockID);
                        send(sockfd, G_buffer, strlen(G_buffer), 0);

                        if(current_round == 5){ // 마지막 라운드 정답 시 결과 화면으로
                            stop_answering = 1;
                            current_state = RESULT_SCREEN;       
                            render_update_needed = 1;
                            return;
                        }
                        current_state = GAME_PAINTER_SCREEN;
                        stop_answering = 1;
                    }
                    else if (strcmp(recv_buffer, "WRONG") == 0) // 틀리면
                    {
                        if (wrong_sound)
                        {
                            Mix_PlayChannel(-1, wrong_sound, 0);
                        }

                        SDL_Rect wrong_answer_rect = {1026, 771, 269, 96};
                        SDL_RenderCopy(renderer, wrong_answer_texture, NULL, &wrong_answer_rect);
                        SDL_RenderPresent(renderer);

                        Uint32 start_time = SDL_GetTicks();
                        while (SDL_GetTicks() - start_time < 1000) {
                            SDL_Delay(16);
                        }
                        SDL_Rect background_portion = {1026, 771, 269, 96}; 
                        SDL_Rect src_rect = {1026, 771, 269, 96};
                        SDL_RenderCopy(renderer, game_mountain_background_texture, &src_rect, &background_portion);
                        SDL_RenderPresent(renderer);
                    }
                }
                break;
            }
        }
    }
    SDL_StopTextInput(); // 텍스트 입력 비활성화(출제자는 입력 안되게)
    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
    return;
}

void close_game_screen(){
    if (correct_sound) {
        Mix_FreeChunk(correct_sound);
        correct_sound = NULL;
    }
    if (wrong_sound) {
        Mix_FreeChunk(wrong_sound);
        wrong_sound = NULL;
    }

    SDL_DestroyTexture(round_texture);
    SDL_DestroyTexture(total_round_texture);
    SDL_DestroyTexture(round1_texture);
    SDL_DestroyTexture(round2_texture);
    SDL_DestroyTexture(round3_texture);
    SDL_DestroyTexture(round4_texture);
    SDL_DestroyTexture(round5_texture);
    SDL_DestroyTexture(score_texture);
    SDL_DestroyTexture(game_mountain_background_texture);
    SDL_DestroyTexture(sketchbook_texture);
    SDL_DestroyTexture(clock1_texture);
    SDL_DestroyTexture(right_answer_texture);
    SDL_DestroyTexture(wrong_answer_texture);
    SDL_DestroyTexture(other_correct_texture);
    if (small_font)
        TTF_CloseFont(small_font);
}

void render_game_screen(SDL_Renderer *renderer, TTF_Font *font)
{
    if (!init_game_screen_images(renderer))
    {
        return;
    }
    init_small_font();

    SDL_RenderCopy(renderer, game_mountain_background_texture, NULL, NULL);

    SDL_Rect round_title_rect = {100, 89, 460, 141};
    SDL_RenderCopy(renderer, round_texture, NULL, &round_title_rect);

    SDL_Rect total_round_rect = {680, 78, 171, 153};
    SDL_RenderCopy(renderer, total_round_texture, NULL, &total_round_rect);

    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);

    SDL_Rect text_box_rect = {937, 898, 447, 79}; // 텍스트박스 위치와 크기
    SDL_Color box_color = {200, 200, 200, 255};   // 텍스트박스 색상
    SDL_Color text_color = {0, 0, 0, 255};        // 텍스트 색상
    char input_buffer[100] = "";                  // 텍스트 입력 저장
    int cursor_visible = 1;                       // 커서 깜박임
    Uint32 last_blink_time = SDL_GetTicks();      // 커서 깜박임 타이머

    SDL_RenderPresent(renderer);

    SDL_Event event;
    int running = 1;

    while(running){
        // 텍스트박스 렌더링
        SDL_SetRenderDrawColor(renderer, box_color.r, box_color.g, box_color.b, box_color.a);
        SDL_RenderFillRect(renderer, &text_box_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &text_box_rect);

        // 입력된 텍스트 렌더링
        if (strlen(input_buffer) > 0) // 텍스트 버퍼가 비어 있지 않은 경우에만 렌더링
        {
            SDL_Surface *text1_surface = TTF_RenderText_Blended(font, input_buffer, text_color);
            if (!text1_surface)
            {
                printf("Failed to create text surface: %s\n", TTF_GetError());
            }
            else
            {
                SDL_Texture *text1_texture = SDL_CreateTextureFromSurface(renderer, text1_surface);
                SDL_Rect text1_rect = {text_box_rect.x + 10, text_box_rect.y + (text_box_rect.h - text1_surface->h) / 2, text1_surface->w, text1_surface->h};
                SDL_RenderCopy(renderer, text1_texture, NULL, &text1_rect);

                SDL_FreeSurface(text1_surface);
                SDL_DestroyTexture(text1_texture);
            }
        }

        // 커서 깜박임
        Uint32 current_time = SDL_GetTicks();
        if (current_time > last_blink_time + 500)
        {
            cursor_visible = !cursor_visible;
            last_blink_time = current_time;
        }
        if (cursor_visible)
        {
            SDL_Rect cursor_rect = {text_box_rect.x + 10 + (strlen(input_buffer) > 0 ? TTF_SizeText(font, input_buffer, NULL, NULL) : 0), text_box_rect.y + 5, 2, text_box_rect.h - 10};
            SDL_SetRenderDrawColor(renderer, text_color.r, text_color.g, text_color.b, text_color.a);
            SDL_RenderFillRect(renderer, &cursor_rect);
        }
        SDL_RenderPresent(renderer);
        if(current_state == GAME_PAINTER_SCREEN){
            drawing_loop(renderer);
        }

        else if (current_state == GAME_PLAYER_SCREEN){
            getting_answer_loop(renderer, font);
        }

        else if (current_state == RESULT_SCREEN){
            current_state = RESULT_SCREEN;       
            render_update_needed = 1;
            running = 0;
        }

        while (SDL_PollEvent(&event))
        {   
            switch (event.type)
            {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_TEXTINPUT:
                if (strlen(input_buffer) < sizeof(input_buffer) - 1)
                {
                    strcat(input_buffer, event.text.text); // 텍스트 추가
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(input_buffer) > 0)
                {
                    input_buffer[strlen(input_buffer) - 1] = '\0'; // 백스페이스 처리
                }
                else if (event.key.keysym.sym == SDLK_RETURN)
                {
                    running = 0; // 텍스트 입력 종료
                }
                break;
            }
        }
    }
    return;
}
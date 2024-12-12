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
#define ANSWER_NUM 10

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
SDL_Texture *white_back_texture = NULL;
SDL_Texture *game_mountain_background_texture = NULL;
SDL_Texture *sketchbook_texture = NULL;
// 시계 이미지 텍스처와 회전 각도 변수
SDL_Texture *clock1_texture = NULL; // 시계 텍스처
static double rotation_angle = 0.0; // 현재 회전 각도
static Uint32 last_frame_time = 0;  // 마지막 프레임 갱신 시간

int prev_x = -1, prev_y = -1;

Mix_Chunk *correct_sound = NULL; // 정답 효과음
Mix_Chunk *wrong_sound = NULL; // 오답 효과음

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

// 시계 렌더링 함수
void render_clock(SDL_Renderer *renderer) {
    SDL_Rect clock1_rect = {1440 - 73 - 180, 160, 180, 180}; // 시계 위치 및 크기

    Uint32 current_time = SDL_GetTicks();
    // 애니메이션 업데이트 (1초마다 회전)
    if (current_time > last_frame_time + 100) {
        rotation_angle += 45.0; // 1초마다 45도 회전
        if (rotation_angle >= 360.0) {
            rotation_angle -= 360.0; // 360도를 초과하면 리셋
        }
        last_frame_time = current_time;
    }

    // 텍스처 회전 렌더링
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

    surface = IMG_Load("assets/images/white_back.png");
    white_back_texture = SDL_CreateTextureFromSurface(renderer, surface);
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

    return 1; // 모든 이미지 로드 성공 시
}

// 그림판 기능
void drawing_loop(SDL_Renderer *renderer)
{
    char set_painter_buffer[MAXLINE];
    snprintf(set_painter_buffer, sizeof(set_painter_buffer), "SET_PAINTER_%d_%d\n", current_room_num, sockID);
    send(sockfd, set_painter_buffer, strlen(set_painter_buffer), 0);
    printf("Send: %s\n", set_painter_buffer);

    char* answer = NULL;
    SDL_Event event;
    SDL_Rect canvas_rect = {100, 360, 690, 540};
    int drawing = 0;
    char buffer[MAXLINE];

    const char *filename = "data/answers.txt";
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Unable to open answers.txt\n");
        return;
    }

    char words[5][100];  // 5개의 단어를 저장할 배열

    // answer_num번째 줄의 단어 선택
    for (int i = 0; i <= answer_num; i++) {
        if (fgets(words[i], sizeof(words[i]), file)) {
            if (i == answer_num) {
                answer = words[answer_num];
                answer[strcspn(answer, "\n")] = 0;  // 개행문자 제거
                break;
            }
        }
    }
    fclose(file);

    // NULL 체크 추가
    if (answer == NULL) {
        return;
    }

    // 다음 라운드를 위해 answer_num 증가
    printf("Draw this word!: %s", answer);
    char answer_command[MAXLINE];
    snprintf(answer_command, sizeof(answer_command), "SET_ANSWER_%d_%s\n", current_room_num, answer);
    send(sockfd, answer_command, strlen(answer_command), 0);

    int stop_drawing = 0;
    static Uint32 last_check_time = 0;
    while (!stop_drawing) {
        render_clock(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
        
        // Non-blocking socket check
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms timeout

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            int nbyte = recv(sockfd, buffer, MAXLINE, 0);
            if (nbyte > 0) {
                buffer[nbyte] = '\0';
                printf("Received: %s\n", buffer);  // 디버깅을 위한 출력 추가
                
                if (strcmp(buffer, "STOP_DRAWING") == 0) {
                    printf("Received STOP_DRAWING from server.\n");
                    stop_drawing = 1;
                    current_state = GAME_PLAYER_SCREEN;
                    printf("current_state: GAME_PLAYER_SCREEN\n");         
                    render_update_needed = 1;
                    break;
                }
                else if (strcmp(buffer, "GAME_OVER") == 0) {
                    stop_drawing = 1;
                    printf("GAME_OVER\n");
                    current_state = RESULT_SCREEN;       
                    render_update_needed = 1;
                    return;
                }
            }
        }

        // CHECK_ROUND 메시지 주기적 전송
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_check_time > 1000) {  // 1초마다 체크
            char check_round_buffer[MAXLINE];
            snprintf(check_round_buffer, sizeof(check_round_buffer), "CHECK_ROUND_%d_%d", current_room_num, sockID);
            send(sockfd, check_round_buffer, strlen(check_round_buffer), 0);
            last_check_time = current_time;
        }

        // SDL Event handling
        while (SDL_PollEvent(&event)) {
            int x = event.button.x;
            int y = event.button.y;

            switch (event.type)
            {
            case SDL_QUIT:
                stop_drawing = 1;
                break;

            case SDL_MOUSEBUTTONDOWN:                
                // "뒤로가기" 버튼 클릭 확인
                SDL_Rect white_back_rect = {73, 80, 179, 58};
                if (x >= white_back_rect.x && x <= white_back_rect.x + white_back_rect.w && y >= white_back_rect.y && y <= white_back_rect.y + white_back_rect.h)
                {
                    printf("back button has been clicked.");
                    current_state = RESULT_SCREEN;
                    render_update_needed = 1;
                    return;
                }
                break;
            }

            if (x >= canvas_rect.x && x <= canvas_rect.x + canvas_rect.w && 
                y >= canvas_rect.y && y <= canvas_rect.y + canvas_rect.h) {
                switch (event.type) {
                    case SDL_MOUSEBUTTONDOWN:
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            drawing = 1;
                            prev_x = event.button.x;
                            prev_y = event.button.y;
                        }
                        break;
                    case SDL_MOUSEBUTTONUP:
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            drawing = 0;
                            prev_x = -1;
                            prev_y = -1;
                        }
                        break;
                    case SDL_MOUSEMOTION:
                        if (drawing) {
                            if (prev_x != -1 && prev_y != -1) {
                                SDL_SetRenderDrawColor(renderer, BLACK);
                                draw_thick_line(prev_x, prev_y, event.motion.x, event.motion.y, renderer);
                            }
                            prev_x = event.motion.x;
                            prev_y = event.motion.y;
                        }
                        break;
                }
            } else {
                if (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
                    drawing = 0;
                }
            }
        }
    }

    answer_num++;
    printf("It has been stopped.\n");
    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
    return;
}

void draw_thick_line(int x1, int y1, int x2, int y2, SDL_Renderer *renderer)
{
    char draw_cmd[MAXLINE];
    snprintf(draw_cmd, MAXLINE, "SEND_DRAWING_%d_%d_%d_%d_%d_%d\n", current_room_num, sockfd, x1, y1, x2, y2);
    send(sockfd, draw_cmd, strlen(draw_cmd), 0);

    for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++)  
    {
        // 두 점을 연결하는 선을 그립니다 (x1, y1)에서 (x2, y2)로
        SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);  // 수평선
        SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);  // 수직선
    }
    SDL_RenderPresent(renderer);  // 변경된 화면을 즉시 렌더링
}

// void getting_answer_loop(SDL_Renderer *renderer)
// {
//     SDL_Event event;
//     int stop_answering = 0;

//     while (!stop_answering) {
//         char check_round_buffer[MAXLINE];
//         snprintf(check_round_buffer, sizeof(check_round_buffer), "CHECK_ROUND_%d_%d", current_room_num, sockID);
//         send(sockfd, check_round_buffer, strlen(check_round_buffer), 0);
//         char tempbuffer[MAXLINE];
//         int tempnbyte = recv(sockfd, tempbuffer, MAXLINE, 0);
//         if (tempnbyte > 0) {
//             tempbuffer[tempnbyte] = '\0';
//             if (strcmp(tempbuffer, "GAME_OVER") == 0) {
//                 printf("GAME_OVER\n");
//                 current_state = RESULT_SCREEN;       
//                 render_update_needed = 1;
//                 return;
//             }
//         }

//         render_clock(renderer);
//         SDL_RenderPresent(renderer);
//         SDL_Delay(16);
//         char player_answer[100];
//         printf("enter answer: ");
//         scanf("%s", player_answer);
//         getchar();
//         printf("%d, %d, %s\n", current_room_num, sockID, player_answer);
//         char send_buffer[MAXLINE];
//         snprintf(send_buffer, sizeof(send_buffer), "RECV_ANSWER_%d_%s", current_room_num, player_answer);
//         printf("%s\n", send_buffer);
//         send(sockfd, send_buffer, strlen(send_buffer), 0);
        
//         char recv_buffer[MAXLINE];
//         int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
//         recv_buffer[nbyte] = '\0';

//         if (strcmp(recv_buffer, "RIGHT") == 0) {
//             if (correct_sound) {
//                 Mix_PlayChannel(-1, correct_sound, 0); // 효과음 재생 (한 번)
//             }
//             char G_buffer[MAXLINE];
//             snprintf(G_buffer, sizeof(G_buffer), "GET_CORRECT_%d_%d\n", current_room_num, sockID);
//             printf("send: %s", G_buffer);
//             send(sockfd, G_buffer, strlen("GET_CORRECT_N_N"), 0);
//             current_state = GAME_PAINTER_SCREEN;
//             printf("current_state: GAME_PAINTER_SCREEN\n");         
//             render_update_needed = 1;
//             stop_answering = 1;
//             break;
//         }

//         else if (strcmp(recv_buffer, "WRONG") == 0){
//             if (wrong_sound) {
//                 Mix_PlayChannel(-1, wrong_sound, 0); // 효과음 재생 (한 번)
//             }
//             printf("try again ...\n");
//         }

//         else if (strncmp(recv_buffer, "DRAW_", 5) == 0) {
//             char *msg = recv_buffer;
//             char *next;
            
//             while ((next = strstr(msg, "DRAW_")) != NULL) {
//                 int x1, y1, x2, y2;
//                 sscanf(next + 5, "%d_%d_%d_%d", &x1, &y1, &x2, &y2);
                
//                 SDL_SetRenderDrawColor(renderer, BLACK);
//                 for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++) {
//                     SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);
//                     SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);
//                 }
//                 SDL_RenderPresent(renderer);
                
//                 msg = next + 5;
//             }
//         }

//         while (SDL_PollEvent(&event))
//         {
//             switch (event.type)
//             {
//             case SDL_QUIT:
//                 stop_answering = 1;  
//                 break;

//             case SDL_MOUSEBUTTONDOWN:
//                 int x = event.button.x;
//                 int y = event.button.y;
                
//                 SDL_Rect white_back_rect = {73, 80, 179, 58};
//                 if (x >= white_back_rect.x && x <= white_back_rect.x + white_back_rect.w && y >= white_back_rect.y && y <= white_back_rect.y + white_back_rect.h)
//                 {
//                     printf("back button has been clicked.\n");
//                     current_state = RESULT_SCREEN;
//                     render_update_needed = 1;
//                     stop_answering = 1;  
//                     return;
//                 }
//                 break;
//             }
//         }
//     }

//     SDL_Rect sketchbook_rect = {70, 250, 750, 700};
//     SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
//     return;
// }

// void getting_answer_loop(SDL_Renderer *renderer, TTF_Font *font)
// {
//     SDL_Rect text_box_rect = {900, 830, 400, 60}; // 텍스트박스 위치와 크기
//     SDL_Color box_color = {200, 200, 200, 255};   // 텍스트박스 색상
//     SDL_Color text_color = {0, 0, 0, 255};        // 텍스트 색상
//     char input_buffer[100] = "";                  // 텍스트 입력 저장
//     int cursor_visible = 1;                       // 커서 깜박임
//     Uint32 last_blink_time = SDL_GetTicks();      // 커서 깜박임 타이머

//     SDL_Event event;
//     int stop_answering = 0;
//     static Uint32 last_check_time = 0;

//     SDL_StartTextInput();

//     while (!stop_answering) {
//         render_clock(renderer);
//         // SDL_RenderPresent(renderer);
//         // SDL_Delay(16);

//         SDL_SetRenderDrawColor(renderer, box_color.r, box_color.g, box_color.b, box_color.a);
//         SDL_RenderFillRect(renderer, &text_box_rect);

//         SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 테두리 색상
//         SDL_RenderDrawRect(renderer, &text_box_rect);

//         if (strlen(input_buffer) > 0)
//         {
//             SDL_Surface *text_surface = TTF_RenderText_Blended(font, input_buffer, text_color);
//             if (text_surface)
//             {
//                 SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
//                 SDL_Rect text_rect = {text_box_rect.x + 10, text_box_rect.y + (text_box_rect.h - text_surface->h) / 2, text_surface->w, text_surface->h};
//                 SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

//                 SDL_FreeSurface(text_surface);
//                 SDL_DestroyTexture(text_texture);
//             }
//         }

//         // CHECK_ROUND 메시지 주기적 전송
//         Uint32 current_time = SDL_GetTicks();
//         if (current_time - last_check_time > 1000) {  // 1초마다 체크
//             char check_round_buffer[MAXLINE];
//             snprintf(check_round_buffer, sizeof(check_round_buffer), "CHECK_ROUND_%d_%d", current_room_num, sockID);
//             send(sockfd, check_round_buffer, strlen(check_round_buffer), 0);
            
//             // Non-blocking socket check
//             fd_set readfds;
//             struct timeval timeout;
//             FD_ZERO(&readfds);
//             FD_SET(sockfd, &readfds);
            
//             timeout.tv_sec = 0;
//             timeout.tv_usec = 10000; // 10ms timeout
            
//             int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
//             if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
//                 char tempbuffer[MAXLINE];
//                 int tempnbyte = recv(sockfd, tempbuffer, MAXLINE, 0);
//                 if (tempnbyte > 0) {
//                     tempbuffer[tempnbyte] = '\0';
//                     printf("Received: %s\n", tempbuffer);
//                     if (strcmp(tempbuffer, "GAME_OVER") == 0) {
//                         printf("GAME_OVER\n");
//                         current_state = RESULT_SCREEN;       
//                         render_update_needed = 1;
//                         return;
//                     }
//                 }
//             }
//             last_check_time = current_time;
//         }
//         if (current_time > last_blink_time + 500)
//         {
//             cursor_visible = !cursor_visible;
//             last_blink_time = current_time;
//         }
//         if (cursor_visible)
//         {
//             int text_width = 0, text_height = 0;
//             TTF_SizeText(font, input_buffer, &text_width, &text_height);
//             SDL_Rect cursor_rect = {text_box_rect.x + 10 + text_width, text_box_rect.y + 5, 2, text_box_rect.h - 10};
//             SDL_SetRenderDrawColor(renderer, text_color.r, text_color.g, text_color.b, text_color.a);
//             SDL_RenderFillRect(renderer, &cursor_rect);
//         }

//         SDL_RenderPresent(renderer);

//         if(answer_num == 5){
//             current_state = RESULT_SCREEN;       
//             render_update_needed = 1;
//             return;
//         }

//         char recv_buffer[MAXLINE];
//         if (strncmp(recv_buffer, "DRAW_", 5) == 0) {
//             char *msg = recv_buffer;
//             char *next;
            
//             while ((next = strstr(msg, "DRAW_")) != NULL) {
//                 int x1, y1, x2, y2;
//                 sscanf(next + 5, "%d_%d_%d_%d", &x1, &y1, &x2, &y2);
                
//                 SDL_SetRenderDrawColor(renderer, BLACK);
//                 for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++) {
//                     SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);
//                     SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);
//                 }
//                 SDL_RenderPresent(renderer);
                
//                 msg = next + 5;
//             }
//         }

//         // SDL Event handling
//         while (SDL_PollEvent(&event)) {
//             switch (event.type)
//             {
//             case SDL_QUIT:
//                 stop_answering = 1;  
//                 break;

//             case SDL_TEXTINPUT:
//                 if (strlen(input_buffer) < sizeof(input_buffer) - 1)
//                 {
//                     strcat(input_buffer, event.text.text); // 텍스트 추가
//                 }
//                 break;

//             case SDL_KEYDOWN:
//                 if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(input_buffer) > 0)
//                 {
//                     input_buffer[strlen(input_buffer) - 1] = '\0'; // 백스페이스 처리
//                 }
//                 else if (event.key.keysym.sym == SDLK_RETURN)
//                 {
//                     input_buffer[strcspn(input_buffer, "\n")] = '\0';
//                     printf("입력된 텍스트: %s\n", input_buffer); // 엔터 입력 시 출력
//                     char send_buffer[MAXLINE];
//                     snprintf(send_buffer, sizeof(send_buffer), "RECV_ANSWER_%d_%s", current_room_num, input_buffer);
//                     send(sockfd, send_buffer, strlen(send_buffer), 0);

//                     // 서버 응답 수신
//                     char recv_buffer[MAXLINE];
//                     int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
//                     recv_buffer[nbyte] = '\0';
//                     printf("서버 응답: %s\n", recv_buffer);

//                     if (strcmp(recv_buffer, "RIGHT") == 0)
//                     {
//                         printf("right!\n");
//                         if (correct_sound)
//                         {
//                             Mix_PlayChannel(-1, correct_sound, 0); // 효과음 재생
//                         }
//                         char G_buffer[MAXLINE];
//                         snprintf(G_buffer, sizeof(G_buffer), "GET_CORRECT_%d_%d\n", current_room_num, sockID);
//                         printf("send: %s", G_buffer);
//                         send(sockfd, G_buffer, strlen(G_buffer), 0);
//                         printf("current_state: GAME_PAINTER_SCREEN\n"); 
//                         current_state = GAME_PAINTER_SCREEN;
//                         stop_answering = 1;
//                     }
//                     else if (strcmp(recv_buffer, "WRONG") == 0)
//                     {
//                         if (wrong_sound)
//                         {
//                             Mix_PlayChannel(-1, wrong_sound, 0); // 효과음 재생
//                         }
//                         printf("정답이 아닙니다. 다시 시도하세요.\n");
//                     }
//                 }
//                 break;
//             }
//         }
//     }

//     answer_num++;
//     SDL_StopTextInput(); // 텍스트 입력 비활성화
//     SDL_Rect sketchbook_rect = {70, 250, 750, 700};
//     SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
//     return;
// }

void getting_answer_loop(SDL_Renderer *renderer, TTF_Font *font)
{
   SDL_Rect text_box_rect = {900, 830, 400, 60}; // 텍스트박스 위치와 크기
   SDL_Color box_color = {200, 200, 200, 255};   // 텍스트박스 색상
   SDL_Color text_color = {0, 0, 0, 255};        // 텍스트 색상
   char input_buffer[100] = "";                  // 텍스트 입력 저장
   int cursor_visible = 1;                       // 커서 깜박임
   Uint32 last_blink_time = SDL_GetTicks();      // 커서 깜박임 타이머

   SDL_Event event;
   int stop_answering = 0;
   static Uint32 last_check_time = 0;

   SDL_StartTextInput();

   while (!stop_answering) {
       render_clock(renderer);

       // 텍스트 박스 그리기
       SDL_SetRenderDrawColor(renderer, box_color.r, box_color.g, box_color.b, box_color.a);
       SDL_RenderFillRect(renderer, &text_box_rect);

       SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 테두리 색상
       SDL_RenderDrawRect(renderer, &text_box_rect);

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

       // 서버로부터 그리기 명령 받기
       fd_set readfds;
       struct timeval timeout;
       FD_ZERO(&readfds);
       FD_SET(sockfd, &readfds);
       timeout.tv_sec = 0;
       timeout.tv_usec = 10000;

       int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
       if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
           char draw_buffer[MAXLINE];
           int nbyte = recv(sockfd, draw_buffer, MAXLINE, 0);
           if (nbyte > 0) {
               draw_buffer[nbyte] = '\0';
               printf("Received: %s\n", draw_buffer);
               
               if (strncmp(draw_buffer, "DRAW_", 5) == 0) {
                   int x1, y1, x2, y2;
                   sscanf(draw_buffer + 5, "%d_%d_%d_%d", &x1, &y1, &x2, &y2);
                   
                   SDL_SetRenderDrawColor(renderer, BLACK);
                   for (int w = -LINE_THICKNESS / 2; w <= LINE_THICKNESS / 2; w++) {
                       SDL_RenderDrawLine(renderer, x1 + w, y1, x2 + w, y2);
                       SDL_RenderDrawLine(renderer, x1, y1 + w, x2, y2 + w);
                   }
               }
               else if (strcmp(draw_buffer, "GAME_OVER") == 0) {
                   printf("GAME_OVER\n");
                   current_state = RESULT_SCREEN;       
                   render_update_needed = 1;
                   return;
               }
           }
       }

       // CHECK_ROUND 메시지 주기적 전송
       Uint32 current_time = SDL_GetTicks();
       if (current_time - last_check_time > 1000) {  // 1초마다 체크
           char check_round_buffer[MAXLINE];
           snprintf(check_round_buffer, sizeof(check_round_buffer), "CHECK_ROUND_%d_%d", current_room_num, sockID);
           send(sockfd, check_round_buffer, strlen(check_round_buffer), 0);
           last_check_time = current_time;
       }

       // 커서 깜박임 처리
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

       if(answer_num == 5){
           current_state = RESULT_SCREEN;       
           render_update_needed = 1;
           return;
       }

       // 이벤트 처리
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
                   printf("입력된 텍스트: %s\n", input_buffer);
                   char send_buffer[MAXLINE];
                   snprintf(send_buffer, sizeof(send_buffer), "RECV_ANSWER_%d_%s", current_room_num, input_buffer);
                   send(sockfd, send_buffer, strlen(send_buffer), 0);

                   // 서버 응답 수신
                   char recv_buffer[MAXLINE];
                   int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
                   recv_buffer[nbyte] = '\0';

                   if (strcmp(recv_buffer, "RIGHT") == 0)
                   {
                       if (correct_sound)
                       {
                           Mix_PlayChannel(-1, correct_sound, 0);
                       }
                       char G_buffer[MAXLINE];
                       snprintf(G_buffer, sizeof(G_buffer), "GET_CORRECT_%d_%d\n", current_room_num, sockID);
                       printf("send: %s", G_buffer);
                       send(sockfd, G_buffer, strlen(G_buffer), 0);
                       printf("current_state: GAME_PAINTER_SCREEN\n"); 
                       current_state = GAME_PAINTER_SCREEN;
                       stop_answering = 1;
                   }
                   else if (strcmp(recv_buffer, "WRONG") == 0)
                   {
                       if (wrong_sound)
                       {
                           Mix_PlayChannel(-1, wrong_sound, 0);
                       }
                       printf("정답이 아닙니다. 다시 시도하세요.\n");
                       memset(input_buffer, 0, sizeof(input_buffer));  // 입력 버퍼 초기화
                   }
               }
               break;

           case SDL_MOUSEBUTTONDOWN:
               int x = event.button.x;
               int y = event.button.y;
               
               SDL_Rect white_back_rect = {73, 80, 179, 58};
               if (x >= white_back_rect.x && x <= white_back_rect.x + white_back_rect.w &&
                   y >= white_back_rect.y && y <= white_back_rect.y + white_back_rect.h)
               {
                   printf("back button has been clicked.\n");
                   current_state = RESULT_SCREEN;
                   render_update_needed = 1;
                   return;
               }
               break;
           }
       }
       
       SDL_Delay(16);
   }

   answer_num++;
   SDL_StopTextInput();
   SDL_Rect sketchbook_rect = {70, 250, 750, 700};
   SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);
}

void close_game_screen(){
    if (correct_sound) {
        Mix_FreeChunk(correct_sound); // 효과음 자원 해제
        correct_sound = NULL;
    }
    if (wrong_sound) {
        Mix_FreeChunk(wrong_sound); // 효과음 자원 해제
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
    SDL_DestroyTexture(white_back_texture);
    SDL_DestroyTexture(game_mountain_background_texture);
    SDL_DestroyTexture(sketchbook_texture);
    SDL_DestroyTexture(clock1_texture);
}

void render_game_screen(SDL_Renderer *renderer, TTF_Font *font)
{
    if (!init_game_screen_images(renderer))
    {
        printf("게임 화면 이미지 로드 실패\n");
        return;
    }

    SDL_RenderCopy(renderer, game_mountain_background_texture, NULL, NULL);

    SDL_Rect round_title_rect = {100, 90, 460, 141};
    SDL_RenderCopy(renderer, round_texture, NULL, &round_title_rect);

    SDL_Rect round_1_rect = {600, 85, 65, 138};
    SDL_RenderCopy(renderer, round1_texture, NULL, &round_1_rect);

    SDL_Rect total_round_rect = {680, 84, 165, 139};
    SDL_RenderCopy(renderer, total_round_texture, NULL, &total_round_rect);

    SDL_Rect white_back_rect = {73, 50, 179, 58};
    SDL_RenderCopy(renderer, white_back_texture, NULL, &white_back_rect); // RESULT_SCREEN용 임시 버튼

    SDL_Rect sketchbook_rect = {70, 250, 750, 700};
    SDL_RenderCopy(renderer, sketchbook_texture, NULL, &sketchbook_rect);

    SDL_Rect text_box_rect = {900, 830, 400, 60}; // 텍스트박스 위치와 크기
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

        // 텍스트박스 테두리 렌더링
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &text_box_rect);

        
        SDL_Texture* current_round_texture = NULL;
        switch(answer_num + 1) {
            case 1:
                current_round_texture = round1_texture;
                break;
            case 2:
                current_round_texture = round2_texture;
                break;
            case 3:
                current_round_texture = round3_texture;
                break;
            case 4:
                current_round_texture = round4_texture;
                break;
            case 5:
                current_round_texture = round5_texture;
                break;
            default:
                current_round_texture = round1_texture;
        }
        
        SDL_Rect round_num_rect = {600, 85, 65, 138};
        SDL_RenderCopy(renderer, current_round_texture, NULL, &round_num_rect);

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

        // 커서 깜박임 구현
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
        //SDL_FreeSurface(text_surface);
        //SDL_DestroyTexture(text_texture);
        SDL_RenderPresent(renderer); // 화면 업데이트
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
            printf("game end\n");
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
                    printf("입력된 텍스트: %s\n", input_buffer); // 엔터 입력 시 출력
                    running = 0;                                   // 텍스트 입력 종료
                }
                break;
            }
        }
    }
}
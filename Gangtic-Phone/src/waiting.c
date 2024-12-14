#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

#define MAX_ROOMS 4
#define MAX_USERS_PER_ROOM 4
#define MAXLINE 1000

int init_waiting_room_images(SDL_Renderer *renderer);
void read_rooms_from_file();
void create_new_room(SDL_Renderer *renderer);
void remove_last_room(SDL_Renderer *renderer);
void render_screen(SDL_Renderer *renderer, int isRefresh);
int handle_room_status(SDL_Renderer *renderer);
void close_waiting_room();

SDL_Texture *waiting_title_texture = NULL;
SDL_Texture *waiting_background_texture = NULL;
SDL_Texture *create_button_texture = NULL;
SDL_Texture *refresh_button_texture = NULL;
SDL_Texture *room1_texture = NULL;
SDL_Texture *room2_texture = NULL;
SDL_Texture *room3_texture = NULL;
SDL_Texture *room4_texture = NULL;
SDL_Texture *waiting_white_back_texture = NULL;
SDL_Texture *x_button_texture = NULL;
SDL_Texture *red_wait_texture = NULL;
SDL_Texture *green_wait_texture = NULL;
SDL_Texture *room_limit_texture = NULL;
SDL_Texture *need_refresh_texture = NULL;
SDL_Texture *cannot_delete_texture = NULL;

int rooms_num = 0;
int current_room_num = -1;

// void print_rooms_status() {
//     printf("print_room_status\n");
//     for (int i = 0; i < room_num; i++) {
//         printf("Room %d\n", i + 1);
//         printf("Room ID: %s\n", rooms[i]);
//         printf("Number of users: %d\n", room_player_num[i]);
//         printf("\n\n");
//     }
// }

int init_waiting_room_images(SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load("assets/images/waiting_title.png");
    waiting_title_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/waiting_background.png");
    waiting_background_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/white_back.png");
    waiting_white_back_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/room1.png");
    room1_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/room2.png");
    room2_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/room3.png");
    room3_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/room4.png");
    room4_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/Create_button.png");
    create_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/Refresh_button.png");
    refresh_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/x_button.png");
    x_button_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/red_wait.png");
    red_wait_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/green_wait.png");
    green_wait_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/room_limit.png");
    room_limit_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/need_refresh.png");
    need_refresh_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("assets/images/cannot_delete.png");
    cannot_delete_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return 1; // 모든 이미지 로드 성공 시
}

void read_rooms_from_file()
{
    const char *filename = "data/rooms.txt";
    FILE *file = fopen(filename, "r");

    // 첫 번째 줄에서 방 개수 읽기
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file))
    {
        rooms_num = atoi(buffer);
    }
    fclose(file);
}

// 방 생성
void create_new_room(SDL_Renderer *renderer) {
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "CREATE_ROOM_%d\n", (rooms_num + 1));
    send(sockfd, buffer, strlen(buffer), 0);

    char recv_buffer[MAXLINE];
    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        recv_buffer[nbyte] = '\0';
        if (strcmp(recv_buffer, "POSSIBLE") == 0) {
            rooms_num++;
            render_screen(renderer, 1);
            return;
        }
        else if (strcmp(recv_buffer, "ALREADY_EXIST") == 0) {
            // 화면 새로고침 필요
            SDL_Rect need_refresh_rect = {353, 521, 733, 96};
            SDL_RenderCopy(renderer, need_refresh_texture, NULL, &need_refresh_rect);
            SDL_RenderPresent(renderer);
            return;
        }
    }
}

void remove_last_room(SDL_Renderer *renderer) {
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "DELETE_ROOM_%d\n", (rooms_num));
    send(sockfd, buffer, strlen(buffer), 0);

    char recv_buffer[MAXLINE];
    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        recv_buffer[nbyte] = '\0';
        if (strcmp(recv_buffer, "POSSIBLE") == 0) {
            rooms_num--;
            render_screen(renderer, 1);
            return;
        }
        else { // 방을 삭제할 수 없음
            SDL_Rect cannot_delete_rect = {215, 521, 1011, 96};
            SDL_RenderCopy(renderer, cannot_delete_texture, NULL, &cannot_delete_rect);
            SDL_RenderPresent(renderer);

            Uint32 start_time = SDL_GetTicks();
            while (SDL_GetTicks() - start_time < 2000) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        return;
                    }
                }
                SDL_Delay(16);
            }
            
            render_screen(renderer, 1);
            return;
        }
    }
}

int join_room(SDL_Renderer *renderer, int Rnumber) {
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "JOIN_ROOM_%d\n", Rnumber);
    send(sockfd, buffer, strlen(buffer), 0);

    char recv_buffer[MAXLINE];
    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        recv_buffer[nbyte] = '\0';
        if (strcmp(recv_buffer, "POSSIBLE") == 0) {
            SDL_Rect green_wait_rect = {329, 521, 782, 96};
            SDL_RenderCopy(renderer, green_wait_texture, NULL, &green_wait_rect);
            SDL_RenderPresent(renderer);

            char join_buffer[MAXLINE];
            snprintf(join_buffer, sizeof(join_buffer), "IM_%d_%d_%s\n", Rnumber, sockID, nickname);
            send(sockfd, join_buffer, strlen(join_buffer), 0);

            while (1) {
                // 이벤트 처리는 계속 수행
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        return 0;
                    }
                }

                fd_set readfds;
                struct timeval timeout;
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;

                if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
                    char status_buffer[MAXLINE];
                    int status_nbyte = recv(sockfd, status_buffer, MAXLINE, 0);
                    if (status_nbyte > 0) {
                        status_buffer[status_nbyte] = '\0';
                        
                        if (strcmp(status_buffer, "GAME_PAINTER_START") == 0) {
                            current_state = GAME_PAINTER_SCREEN;
                            render_update_needed = 1;
                            return 1;
                        }
                        else if (strcmp(status_buffer, "GAME_PLAYER_START") == 0) {
                            current_state = GAME_PLAYER_SCREEN;
                            render_update_needed = 1;
                            return 1;
                        }
                        else if (strcmp(status_buffer, "WAIT") == 0){
                            continue;
                        }
                    }
                }
                SDL_Delay(16);
            }
        }
        else{
            SDL_Rect red_wait_rect = {177, 521, 1086, 96};
            SDL_RenderCopy(renderer, red_wait_texture, NULL, &red_wait_rect);
            SDL_RenderPresent(renderer);

            Uint32 start_time = SDL_GetTicks();
            while (SDL_GetTicks() - start_time < 2000) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        return 0;
                    }
                }
                SDL_Delay(16);
            }
            
            render_screen(renderer, 1);
            return 0;
        }
    }
    return 0;
}

void render_screen(SDL_Renderer *renderer, int isRefresh){
    if(isRefresh){
        SDL_RenderClear(renderer); 

        SDL_RenderCopy(renderer, waiting_background_texture, NULL, NULL);

        SDL_Rect waiting_title_rect = {341, 65, 736, 130};
        SDL_RenderCopy(renderer, waiting_title_texture, NULL, &waiting_title_rect);

        SDL_Rect create_button_rect = {263, 243, 399, 94};
        SDL_RenderCopy(renderer, create_button_texture, NULL, &create_button_rect);

        SDL_Rect refresh_button_rect = {779, 243, 399, 94};
        SDL_RenderCopy(renderer, refresh_button_texture, NULL, &refresh_button_rect);

        SDL_Rect white_back_rect = {73, 80, 179, 58};
        SDL_RenderCopy(renderer, waiting_white_back_texture, NULL, &white_back_rect);
    }

    read_rooms_from_file();

    if (rooms_num >= 1) {
        SDL_Rect room1_button_rect = {145, 401, 517, 235};
        SDL_RenderCopy(renderer, room1_texture, NULL, &room1_button_rect);
        SDL_Rect x_button_rect = {590, 403, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (rooms_num >= 2) {
        SDL_Rect room2_button_rect = {779, 401, 517, 235};
        SDL_RenderCopy(renderer, room2_texture, NULL, &room2_button_rect);
        SDL_Rect x_button_rect = {1223, 403, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (rooms_num >= 3) {
        SDL_Rect room3_button_rect = {145, 693, 517, 235};
        SDL_RenderCopy(renderer, room3_texture, NULL, &room3_button_rect);
        SDL_Rect x_button_rect = {590, 695, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (rooms_num == 4) {
        SDL_Rect room4_button_rect = {779, 693, 517, 235};
        SDL_RenderCopy(renderer, room4_texture, NULL, &room4_button_rect);
        SDL_Rect x_button_rect = {1223, 695, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    
    if(isRefresh == 2){
        SDL_Rect green_wait_rect = {329, 518, 782, 96};
        SDL_RenderCopy(renderer, green_wait_texture, NULL, &green_wait_rect);
    }
    
    if(isRefresh == 3){
        SDL_Rect red_wait_rect = {329, 518, 782, 96};
        SDL_RenderCopy(renderer, red_wait_texture, NULL, &red_wait_rect);
    }
    
    SDL_RenderPresent(renderer);
}

void close_waiting_room(){
    SDL_DestroyTexture(waiting_title_texture);
    SDL_DestroyTexture(waiting_background_texture);
    SDL_DestroyTexture(waiting_white_back_texture);
    SDL_DestroyTexture(room1_texture);
    SDL_DestroyTexture(room2_texture);
    SDL_DestroyTexture(room3_texture);
    SDL_DestroyTexture(room4_texture);
    SDL_DestroyTexture(create_button_texture);
    SDL_DestroyTexture(refresh_button_texture);
    SDL_DestroyTexture(x_button_texture);
}

void render_waiting_room(SDL_Renderer *renderer)
{
    if (!init_waiting_room_images(renderer))
    {
        return;
    }

    SDL_RenderCopy(renderer, waiting_background_texture, NULL, NULL);

    SDL_Rect waiting_title_rect = {341, 65, 736, 130};
    SDL_RenderCopy(renderer, waiting_title_texture, NULL, &waiting_title_rect);

    SDL_Rect create_button_rect = {263, 243, 399, 94};
    SDL_RenderCopy(renderer, create_button_texture, NULL, &create_button_rect);

    SDL_Rect refresh_button_rect = {779, 243, 399, 94};
    SDL_RenderCopy(renderer, refresh_button_texture, NULL, &refresh_button_rect);

    SDL_Rect white_back_rect = {73, 80, 179, 58};
    SDL_RenderCopy(renderer, waiting_white_back_texture, NULL, &white_back_rect);

    SDL_RenderPresent(renderer);

    render_screen(renderer, 0);
    SDL_Event event;

    int running = 1;
    while(running){
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

                // "Create" 버튼 클릭 확인
                else if (x >= 263 && x <= 662 && y >= 243 && y <= 337)
                {
                    create_new_room(renderer); // 방 생성 함수 호출
                }

                // "Refresh" 버튼 클릭 확인
                else if (x >= 779 && x <= 1178 && y >= 243 && y <= 337)
                {
                    render_screen(renderer, 1);
                }

                // 참가할 방 클릭
                else if (x >= 145 && x <= 500 && y >= 399 && y <= 634) {
                    if(rooms_num >= 1){
                        if(join_room(renderer, 1) == 1){
                            current_room_num = 1;
                            running = 0;
                        }                        
                        return;
                    }
                }
                else if (x >= 779 && x <= 1200 && y >= 401 && y <= 636) { 
                    // current_room_num = 2;
                    // if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                    //     room_player_num[current_room_num - 1]++;
                    //     int status = handle_room_status(renderer);  // handle_room_status를 한 번만 호출
                    //     if(status == 5){
                    //         temp_state = GAME_PAINTER_SCREEN;
                    //         render_update_needed = 1;
                    //     }
                    //     else if(status == 7){
                    //         temp_state = GAME_PLAYER_SCREEN;
                    //         render_update_needed = 1;
                    //     }
                    // }
                    if(rooms_num >= 2){
                        if(join_room(renderer, 2) == 1){
                            current_room_num = 2;
                            running = 0;
                        }                        
                        return;
                    }
                }
                else if (x >= 145 && x <= 500 && y >= 690 && y <= 870) {
                    if(rooms_num >= 3){
                        if(join_room(renderer, 3) == 1){
                            current_room_num = 3;
                            running = 0;
                        }                        
                        return;
                    }
                }
                else if (x >= 779 && x <= 1200 && y >= 690 && y <= 870) { 
                    if(rooms_num == 4){
                        if(join_room(renderer, 4) == 1){
                            current_room_num = 4;
                            running = 0;
                        }                        
                        return;
                    }
                }

                if (rooms_num > 0) 
                {
                    // 마지막 방 x 버튼의 위치 계산
                    SDL_Rect x_button_rect;
                    if (rooms_num == 1) {
                        x_button_rect = (SDL_Rect){590, 403, 45, 68};
                    } else if (rooms_num == 2) {
                        x_button_rect = (SDL_Rect){1223, 403, 45, 68};
                    } else if (rooms_num == 3) {
                        x_button_rect = (SDL_Rect){590, 695, 45, 68};
                    } else if (rooms_num == 4) {
                        x_button_rect = (SDL_Rect){1223, 695, 45, 68};
                    }

                    // x 버튼 클릭 확인
                    if (x >= x_button_rect.x && x <= x_button_rect.x + x_button_rect.w &&
                        y >= x_button_rect.y && y <= x_button_rect.y + x_button_rect.h) {
                        remove_last_room(renderer); // 마지막 방 삭제
                    }
                }
            }
        }
    }
}
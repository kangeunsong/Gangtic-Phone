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
void read_rooms_from_file(char ***rooms_list);
void create_new_room(SDL_Renderer *renderer, char ***rooms_list);
void remove_last_room(SDL_Renderer *renderer, char ***rooms_list);
void save_rooms_to_file(char **rooms_list, int room_count);
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

GameState temp_state = WAITING_ROOM;

int room_num = 0;
int current_room_num = -1;
char **rooms = NULL;
int room_player_num[4] = {0,};
int temp_state_num = 0;

void print_rooms_status() {
    printf("print_room_status\n");
    for (int i = 0; i < room_num; i++) {
        printf("Room %d\n", i + 1);
        printf("Room ID: %s\n", rooms[i]);
        printf("Number of users: %d\n", room_player_num[i]);
        printf("\n\n");
    }
}

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

    return 1; // 모든 이미지 로드 성공 시
}

void get_room_info(){
    char buffer[MAXLINE];

    snprintf(buffer, sizeof(buffer), "GET_ROOM_INTO_%d\n", current_room_num);
    send(sockfd, buffer, strlen(buffer), 0);

    char recv_buffer[MAXLINE];
    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        recv_buffer[nbyte] = '\0';
        printf("Received: %s\n", recv_buffer);  // 디버깅을 위한 출력 추가
        
        if (strncmp(recv_buffer, "ROOM_NUM_", 9) == 0) {
            room_player_num[0] = atoi(&recv_buffer[9]);
            room_player_num[1] = atoi(&recv_buffer[11]);
            room_player_num[2] = atoi(&recv_buffer[13]);
            room_player_num[3] = atoi(&recv_buffer[15]);
        }
    }
}

void read_rooms_from_file(char ***rooms_list)
{
    const char *filename = "data/rooms.txt";
    FILE *file = fopen(filename, "r");

    // 첫 번째 줄에서 방 개수 읽기
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file))
    {
        room_num = atoi(buffer);
    }

    // 방 개수만큼 동적 메모리 할당
    *rooms_list = (char **)malloc(room_num * sizeof(char *));
    if (*rooms_list == NULL)
    {
        printf("메모리 할당 실패\n");
        fclose(file);
        return;
    }

    // 방 이름들을 읽어 rooms_list에 저장
    int i = 0;
    while (fgets(buffer, sizeof(buffer), file) && i < room_num) {
        buffer[strcspn(buffer, "\n")] = 0;  // 개행 문자 제거
        (*rooms_list)[i] = (char *)malloc(100 * sizeof(char)); // 방 이름의 최대 길이를 100으로 설정
        // 방 이름과 사용자 수 읽기
        int num_users;
        // 방 이름과 사용자 수를 각각 읽어옴
        sscanf(buffer, "%s %d", (*rooms_list)[i], &num_users);  // 방 이름과 사용자 수를 읽음
        room_player_num[i] = num_users;  // 방의 사용자 수 저장

        i++;
    }
    fclose(file);
}

// 방 생성
void create_new_room(SDL_Renderer *renderer, char ***rooms_list) {
    if (room_num >= 4) { // 최대 4개의 방만 생성 가능
        printf("Maximum number of rooms reached!\n");
        return;
    }

    room_num++;  // 방 개수 증가

    if (*rooms_list == NULL) {
        *rooms_list = malloc(room_num * sizeof(char *));
    }
    else{
        *rooms_list = realloc(*rooms_list, room_num * sizeof(char *));
    }

    // 방 이름 자동 생성
    char new_room_info[20];
    sprintf(new_room_info, "Room%d", room_num);
    (*rooms_list)[room_num - 1] = strdup(new_room_info);
    room_player_num[room_num - 1]=0;

    // rooms.txt 파일 업데이트
    save_rooms_to_file(*rooms_list, room_num);
    render_screen(renderer, 1);

    send_command("CREATE_ROOM");
}

void remove_last_room(SDL_Renderer *renderer, char ***rooms_list) {
    if (room_num <= 0) {
        printf("No rooms to remove.\n");
        return;
    }

    free((*rooms_list)[room_num - 1]);
    (*rooms_list)[room_num - 1] = NULL;
    room_player_num[room_num] = 0;
    room_num--;

    // 방 목록 크기 조정
    if (room_num > 0) {
        *rooms_list = realloc(*rooms_list, room_num * sizeof(char *));
    } else {
        free(*rooms_list);
        *rooms_list = NULL;
    }

    // rooms.txt 파일 업데이트
    save_rooms_to_file(*rooms_list, room_num);
    render_screen(renderer, 1);
}

// rooms.txt에 저장
void save_rooms_to_file(char **rooms_list, int room_count) {
    FILE *file = fopen("data/rooms.txt", "w");
    if (file == NULL) {
        printf("Unable to open rooms.txt for writing\n");
        return;
    }

    // 방 개수 저장
    fprintf(file, "%d\n", room_count);

    // 방 이름들을 파일에 저장
    for (int i = 0; i < room_count; i++) {
        // 방 이름과 사용자 수 저장
        fprintf(file, "%s %d\n", rooms_list[i], room_player_num[i]);  // rooms[i][0]은 해당 방의 사용자 수
    }

    read_rooms_from_file(&rooms);
    fclose(file);
}

void render_screen(SDL_Renderer *renderer, int isRefresh){
    // get_room_info();
    printf("here?1\n");
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

    read_rooms_from_file(&rooms);

    if (room_num >= 1) {
        SDL_Rect room1_button_rect = {145, 401, 517, 235};
        SDL_RenderCopy(renderer, room1_texture, NULL, &room1_button_rect);
        SDL_Rect x_button_rect = {590, 403, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (room_num >= 2) {
        SDL_Rect room2_button_rect = {779, 401, 517, 235};
        SDL_RenderCopy(renderer, room2_texture, NULL, &room2_button_rect);
        SDL_Rect x_button_rect = {1223, 403, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (room_num >= 3) {
        SDL_Rect room3_button_rect = {145, 693, 517, 235};
        SDL_RenderCopy(renderer, room3_texture, NULL, &room3_button_rect);
        SDL_Rect x_button_rect = {590, 695, 45, 68};
        SDL_RenderCopy(renderer, x_button_texture, NULL, &x_button_rect);
    }
    if (room_num == 4) {
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

// 클라이언트가 방에 입장 시 상태를 받아서 화면 전환
int handle_room_status(SDL_Renderer *renderer) {
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "JOIN_ROOM_%d\n", current_room_num);
    send(sockfd, buffer, strlen(buffer), 0);

    char recv_buffer[MAXLINE];
    int nbyte = recv(sockfd, recv_buffer, MAXLINE, 0);
    if (nbyte > 0) {
        recv_buffer[nbyte] = '\0';
        if (strcmp(recv_buffer, "ROOM_FULL") == 0) {
            return 0;
        }
        else if (strncmp(recv_buffer, "JOIN_POSSIBLE_", 14) == 0) {
            int join_check_user_num = atoi(&recv_buffer[14]);
            if (join_check_user_num == 1) {
                temp_state_num = 5;
            } else if (join_check_user_num > 1) {
                temp_state_num = 7;
            }
            
            if (join_check_user_num == 4) {
                current_state = temp_state;
                render_update_needed = 1;
            } else {
                current_state = WAITING_FOR_PLAYERS;
                // 각 방에 따라 다른 대기 화면 설정
                if (current_room_num == 1) {
                    render_screen(renderer, 3);  // room1은 red_wait
                } else if (current_room_num == 2) {
                    render_screen(renderer, 2);  // room2는 green_wait
                }
            }
            return temp_state_num;
        }
    }
    return 0;
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
        printf("홈 화면 이미지 로드 실패\n");
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
    // read_rooms_from_file(&rooms);
    // print_rooms_status();
    get_room_info();
    printf("here?2\n");

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
                
                get_room_info();
                printf("here?3\n");
                
                // "뒤로가기" 버튼 클릭 확인
                SDL_Rect white_back_rect = {73, 80, 179, 58};
                if (x >= white_back_rect.x && x <= white_back_rect.x + white_back_rect.w && y >= white_back_rect.y && y <= white_back_rect.y + white_back_rect.h)
                {
                    current_state = HOME_SCREEN;
                    for (int i = 0; i < room_num; i++)
                    {
                        free(rooms[i]); // 각 char*에 대해 메모리 해제
                    }
                    free(rooms);
                    rooms = NULL;
                    return;
                }

                // "Create" 버튼 클릭 확인
                else if (x >= 263 && x <= 662 && y >= 243 && y <= 337)
                {
                    create_new_room(renderer, &rooms); // 방 생성 함수 호출
                    get_room_info();
                    printf("here?4\n");
                }

                // "Refresh" 버튼 클릭 확인
                else if (x >= 779 && x <= 1178 && y >= 243 && y <= 337)
                {
                    render_screen(renderer, 1);
                }

                // 참가할 방 클릭
                else if (x >= 145 && x <= 500 && y >= 399 && y <= 634) {
                    printf("here?111\n");
                    // current_room_num = 1;
                    // if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                    //     room_player_num[current_room_num - 1]++;
                    //     int status = handle_room_status(renderer);  // handle_room_status를 한 번만 호출
                    //     printf("%d\n", status);
                    //     if(status == 5){
                    //         temp_state = GAME_PAINTER_SCREEN;
                    //         render_update_needed = 1;
                    //     }
                    //     else if(status == 7){
                    //         temp_state = GAME_PLAYER_SCREEN;
                    //         render_update_needed = 1;
                    //     }
                    // }

                    // return;

                    current_room_num = 1;
                    if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                        room_player_num[current_room_num - 1]++;
                        int status = handle_room_status(renderer);
                        printf("%d\n", status);
                        
                        // 1번 방은 바로 게임으로 진입
                        if(status == 5){
                            current_state = GAME_PAINTER_SCREEN;
                        } else {
                            current_state = GAME_PLAYER_SCREEN;
                        }
                        render_update_needed = 1;
                        return;  // return 추가
                    }
                }
                else if (x >= 779 && x <= 1200 && y >= 401 && y <= 636) { 
                    current_room_num = 2;
                    if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                        room_player_num[current_room_num - 1]++;
                        int status = handle_room_status(renderer);  // handle_room_status를 한 번만 호출
                        if(status == 5){
                            temp_state = GAME_PAINTER_SCREEN;
                            render_update_needed = 1;
                        }
                        else if(status == 7){
                            temp_state = GAME_PLAYER_SCREEN;
                            render_update_needed = 1;
                        }
                    }
                    return;
                }
                else if (x >= 145 && x <= 500 && y >= 690 && y <= 870) {
                    current_room_num = 3;
                    if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                        room_player_num[current_room_num - 1]++;
                        int status = handle_room_status(renderer);  // handle_room_status를 한 번만 호출
                        if(status == 5){
                            temp_state = GAME_PAINTER_SCREEN;
                            render_update_needed = 1;
                        }
                        else if(status == 7){
                            temp_state = GAME_PLAYER_SCREEN;
                            render_update_needed = 1;
                        }
                    }
                    return;
                }
                else if (x >= 779 && x <= 1200 && y >= 690 && y <= 870) { 
                    current_room_num = 4;
                    if(room_player_num[current_room_num - 1] < MAX_USERS_PER_ROOM){
                        room_player_num[current_room_num - 1]++;
                        int status = handle_room_status(renderer);  // handle_room_status를 한 번만 호출
                        if(status == 5){
                            temp_state = GAME_PAINTER_SCREEN;
                        }
                        else if(status == 7){
                            temp_state = GAME_PLAYER_SCREEN;
                        }
                    }
                    return;
                }

                if (room_num > 0) 
                {
                    // 마지막 방 x 버튼의 위치 계산
                    SDL_Rect x_button_rect;
                    if (room_num == 1) {
                        x_button_rect = (SDL_Rect){590, 403, 45, 68};
                    } else if (room_num == 2) {
                        x_button_rect = (SDL_Rect){1223, 403, 45, 68};
                    } else if (room_num == 3) {
                        x_button_rect = (SDL_Rect){590, 695, 45, 68};
                    } else if (room_num == 4) {
                        x_button_rect = (SDL_Rect){1223, 695, 45, 68};
                    }

                    // x 버튼 클릭 확인
                    if (x >= x_button_rect.x && x <= x_button_rect.x + x_button_rect.w &&
                        y >= x_button_rect.y && y <= x_button_rect.y + x_button_rect.h) {
                        remove_last_room(renderer, &rooms); // 마지막 방 삭제
                        send_command("REMOVE_ROOM");
                    }
                }
            }
        }
        if (current_state == WAITING_FOR_PLAYERS) {
            // printf("here?222\n");
            static Uint32 last_check_time = 0;
            Uint32 current_time = SDL_GetTicks();
            
            if (current_time - last_check_time > 1000) {  // 1초마다 체크
                fd_set readfds;
                struct timeval timeout;
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 10000;

                if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
                    char check_buffer[MAXLINE];
                    int nbyte = recv(sockfd, check_buffer, MAXLINE, 0);
                    if (nbyte > 0) {
                        check_buffer[nbyte] = '\0';
                        if (strncmp(check_buffer, "REQUEST_USER_NUM_", 17) == 0) {
                            int user_count = atoi(&check_buffer[17]);
                            if (user_count < 4){
                                printf("%d, %d\n", current_room_num, user_count);
                                render_screen(renderer, 2);
                            }
                            else if (user_count == 4){  // >= 로 수정
                                printf("%d, %d\n", current_room_num, user_count);
                                render_screen(renderer, 3);
                                
                                // 화면 전환도 여기서 처리
                                current_state = temp_state;
                                render_update_needed = 1;
                                return;
                            }
                        }
                    }
                }
                last_check_time = current_time;
            }
            
            // 대기 화면 업데이트
            // render_screen(renderer, 1);
        }
        
        SDL_Delay(16);
    }
}
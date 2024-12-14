#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "home.h"
#include "waiting.h"
#include "game.h"
#include "result.h"
#include "client.h"
#include "setting.h"

// #define SERVER_IP "127.25.228.113"
#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MAXLINE 1000

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 1024

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define BLUE 0, 0, 255, 255
#define RED 255, 0, 0, 255

// 서버와의 통신을 위한 함수들
void connect_to_server(const char* server_ip, int server_port);
void send_command(const char* command);
void send_nickname(const char* nickname);  // 닉네임을 서버로 전송하는 함수

void init_music();
void init_correct_sound();
void init_wrong_sound();

// 게임 진행 함수들
int init_sdl();

// SDL 관련 전역 변수
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

int sockfd; // 서버 소켓
int sockID;
int render_update_needed = 1;
GameState current_state = HOME_SCREEN;
char *nickname = NULL;
int answer_num = 0;

// 초기화 함수
int init_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Gangtic Phone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("assets/fonts/font.ttf", 55); // 폰트 파일 필요

    // init_music();  // 소리 잠깐 없애놓음
    init_correct_sound();
    init_wrong_sound();

    return 1;
}

// 서버에 연결
void connect_to_server(const char* server_ip, int server_port) {
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket() failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        exit(1);
    }
    
    int server_socket_id;
    recv(sockfd, &server_socket_id, sizeof(int), 0);
    sockID = server_socket_id; // Update sockfd to use server's ID
    
    printf("Connected with sockfd = %d\n", sockID);
}

// 서버에 명령을 보내는 함수
void send_command(const char* command) {
    ssize_t bytes_sent = send(sockfd, command, strlen(command), 0);  // 반환값으로 보낸 바이트 수 받기

    if (bytes_sent < 0) {
        perror("send() failed");
    } else {
        printf("Sent command: %s\n", command);
    }
}

void send_nickname(const char* nickname) {
    send(sockfd, nickname, strlen(nickname), 0);  // 닉네임 서버로 전송
}

void Gangtic_Phone(){
    SDL_Event event;
    int running = 1;
    while (running)
    {
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

                if (current_state == HOME_SCREEN)
                {
                    // "게임 시작" 버튼 클릭
                    if (x >= 58 && x <= 58 + 479 && y >= 580 && y <= 580 + 197)
                    {
                        current_state = WAITING_ROOM;
                        render_update_needed = 1;
                        printf("상태 변경: HOME_SCREEN -> WAITING_ROOM\n");
                    }
                    // "설정" 버튼 클릭
                    else if (x >= 58 && x <= 58 + 368 && y >= 822 && y <= 822 + 141)
                    {
                        printf("상태 변경: HOME_SCREEN -> SETTING_SCREEN\n");
                        current_state = SETTING_SCREEN;
                        render_update_needed = 1;
                    }
                }
            }  
            if(render_update_needed){
                if (current_state == HOME_SCREEN)
                {
                    render_home_screen(renderer);
                    render_update_needed = 0;
                }
                else if (current_state == WAITING_ROOM)
                {
                    render_waiting_room(renderer);
                }
                else if (current_state == SETTING_SCREEN)
                {
                    render_setting_screen(renderer);
                }
                else if (current_state == GAME_PAINTER_SCREEN)
                {
                    render_game_screen(renderer, font);
                }
                else if (current_state == GAME_PLAYER_SCREEN)
                {
                    render_game_screen(renderer, font);
                }
                else if (current_state == RESULT_SCREEN)
                {
                    render_result_screen(renderer, font);
                    if(render_update_needed == 0){
                        running = 0;
                    }
                }
            }   
        }
    }
    return ;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("사용법 : %s name\n", argv[0]);
        exit(0);
    }
    
    connect_to_server(SERVER_IP, PORT); // 서버 연결
    send_nickname(argv[1]);  // 사용자 입력 닉네임 전송
    nickname = argv[1];

    printf("My nickname is %s, and my sockID is %d, my sockfd is %d!\n", nickname, sockID, sockfd);

    if (!init_sdl()){
        return 1;
    }
    
    Gangtic_Phone();

    //close sdl
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    Mix_CloseAudio();
    close_home_screen();
    close_setting_screen();
    close_waiting_room();
    close_game_screen();
    close_result_screen();

    return 0;
}
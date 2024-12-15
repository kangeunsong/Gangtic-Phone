#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "utils.h"

#define MAX_ROOMS 4
#define MAX_USERS_PER_ROOM 4
#define MAX_ROUND 5
#define MAXLINE 1000
#define PORT 8080

Game games[MAX_ROOMS]; // 전체 게임 진행 상황이 담긴 구조체 배열
int rooms_num = 0;

pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;  // 방 목록을 보호할 뮤텍스

// 클라이언트의 요청을 처리하는 함수
void* handle_client(void* client_sock_ptr) {
    int client_sock = *((int*)client_sock_ptr);  // 클라이언트 소켓을 안전하게 받음
    send(client_sock, &client_sock, sizeof(int), 0);
    char buffer[MAXLINE*2];
    char cmdline[MAXLINE*2];

    while (1) {
        int nbyte = recv(client_sock, buffer, MAXLINE, 0);
        if (nbyte <= 0) {
            remove_client(client_sock);
            close(client_sock);  // 클라이언트 소켓 닫기
            free(client_sock_ptr);  // 동적으로 할당된 메모리 해제
            return NULL;
        }
        
        buffer[nbyte] = '\0';

        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            strncpy(cmdline, line, MAXLINE);
            cmdline[MAXLINE-1] = '\0';

            if (strcmp(cmdline, "EXIT") == 0) { 
                printf("Client requested to exit. Closing connection.\n");
                remove_client(client_sock);
                close(client_sock);  // 클라이언트 소켓 닫기
                free(client_sock_ptr);  // 동적으로 할당된 메모리 해제
                // return NULL;
                break;
            }

            else if (strcmp(cmdline, "GO_TO_HOME") == 0) { 
                printf("Server got GO_TO_HOME!\n");
                remove_client(client_sock);
            } 

            // 방 생성
            else if (strncmp(cmdline, "CREATE_ROOM_", 12) == 0) { 
                int create_room_num = atoi(&cmdline[12]);
                if(games[create_room_num-1].is_existing == 1){ // 방이 이미 존재할 경우
                    send(client_sock, "ALREADY_EXIST", strlen("ALREADY_EXIST"), 0);
                }

                else{
                    rooms_num++;
                    update_room_txtfile(); 
                    send(client_sock, "POSSIBLE", strlen("POSSIBLE"), 0);
                }
            } 

            // 방 삭제
            else if (strncmp(cmdline, "DELETE_ROOM_", 12) == 0) { 
                int delete_room_num = atoi(&cmdline[12]);
                if(games[delete_room_num-1].users_num > 0){ // 방에 플레이어가 존재할 경우
                    send(client_sock, "IMPOSSIBLE", strlen("IMPOSSIBLE"), 0);
                }

                else{
                    rooms_num--;
                    update_room_txtfile(); 
                    send(client_sock, "POSSIBLE", strlen("POSSIBLE"), 0);
                }
            } 
            
            // 방 입장 가능 확인
            else if (strncmp(cmdline, "JOIN_ROOM_", 10) == 0) { 
                int join_room_num = atoi(&cmdline[10]);
                if(games[join_room_num-1].users_num < 4){ 
                    send(client_sock, "POSSIBLE", strlen("POSSIBLE"), 0);
                }

                else{
                    send(client_sock, "IMPOSSIBLE", strlen("IMPOSSIBLE"), 0);
                }
            }
            
            // 방 입장
            else if (strncmp(cmdline, "IM_", 3) == 0) { 
                int user_room_num = atoi(&cmdline[3]);
                int user_sockID = atoi(&cmdline[5]);
                char user_nickname[MAXLINE];
                strcpy(user_nickname, cmdline + 7);

                if(games[user_room_num-1].users_num == 0){
                    games[user_room_num-1].drawing_sknum = user_sockID;
                }
                games[user_room_num-1].users_num++;
                games[user_room_num-1].users_sknum[games[user_room_num-1].users_num-1] = user_sockID;
                strcpy(games[user_room_num-1].users_nickname[games[user_room_num-1].users_num-1], user_nickname);

                update_room_txtfile();
                if(games[user_room_num-1].users_num == 4){
                    init_game_log(user_room_num);
                    for(int i=0;i<MAX_USERS_PER_ROOM;i++){
                        if(games[user_room_num-1].users_sknum[i] == games[user_room_num-1].drawing_sknum){
                            send(games[user_room_num-1].users_sknum[i], "GAME_PAINTER_START", strlen("GAME_PAINTER_START"), 0);
                        }
                        else{
                            send(games[user_room_num-1].users_sknum[i], "GAME_PLAYER_START", strlen("GAME_PLAYER_START"), 0);
                        }
                    }
                    games[user_room_num-1].round++;
                }
                else{
                    send(client_sock, "WAIT", strlen("WAIT"), 0);
                }
            }

            // 라운드 반환
            else if (strncmp(cmdline, "CHECK_ROUND_", 12) == 0) {
                int user_room_num = atoi(&cmdline[12]);
                char round_buffer[MAXLINE];
                snprintf(round_buffer, sizeof(round_buffer), "ROUND_%d", games[user_room_num-1].round);
                send(client_sock, round_buffer, strlen(round_buffer), 0);
            }

            // 정답 설정
            else if (strncmp(cmdline, "SET_ANSWER_", 11) == 0) { 
                int user_room_num = atoi(&cmdline[11]);
                char *answer_start = strchr(&cmdline[12], '_');
                
                if (!answer_start) {
                    printf("Invalid answer format\n");
                    return NULL;
                }
                
                answer_start++;
                answer_start[strcspn(answer_start, "\n")] = '\0';
                
                if (games[user_room_num-1].answer != NULL) {
                    free(games[user_room_num-1].answer);
                }
                
                // Allocate and copy new answer
                games[user_room_num-1].answer = strdup(answer_start);
                if (!games[user_room_num-1].answer) {
                    printf("Memory allocation failed!\n");
                    return NULL;
                }
                write_round_start(user_room_num, games[user_room_num-1].round);
                write_answer_set(user_room_num, games[user_room_num-1].answer);
            }

            // 다른 클라이언트 화면에 그림이 나타나게
            else if (strncmp(cmdline, "SEND_DRAWING_", 13) == 0) { 
                int user_room_num, x1, y1, x2, y2;
                sscanf(cmdline + 13, "%d_%d_%d_%d_%d", &user_room_num, &x1, &y1, &x2, &y2);

                for(int i=0;i<MAX_USERS_PER_ROOM;i++){
                    if(games[user_room_num - 1].users_sknum[i] != games[user_room_num - 1].drawing_sknum){
                        char send_drawing[MAXLINE];
                        snprintf(send_drawing, sizeof(buffer), "DRAW_%d_%d_%d_%d", x1, y1, x2, y2);
                        send(games[user_room_num - 1].users_sknum[i], send_drawing, strlen(send_drawing), 0);
                    }
                }
            }

            // 문제 맞혔을 때
            else if (strncmp(cmdline, "GET_CORRECT_", 12) == 0) {
                int correct_user_room_num = atoi(&cmdline[12]);
                int correct_user_sknum = atoi(&cmdline[14]);

                for(int i = 0; i < MAX_USERS_PER_ROOM ; i++){
                    if(games[correct_user_room_num-1].users_sknum[i] == correct_user_sknum){
                        int correct_user_index = i;
                        games[correct_user_room_num-1].users_score[correct_user_index] += 10;
                        write_point_gained(correct_user_room_num, games[correct_user_room_num-1].users_nickname[correct_user_index], 10);
                        if(games[correct_user_room_num-1].round >= MAX_ROUND){ // 마지막 라운드면 게임 끝
                            writing_result(correct_user_room_num); 
                            for(int j=0;j<MAX_USERS_PER_ROOM;j++){
                                if(games[correct_user_room_num-1].users_sknum[j] != correct_user_sknum){
                                    send(games[correct_user_room_num-1].users_sknum[j], "GAME_OVER", strlen("GAME_OVER"), 0);
                                }
                            }
                            return NULL;
                        }
                        send(games[correct_user_room_num-1].drawing_sknum, "STOP_DRAWING", strlen("STOP_DRAWING"), 0);
                        for(int j=0;j<MAX_USERS_PER_ROOM;j++){ // 기존 출제자와 정답자를 제외한 나머지 클라이언트들에게 라운드 변경 알림
                            if((games[correct_user_room_num-1].users_sknum[j] != correct_user_sknum) && (games[correct_user_room_num-1].users_sknum[j] != games[correct_user_room_num-1].drawing_sknum)){
                                send(games[correct_user_room_num-1].users_sknum[j], "NEXT_ROUND", strlen("NEXT_ROUND"), 0);
                            }
                        }
                        games[correct_user_room_num-1].drawing_sknum=correct_user_sknum; //정답자를 다음 라운드의 출제자로
                        games[correct_user_room_num-1].round++;
                    }
                }
            }
            
            // 답을 받아서 확인
            else if (strncmp(cmdline, "RECV_ANSWER_", 12) == 0) {
                int user_room_num = 0;
                char recv_answer[MAXLINE] = {0};

                if (sscanf(&cmdline[12], "%d_%s", &user_room_num, recv_answer) == 2) { 
                    char *stored_answer = games[user_room_num - 1].answer;
                    stored_answer[strcspn(stored_answer, "\r\n")] = '\0';
                    recv_answer[strcspn(recv_answer, "\r\n")] = '\0';
                    if (stored_answer) {
                        int temp_index = 0;
                        for(int i=0;i<MAX_USERS_PER_ROOM;i++){
                            if(games[user_room_num-1].users_sknum[i]==client_sock){
                                temp_index = i;
                            }
                        }
                        if (strcasecmp(recv_answer, stored_answer) == 0) {
                            write_guess_result(user_room_num, games[user_room_num-1].users_nickname[temp_index], recv_answer, 1); 
                            send(client_sock, "RIGHT", strlen("RIGHT"), 0);
                        } else {
                            write_guess_result(user_room_num, games[user_room_num-1].users_nickname[temp_index], recv_answer, 0); 
                            send(client_sock, "WRONG", strlen("WRONG"), 0);
                        }
                    }
                }
            }

            // 게임이 끝나면 해당 방의 구조체 배열을 초기화
            else if (strncmp(cmdline, "FINISH_", 5) == 0) { 
                int exit_room_num = atoi(&cmdline[5]);
                games[exit_room_num-1].users_num--;
                if(games[exit_room_num-1].users_num == 0){
                    if (games[exit_room_num-1].answer != NULL) {
                        free(games[exit_room_num-1].answer);
                    }
                    memset(&games[exit_room_num-1], 0, sizeof(Game));
                }
                break; 
            }

            else {
                printf("%s님 입장\n", buffer);
            }

            line = strtok(NULL, "\n");
        }
        memset(buffer, 0, sizeof(buffer));
    }

    close(client_sock);  // 클라이언트 소켓 닫기
    free(client_sock_ptr);
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    int listen_sock, client_sock;
    socklen_t addr_len = sizeof(client_addr);

    memset(games, 0, sizeof(games));

    FILE *file = fopen("data/rooms.txt", "w");
    if (file == NULL) {
        perror("Unable to open rooms.txt for writing");
        exit(1);
    }
    fprintf(file, "0\n");
    fclose(file);

    // 서버 소켓 생성
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket() failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(1);
    }

    listen(listen_sock, 5);
    printf("서버가 시작되었습니다.\n");

    while (1) {
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            continue;
        }

        // 동적 메모리 할당으로 클라이언트 소켓 전달
        int* client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;

        // 새 스레드로 클라이언트 처리
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void*)client_sock_ptr) != 0) {
            perror("pthread_create() failed");
        }

        // 스레드가 종료되면 자원 해제
        pthread_detach(client_thread);
    }

    close(listen_sock);
    memset(games, 0, sizeof(games));
    return 0;
}
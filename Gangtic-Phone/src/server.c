#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_ROOMS 4
#define MAX_USERS_PER_ROOM 4
#define MAXLINE 1000
#define PORT 8080

typedef struct {
    int room_id;
    int num_users;
    int users[MAX_USERS_PER_ROOM]; // 방에 입장한 사용자들의 소켓 번호
} Room;

typedef struct{
    int drawing_user_sockfd;
    char* answer;
    int scores[MAX_USERS_PER_ROOM];
    int drawing_client_sock; 
    int round;
} RunningGame;

typedef struct{
    int is_existing;
    int users_num;
    int users_sknum[4];
    int users_score[4];
    char users_nickname[4][MAXLINE];
    char *answer;
    int round;
    int drawing_sknum;
} Game;

Room rooms[MAX_ROOMS];  // 방 목록
RunningGame runninggames[MAX_ROOMS];

Game games[MAX_ROOMS];
int rooms_num = 0;

int num_rooms = 0;      // 현재 방의 수
int drawing_user[4] = {0, 0, 0, 0};

pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;  // 방 목록을 보호할 뮤텍스

void* handle_client(void* client_sock_ptr);
void update_room_txtfile();
void remove_room(int client_sock);
int join_room(int client_sock, int room_num); // 방에 사용자 추가
int return_room_num(int client_sock); // 현재 클라이언트가 속한 방 번호 반환
void remove_client(int client_sock);
void print_rooms_status();

// 클라이언트의 요청을 처리하는 함수
void* handle_client(void* client_sock_ptr) {
    int client_sock = *((int*)client_sock_ptr);  // 클라이언트 소켓을 안전하게 받음
    send(client_sock, &client_sock, sizeof(int), 0);
    char buffer[MAXLINE*2];
    char cmdline[MAXLINE*2];

    // 클라이언트로부터 데이터를 받는다
    while (1) {
        int nbyte = recv(client_sock, buffer, MAXLINE, 0);
        if (nbyte <= 0) {
            printf("No data received or connection closed\n");
            remove_client(client_sock);
            close(client_sock);  // 클라이언트 소켓 닫기
            free(client_sock_ptr);  // 동적으로 할당된 메모리 해제
            return NULL;
        }
        
        buffer[nbyte] = '\0';
        printf("Received data: %s\n", buffer);

        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            strncpy(cmdline, line, MAXLINE);
            cmdline[MAXLINE-1] = '\0';

            if (strcmp(cmdline, "EXIT") == 0) { // "EXIT" 메시지를 받으면 종료
                printf("Client requested to exit. Closing connection.\n");
                remove_client(client_sock);
                break; 
            }

            else if (strcmp(cmdline, "GO_TO_HOME") == 0) { 
                printf("Server got GO_TO_HOME!\n");
                remove_client(client_sock);
            } 

            else if (strncmp(cmdline, "CREATE_ROOM_", 12) == 0) { // 방 생성
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

            else if (strncmp(cmdline, "DELETE_ROOM_", 12) == 0) { // 방 삭제
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
            
            else if (strncmp(cmdline, "JOIN_ROOM_", 10) == 0) { // 방 입장 가능 확인
                int join_room_num = atoi(&cmdline[10]);
                if(games[join_room_num-1].users_num < 4){ 
                    send(client_sock, "POSSIBLE", strlen("POSSIBLE"), 0);
                }

                else{
                    send(client_sock, "IMPOSSIBLE", strlen("IMPOSSIBLE"), 0);
                }
            }
            
            else if (strncmp(cmdline, "IM_", 3) == 0) { // 방 입장
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
                print_rooms_status();
                if(games[user_room_num-1].users_num == 4){
                    for(int i=0;i<MAX_USERS_PER_ROOM;i++){
                        if(games[user_room_num-1].users_sknum[i] == games[user_room_num-1].drawing_sknum){
                            send(games[user_room_num-1].users_sknum[i], "GAME_PAINTER_START", strlen("GAME_PAINTER_START"), 0);
                        }
                        else{
                            send(games[user_room_num-1].users_sknum[i], "GAME_PLAYER_START", strlen("GAME_PLAYER_START"), 0);
                        }
                    }
                }
                else{
                    send(client_sock, "WAIT", strlen("WAIT"), 0);
                }
            }

            else if (strncmp(cmdline, "CHECK_ROUND_", 12) == 0) {
                int user_room_num = atoi(&cmdline[12]);
                int user_socket_num = atoi(&cmdline[14]);

                if(runninggames[user_room_num - 1].round > 5){
                    send(user_socket_num, "GAME_OVER", strlen("GAME_OVER"), 0);
                }
                else{
                    send(user_socket_num, "NOTHING", strlen("NOTHING"), 0);
                }
            }

            // 출제자 설정
            else if (strncmp(cmdline, "SET_PAINTER_", 12) == 0) {
                int drawing_user_room_num, drawing_user_sknum;
        
                // sscanf를 사용하여 두 값을 추출
                sscanf(cmdline + 12, "%d_%d", &drawing_user_room_num, &drawing_user_sknum);
                printf("%d, %d\n", drawing_user_room_num, drawing_user_sknum);
                runninggames[drawing_user_room_num-1].drawing_client_sock = drawing_user_sknum;
                runninggames[drawing_user_room_num-1].round++;
            }

            else if (strncmp(cmdline, "SET_ANSWER_", 11) == 0) {
                int user_room_num = atoi(&cmdline[11]);
                char *answer_start = strchr(&cmdline[12], '_');
                
                if (!answer_start) {
                    printf("Invalid answer format\n");
                    return NULL;
                }
                
                answer_start++; // Skip the underscore
                // Remove newline if present
                answer_start[strcspn(answer_start, "\n")] = '\0';
                
                if (user_room_num > 0 && user_room_num <= MAX_ROOMS) {
                    // Free existing answer if any
                    if (runninggames[user_room_num-1].answer != NULL) {
                        free(runninggames[user_room_num-1].answer);
                    }
                    
                    // Allocate and copy new answer
                    runninggames[user_room_num-1].answer = strdup(answer_start);
                    if (!runninggames[user_room_num-1].answer) {
                        printf("Memory allocation failed!\n");
                        return NULL;
                    }
                    
                    printf("Stored answer for room %d: %s\n", user_room_num, runninggames[user_room_num-1].answer);
                } else {
                    printf("Invalid room number: %d\n", user_room_num);
                }
            }

            else if (strncmp(cmdline, "SEND_DRAWING_", 13) == 0) { // 다른 클라이언트 화면에 그림이 나타나게
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
                printf("GET_CORRECT\n");
                int correct_user_room_num = atoi(&cmdline[12]);
                int correct_user_sknum = atoi(&cmdline[14]);
                printf("Server got GET_CORRECT_%d_%d!\n", correct_user_room_num, correct_user_sknum);

                send(runninggames[correct_user_room_num-1].drawing_client_sock, "STOP_DRAWING", strlen("STOP_DRAWING"), 0);
                printf("I sent the painter to stop drawing!\n");

                for(int i = 0; i < 4 ; i++){
                    if(rooms[correct_user_room_num-1].users[i] == correct_user_sknum){
                        int correct_user_index = i;
                        runninggames[correct_user_room_num-1].scores[correct_user_index] += 10;
                    }
                }
            }
            
            // 답을 받아서 확인
            else if (strncmp(cmdline, "RECV_ANSWER_", 12) == 0) {
                int user_room_num = 0;
                char recv_answer[MAXLINE] = {0}; 
                printf("12331233\n");

                if (sscanf(&cmdline[12], "%d_%s", &user_room_num, recv_answer) == 2) { 
                    printf("12331233\n");
                    // Clean up the strings before comparison                
                    char *stored_answer = runninggames[user_room_num - 1].answer;
                    stored_answer[strcspn(stored_answer, "\r\n")] = '\0';
                    recv_answer[strcspn(recv_answer, "\r\n")] = '\0';
                    if (stored_answer) {
                        for(int i = 0; i < strlen(recv_answer) + 1; i++) {
                            printf("%02x ", (unsigned char)recv_answer[i]);
                        }
                        for(int i = 0; i < strlen(stored_answer) + 1; i++) {
                            printf("%02x ", (unsigned char)stored_answer[i]);
                        }
                        printf("\n");

                        if (strcasecmp(recv_answer, stored_answer) == 0) {
                            send(client_sock, "RIGHT", strlen("RIGHT"), 0);
                        } else {
                            send(client_sock, "WRONG", strlen("WRONG"), 0);
                        }
                    }
                }
            }

            // 그림 다른 클라이언트한테 반영
            else if (strcmp(cmdline, "RECV_DRAWING") == 0) {
                printf("Server got RECV_DRAWING!\n");
                // 유저가 그림 그리는 것을 받음 -> 다른 클라이언트들 화면에 반영할 것
            }

            else {
                printf("%s님 입장\n", buffer);  // "kangeunsong님 입장" 출력
            }

            line = strtok(NULL, "\n");
        }
        memset(buffer, 0, sizeof(buffer));  // buffer 초기화
    }

    close(client_sock);  // 클라이언트 소켓 닫기
    free(client_sock_ptr);  // 동적 할당된 메모리 해제
    return NULL;
}

// rooms.txt 업데이트 함수
void update_room_txtfile() {
    pthread_mutex_lock(&rooms_mutex);  // 방 목록에 접근하기 전에 뮤텍스 잠금

    FILE *file = fopen("data/rooms.txt", "r+");
    if (file == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    // Create a temporary file to write updated content
    FILE *temp_file = fopen("data/rooms_temp.txt", "w");
    if (temp_file == NULL) {
        fclose(file);
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    fprintf(temp_file, "%d\n", rooms_num);

    for (int i = 0; i < rooms_num; i++) {
        // 방 이름과 사용자 수 저장
        fprintf(temp_file, "Room%d %d\n", i+1, games[i].users_num);  // rooms[i][0]은 해당 방의 사용자 수
    }

    fclose(file);
    fclose(temp_file);

    // Replace the original file with the updated temporary file
    if (rename("data/rooms_temp.txt", "data/rooms.txt") != 0) {
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    pthread_mutex_unlock(&rooms_mutex);  // 방 목록 접근이 끝났으므로 뮤텍스 잠금 해제
}

// 방 삭제 함수
void remove_room(int client_sock) {
    pthread_mutex_lock(&rooms_mutex);  // 방 목록에 접근하기 전에 뮤텍스 잠금

    if (num_rooms > 0) {
        // 마지막 방을 삭제
        num_rooms--;
        printf("방 %d가 삭제되었습니다.\n", rooms[num_rooms].room_id);

        // 마지막 방의 사용자들 초기화
        rooms[num_rooms].num_users = 0;
        memset(rooms[num_rooms].users, 0, sizeof(rooms[num_rooms].users));  // 방의 사용자 소켓 초기화

        printf("방 삭제 성공");
    } else {
        // 방이 0개라면 삭제 불가
        printf("방 삭제 실패: 방이 존재하지 않음\n");
    }

    pthread_mutex_unlock(&rooms_mutex);  // 방 목록 접근이 끝났으므로 뮤텍스 잠금 해제
}

// 사용자가 방에 들어갈 때, 방에 사용자 추가
int join_room(int client_sock, int Rnumber) {
    pthread_mutex_lock(&rooms_mutex);  // 방 목록에 접근하기 전에 뮤텍스 잠금

    if (Rnumber < 1 || Rnumber > rooms_num) {
        printf("잘못된 방 번호입니다.\n");
        pthread_mutex_unlock(&rooms_mutex);
        return -1;
    }

    // 방에 추가할 수 있는 자리가 있다면
    if (rooms[rooms_num - 1].num_users < MAX_USERS_PER_ROOM) {
        rooms[rooms_num - 1].users[rooms[rooms_num - 1].num_users] = client_sock;
        rooms[rooms_num - 1].num_users++;
    } else {
        printf("방이 가득 찼습니다.\n");
        pthread_mutex_unlock(&rooms_mutex);
        return -1;
    }

    FILE *file = fopen("data/rooms.txt", "r+");
    if (file == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        return -1;
    }

    // Create a temporary file to write updated content
    FILE *temp_file = fopen("data/rooms_temp.txt", "w");
    if (temp_file == NULL) {
        fclose(file);
        pthread_mutex_unlock(&rooms_mutex);
        return -1;
    }

    fprintf(temp_file, "%d\n", num_rooms);

    // Read and update room lines
    int line_num = 1;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (line_num == rooms_num) {
            // Update the room's user count
            fprintf(temp_file, "Room%d %d\n", rooms_num, rooms[rooms_num - 1].num_users);
        } else {
            fputs(buffer, temp_file);
        }
        line_num++;
    }

    fclose(file);
    fclose(temp_file);

    // Replace the original file with the updated temporary file
    if (rename("data/rooms_temp.txt", "data/rooms.txt") != 0) {
        pthread_mutex_unlock(&rooms_mutex);
        return -1;
    }

    pthread_mutex_unlock(&rooms_mutex);

    if(rooms[rooms_num-1].num_users > 4){
        send(client_sock, "ROOM_FULL", strlen("ROOM_FULL"), 0);
    }
    else{
        char join_buffer[MAXLINE];
        
        printf("***** %d\n *****", rooms[rooms_num-1].num_users);
        snprintf(join_buffer, sizeof(join_buffer), "JOIN_POSSIBLE_%d\n", rooms[rooms_num-1].num_users);
        send(client_sock, join_buffer, strlen(join_buffer), 0);
    }
    
    return 0;
}

// 클라이언트가 속한 방 번호 반환
int return_room_num(int client_sock) {
    pthread_mutex_lock(&rooms_mutex);  // 방 목록에 접근하기 전에 뮤텍스 잠금

    for (int i = 0; i < num_rooms; i++) {
        for (int j = 0; j < rooms[i].num_users; j++) {
            if (rooms[i].users[j] == client_sock) {
                printf("클라이언트가 속한 방: %d\n", i + 1);
                int client_room_num = i + 1;
                
                // 방 번호를 네트워크 바이트 순서로 변환하여 전송
                int network_room_num = htonl(client_room_num);
                printf("network_room_num: %d\n", network_room_num);
                send(client_sock, &network_room_num, sizeof(int), 0);  // 방 번호를 클라이언트로 전송
                
                pthread_mutex_unlock(&rooms_mutex);  // 방 목록 접근이 끝났으므로 뮤텍스 잠금 해제
                return client_room_num;  // 1부터 시작하는 방 번호 반환
            }
        }
    }

    printf("아직 방에 속하지 않았습니다.\n");

    pthread_mutex_unlock(&rooms_mutex);  // 방 목록 접근이 끝났으므로 뮤텍스 잠금 해제
    return -1;
}

void remove_client(int client_sock){
    int room_num = return_room_num(client_sock);

    if (room_num != -1) {
        pthread_mutex_lock(&rooms_mutex);
        for (int j = 0; j < rooms[room_num - 1].num_users; j++) {
            if (rooms[room_num - 1].users[j] == client_sock) {
                // 해당 클라이언트 소켓을 방에서 삭제
                rooms[room_num - 1].users[j] = 0;
                rooms[room_num - 1].num_users--;
                printf("클라이언트가 방 %d에서 제거되었습니다.\n", room_num);
                break;
            }
        }
        pthread_mutex_unlock(&rooms_mutex);
    }
}

void print_rooms_status() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        printf("Room %d\n", i + 1);
        printf("Number of users: %d\n", games[i].users_num);
        printf("Users in the room: ");
        
        // 방에 있는 사용자의 소켓 번호 출력
        for (int j = 0; j < MAX_USERS_PER_ROOM; j++) {
            printf("%d ", games[i].users_sknum[j]);
        }
        printf("\n\n");
    }
} // 확인용

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    int listen_sock, client_sock;
    socklen_t addr_len = sizeof(client_addr);

    memset(rooms, 0, sizeof(rooms));
    memset(runninggames, 0, sizeof(runninggames));
    print_rooms_status();
    printf("시작 전 초기화를 진행합니다.\n");

    FILE *file = fopen("data/rooms.txt", "w");
    if (file == NULL) {
        perror("Unable to open rooms.txt for writing");
        exit(1);
    }
    // 첫 번째 줄에 방 개수 0을 작성
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

    // 클라이언트 처리 루프
    while (1) {
        printf("start of while\n");
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            continue;  // 오류 발생 시 다시 연결 대기
        }
        printf("Client connected: %d\n", client_sock);

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
    memset(rooms, 0, sizeof(rooms));
    memset(runninggames, 0, sizeof(runninggames));
    printf("종료 전 초기화를 진행합니다.\n");
    print_rooms_status();
    return 0;
}
#include <time.h>
#include "utils.h"
#include "server.h"

typedef struct {
    char nickname[MAXLINE];
    int score;
} PlayerScore;

// rooms.txt 업데이트 함수
void update_room_txtfile() {
    pthread_mutex_lock(&rooms_mutex);  // 방 목록에 접근하기 전에 뮤텍스 잠금

    FILE *file = fopen("data/rooms.txt", "r+");
    if (file == NULL) {
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    FILE *temp_file = fopen("data/rooms_temp.txt", "w");
    if (temp_file == NULL) {
        fclose(file);
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    fprintf(temp_file, "%d\n", rooms_num);

    for (int i = 0; i < rooms_num; i++) {
        fprintf(temp_file, "Room%d %d\n", i+1, games[i].users_num);
    }

    fclose(file);
    fclose(temp_file);

    if (rename("data/rooms_temp.txt", "data/rooms.txt") != 0) {
        pthread_mutex_unlock(&rooms_mutex);
        return;
    }

    pthread_mutex_unlock(&rooms_mutex);  // 방 목록 접근이 끝났으므로 뮤텍스 잠금 해제
}

// 클라이언트가 속한 방 번호 반환
int return_room_num(int client_sock) {
    pthread_mutex_lock(&rooms_mutex);

    for (int i = 0; i < rooms_num; i++) {
        for (int j = 0; j < games[i].users_num; j++) {
            if (games[i].users_sknum[j] == client_sock) {
                int client_room_num = i + 1;
                int network_room_num = htonl(client_room_num);
                send(client_sock, &network_room_num, sizeof(int), 0);
                pthread_mutex_unlock(&rooms_mutex);
                return client_room_num;
            }
        }
    }

    pthread_mutex_unlock(&rooms_mutex);
    return -1;
}

// 클라이언트 제거
void remove_client(int client_sock){
    int Rnumber = return_room_num(client_sock);

    if (Rnumber != -1) {
        pthread_mutex_lock(&rooms_mutex);
        for (int j = 0; j < games[Rnumber - 1].users_num; j++) {
            if (games[Rnumber - 1].users_sknum[j] == client_sock) {
                games[Rnumber - 1].users_sknum[j] = 0;
                games[Rnumber - 1].users_num--;
                printf("클라이언트가 방 %d에서 제거되었습니다.\n", Rnumber);
                break;
            }
        }
        pthread_mutex_unlock(&rooms_mutex);
    }
}

// 점수 내림차순으로 정렬
static int compare_scores(const void *a, const void *b) {
    const PlayerScore *player_a = (const PlayerScore *)a;
    const PlayerScore *player_b = (const PlayerScore *)b;
    return player_b->score - player_a->score;
}

// result_n(n=Rnumber).txt에 결과를 입력
void writing_result(int Rnumber) {
    char filename[MAXLINE];
    char temp_filename[MAXLINE];
    snprintf(filename, sizeof(filename), "running_game/result_%d.txt", Rnumber);
    snprintf(temp_filename, sizeof(temp_filename), "running_game/temp_result_%d.txt", Rnumber);
    
    FILE *temp_file = fopen(temp_filename, "w");
    if (temp_file == NULL) {
        perror("Temporary result file creation failed");
        return;
    }

    PlayerScore players[MAX_USERS_PER_ROOM];
    int num_players = games[Rnumber-1].users_num;
    
    for (int i = 0; i < num_players; i++) {
        strncpy(players[i].nickname, games[Rnumber-1].users_nickname[i], MAXLINE);
        players[i].score = games[Rnumber-1].users_score[i];
    }

    qsort(players, num_players, sizeof(PlayerScore), compare_scores);

    for (int i = 0; i < num_players; i++) {
        fprintf(temp_file, "%s %d\n", players[i].nickname, players[i].score);
    }

    fclose(temp_file);

    if (rename(temp_filename, filename) != 0) {
        perror("Failed to rename temporary file");
        remove(temp_filename);
        return;
    }
}

//게임 로그 작성 함수들
void init_game_log(int Rnumber) {
    char filename[MAXLINE];
    snprintf(filename, sizeof(filename), "data/game_log/game_log_%d.txt", Rnumber);
    
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Game log file creation failed");
        return;
    }
    
    time_t current_time;
    struct tm *time_info;
    time(&current_time);
    time_info = localtime(&current_time);
    
    fprintf(file, "-----------------------------------\n");
    fprintf(file, "Date: %d-%02d-%02d\n", time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday);
    fprintf(file, "Time: %02d:%02d:%02d\n\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    fprintf(file, "<Room%d>\n\n", Rnumber);
    fclose(file);
}

void write_game_log(int Rnumber, const char* content) {
    char filename[MAXLINE];
    snprintf(filename, sizeof(filename), "data/game_log/game_log_%d.txt", Rnumber);
    
    FILE *file = fopen(filename, "a");  // append 모드로 열기
    if (file == NULL) {
        perror("Game log file open failed");
        return;
    }
    
    fprintf(file, "%s", content);
    fclose(file);
}

void write_round_start(int Rnumber, int round_num) {
    char content[MAXLINE];
    snprintf(content, sizeof(content), "Round: %d\n", round_num);
    write_game_log(Rnumber, content);
}

void write_answer_set(int Rnumber, const char* answer) {
    char content[MAXLINE];
    snprintf(content, sizeof(content), "Answer: %s\n", answer);
    write_game_log(Rnumber, content);
}

void write_guess_result(int Rnumber, const char* nickname, const char* guess, int is_correct) {
    char content[MAXLINE];
    snprintf(content, sizeof(content), "%s: %s (%s)\n", nickname, guess, is_correct ? "Right" : "Wrong");
    write_game_log(Rnumber, content);
}

void write_point_gained(int Rnumber, const char* nickname, int points) {
    char content[MAXLINE];
    snprintf(content, sizeof(content), "%s got %d points!\n\n", nickname, points);
    write_game_log(Rnumber, content);
}
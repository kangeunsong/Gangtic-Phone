#include "utils.h"

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

    printf("아직 방에 속하지 않았습니다.\n");
    pthread_mutex_unlock(&rooms_mutex);
    return -1;
}

// 클라이언트 제거
void remove_client(int client_sock){
    int room_num = return_room_num(client_sock);

    if (room_num != -1) {
        pthread_mutex_lock(&rooms_mutex);
        for (int j = 0; j < games[room_num - 1].users_num; j++) {
            if (games[room_num - 1].users_sknum[j] == client_sock) {
                games[room_num - 1].users_sknum[j] = 0;
                games[room_num - 1].users_num--;
                printf("클라이언트가 방 %d에서 제거되었습니다.\n", room_num);
                break;
            }
        }
        pthread_mutex_unlock(&rooms_mutex);
    }
}

// 확인용 출력문
void print_rooms_status() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        printf("Room %d\n", i + 1);
        printf("Number of users: %d\n", games[i].users_num);
        printf("Users in the room: ");
        
        for (int j = 0; j < MAX_USERS_PER_ROOM; j++) {
            printf("%d ", games[i].users_sknum[j]);
        }
        printf("\n\n");
    }
}
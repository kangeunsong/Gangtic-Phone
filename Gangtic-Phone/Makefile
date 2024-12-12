# 변수 정의
CC = gcc                  # 컴파일러
CFLAGS = -Wall -g -I/home/kangeunsong/SP_project/include  # 컴파일 옵션 (경고 메시지 및 디버깅 기호 포함, 헤더 파일 경로 추가)
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lpthread # 필요 라이브러리

# src 폴더 내의 특정 .c 파일 목록
SRCS_SERVER = src/server.c
SRCS_CLIENT = src/client.c src/home.c src/waiting.c src/game.c src/result.c src/setting.c

# 생성할 실행 파일
EXEC_SERVER = server
EXEC_CLIENT = client

# 기본 타겟: 서버와 클라이언트 모두 빌드
all: $(EXEC_SERVER) $(EXEC_CLIENT)

# 서버 실행 파일 생성
$(EXEC_SERVER): $(SRCS_SERVER)
	$(CC) $(CFLAGS) $(SRCS_SERVER) -o $(EXEC_SERVER) $(LIBS)

# 클라이언트 실행 파일 생성
$(EXEC_CLIENT): $(SRCS_CLIENT)
	$(CC) $(CFLAGS) $(SRCS_CLIENT) -o $(EXEC_CLIENT) $(LIBS)

# 클린 명령: 컴파일된 파일 삭제
clean:
	rm -f $(EXEC_SERVER) $(EXEC_CLIENT)

# 디버깅 및 기타 옵션을 추가하고 싶을 때 사용 가능
debug: CFLAGS += -DDEBUG
debug: clean $(EXEC_SERVER) $(EXEC_CLIENT)

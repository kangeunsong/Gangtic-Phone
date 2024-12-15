CC = gcc
CFLAGS = -Wall -g
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lpthread

# 기본 프로젝트 경로 (본인의 환경에 맞게 경로 수정 필요)
PROJECT_ROOT = /home/kangeunsong/Gangtic-Phone/Gangtic-Phone

SERVER_INCLUDE = -I$(PROJECT_ROOT)/include/server
CLIENT_INCLUDE = -I$(PROJECT_ROOT)/include/client

SRCS_SERVER = src/server/server.c src/server/utils.c
SRCS_CLIENT = src/client/client.c src/client/home.c src/client/waiting.c src/client/game.c src/client/result.c src/client/setting.c

EXEC_SERVER = server
EXEC_CLIENT = client

all: $(EXEC_SERVER) $(EXEC_CLIENT)

$(EXEC_SERVER): $(SRCS_SERVER)
	$(CC) $(CFLAGS) $(SERVER_INCLUDE) $(SRCS_SERVER) -o $(EXEC_SERVER) $(LIBS)

$(EXEC_CLIENT): $(SRCS_CLIENT)
	$(CC) $(CFLAGS) $(CLIENT_INCLUDE) $(SRCS_CLIENT) -o $(EXEC_CLIENT) $(LIBS)

clean:
	rm -f $(EXEC_SERVER) $(EXEC_CLIENT)

debug: CFLAGS += -DDEBUG
debug: clean $(EXEC_SERVER) $(EXEC_CLIENT)
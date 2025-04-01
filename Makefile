CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lpthread -lsqlite3

SERVER_SRC = src/server/server.c \
             src/server/auth.c \
             src/server/chat.c \
             src/server/db.c \
             src/server/server_network.c \
             src/common/util.c \
             src/common/network.c

CLIENT_SRC = src/client/client.c \
             src/client/client_network.c \
             src/client/ui.c \
             src/common/util.c \
             src/common/network.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

all: whisp_server whisp_client

whisp_server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

whisp_client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) whisp_server whisp_client

.PHONY: all clean

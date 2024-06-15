CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

SERVER_EXEC = neHosServer
CLIENT_EXEC = neHosClient

SERVER_SRCS = neHosServer.c guestQueue.c
CLIENT_SRCS = neHosClient.c clientList.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

.PHONY: all clean

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC)

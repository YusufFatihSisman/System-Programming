CC = gcc
CFLAGS = -Wall
LIB = -lrt -lpthread
DEPS = queue.h helper.h
OBJ = server.o queue.o helper.o

all: server client


%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS) $(LIB)

server: $(OBJ)
	$(CC)  -o $@ $^ $(CFLAGS) $(LIB)

client: client.c
	$(CC) -o client client.c $(CFLAGS)

CC = gcc
CFLAGS = -Wall
LIB = -lrt -lpthread
DEPS = queue.h helper.h
OBJ = main.o queue.o helper.o

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS) $(LIB)

program: $(OBJ)
	$(CC)  -o $@ $^ $(CFLAGS) $(LIB)

CC = clang
CFLAGS = -Wall -c
SOURCE = simplog.c
OBJ = simplog.o

all:
	$(CC) $(CFLAGS) $(SOURCE)

clean:
	rm -f $(OBJ) 

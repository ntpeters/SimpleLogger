CC = clang
CFLAGS = -Wall -c
LIB = libsimplog.a
SOURCE = simplog.c
OBJ = simplog.o

all:
	$(CC) $(CFLAGS) $(SOURCE)
	ar -cvq $(LIB) $(OBJ)

clean:
	rm -f $(LIB)
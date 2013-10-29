CC = clang
CFLAGS = -Wall -c
LIBNAME = libsimplog.a
SOURCE = log.c
OBJ = log.o

all:
	$(CC) $(CFLAGS) $(SOURCE)
	ar -cvq $(LIBNAME) $(OBJ)

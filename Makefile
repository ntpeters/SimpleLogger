CC = clang
CFLAGS = -g -Wall -c
SOURCE = simplog.c
OBJ = simplog.o simplog-temp.o backtrace-symbols.o

all:
	$(CC) $(CFLAGS) $(SOURCE); mv simplog.o simplog-temp.o; \
	$(CC) -ansi $(CFLAGS) backtrace-symbols.c; \
	ld -r simplog-temp.o backtrace-symbols.o -o simplog.o

clean:
	rm -f $(OBJ) 

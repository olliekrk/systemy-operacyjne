CC=gcc
CFLAGS=-Wall

all: clean static_build

static_build:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) program.o -o program




clean:
	rm -f *.o
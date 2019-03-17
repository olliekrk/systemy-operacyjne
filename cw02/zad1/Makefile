CC=gcc
CFLAGS=-Wall

all: clean static_build test

static_build:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) program.o -o program

test: tests.sh
	./tests.sh > wyniki.txt

clean:
	rm -f program.o program
	rm -f records*
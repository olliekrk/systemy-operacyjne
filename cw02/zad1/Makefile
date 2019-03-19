CC=gcc
CFLAGS=-Wall

all: clean static_build test clean

static_build:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) program.o -o program

test: tests.sh
	./tests.sh > wyniki.txt

clean:
	rm -f program.o program
	rm -f records records_copy_lib records_copy_sys
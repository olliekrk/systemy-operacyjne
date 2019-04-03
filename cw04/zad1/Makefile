CC=gcc
CFLAGS=-Wall -ggdb

CLEANLIST=program_a.o program_a program_b.o program_b

.PHONY: all build clean

all: build

build: program_a.c program_b.c
	$(CC) -c $(CFLAGS) program_a.c -o program_a.o
	$(CC) $(CFLAGS) program_a.o -o program_a
	$(CC) -c $(CFLAGS) program_b.c -o program_b.o
	$(CC) $(CFLAGS) program_b.o -o program_b

clean:
	rm -f $(CLEANLIST)
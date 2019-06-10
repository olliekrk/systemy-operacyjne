CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread -lm

.PHONY: all clean

all: client server

client: client.h client.c socketman.h
	$(CC) -c $(CFLAGS) client.c -o client.o
	$(CC) $(CFLAGS) client.o -o client

server: server.h server.c socketman.h
	$(CC) -c $(CFLAGS) server.c -o server.o
	$(CC) $(CFLAGS) server.o -o server

srun:
	./server 8888 sysopyserver

c1run:
	./client Adammalysz WEB 127.0.0.1:8888

c2run:
	./client Ceglak WEB 127.0.0.1:8888

c3run:
	./client Jaglak UNIX sysopyserver

clean:
	rm -rf client server *.o
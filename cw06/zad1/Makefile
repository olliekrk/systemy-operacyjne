all: build test clean

build: client.c server.c utils6.h
	gcc -c client.c -o client.o
	gcc client.o -o client
	gcc -c server.c -o server.o
	gcc server.o -o server

server_run:
	./server &

client_run:
	./client

test: server_run client_run

clean:
	rm -f client.o client server.o server
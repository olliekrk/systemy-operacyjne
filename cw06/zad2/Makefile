all: build test clean

build: server.c client.c utils6.h
	gcc -c server.c -o server.o
	gcc server.o -o server -lrt
	gcc -c client.c -o client.o
	gcc client.o -o client -lrt

server_run:
	./server &

client_run:
	./client

test: server_run client_run

clean:
	rm -f client.o client server.o server
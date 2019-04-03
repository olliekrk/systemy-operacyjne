CC=gcc
CFLAGS=-Wall

all: clean build catcher

build: build_sender build_catcher

build_catcher:
	$(CC) $(CFLAGS) -c catcher.c -o catcher.o
	$(CC) $(CFLAGS) catcher.o -o catcher

build_sender:
	$(CC) $(CFLAGS) -c sender.c -o sender.o
	$(CC) $(CFLAGS) sender.o -o sender

catcher:
#	./catcher KILL &
#	./catcher SIGQUEUE &
	./catcher SIGRT &

clean:
	rm -f sender.o sender catcher.o catcher

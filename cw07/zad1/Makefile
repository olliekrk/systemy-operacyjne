all: build

build: loader.c trucker.c trucker.h common.h
	gcc -c trucker.c -o trucker.o
	gcc -c loader.c -o loader.o
	gcc trucker.o -o trucker
	gcc loader.o -o loader

clean:
	rm -f loader.o trucker.o loader trucker
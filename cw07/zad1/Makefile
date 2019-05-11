all: build

build: loader.c trucker.c trucker.h common.h
	gcc -c trucker.c -o trucker.o
	gcc -c loader.c -o loader.o
	gcc trucker.o -o trucker
	gcc loader.o -o loader

clean:
	rm -f loader.o trucker.o loader trucker

run_tracker: trucker.c trucker.h common.h
	./trucker 100 150 10

run_loader: loader.c loader.h common.h
	./loader 10 1
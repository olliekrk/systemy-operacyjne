all: build

build: loader.c trucker.c trucker.h common.h
	gcc -c trucker.c -o trucker.o
	gcc -c loader.c -o loader.o
	gcc -c factory.c -o factory.o
	gcc trucker.o -o trucker -lpthread -lrt
	gcc loader.o -o loader -lpthread -lrt
	gcc factory.o -o factory -lpthread -lrt

clean:
	rm -f loader.o trucker.o factory.o loader trucker factory

run_trucker:
	./trucker 100 150 10

run_loader:
	./loader 10

run_factory:
	./factory 3 100
all: clean build run

clean:
	rm -f filterman.o filterman

build:
	gcc -c filterman.c -o filterman.o
	gcc filterman.o -o filterman -lpthread -lm

run:
	./filterman 4 block coins.ascii.pgm filter1.txt output.ascii.pgm
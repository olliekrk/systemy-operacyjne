all: build test clean


build:
	gcc -c program.c -o program.o
	gcc program.o -o program

test:
	./program commands.txt

clean:
	rm -f program program.o
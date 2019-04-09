all: build test clean

build:
	gcc -c program.c -o program.o
	gcc program.c -o program

test:
	./program file1.txt
	./program file1.txt file2.txt

clean:
	rm -f program.o program
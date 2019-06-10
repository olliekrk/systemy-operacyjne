all: build run clean

clean:
	rm -f filterman.o filters.o

build: filters.c filterman.c images.h
	rm -rf output times.txt
	mkdir output
	gcc -c filterman.c -o filterman.o
	gcc filterman.o -o filterman -lpthread -lm
	gcc -c filters.c -o filters.o
	gcc filters.o -o filters

run_filters:
	rm -rf filters_dir
	mkdir filters_dir
	./filters 3 6 9 12 22 32

run_tests:
	./tests.sh

run: run_filters run_tests
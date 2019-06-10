CC=gcc
CFLAGS=-Wall

all: clean prepare_dir build test clean_after

build:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) $(CFLAGS) program.o -o program

prepare_dir:
	mkdir example_dir
	mkdir example_dir/other_dir
	mkdir example_dir/other_dir/another_dir
	touch example_dir/example_file
	touch example_dir/other_dir/other_file
	touch example_dir/other_dir/another_dir/another_file
	ln -s /home example_dir/home
	ln -s /usr/lib example_dir/other_dir/lib

test:
	./program ../ ">" "19-03-2019 22:11:11" dir > dir_results.txt
	./program ../ "<" "19-03-2019 22:22:11" nftw > nftw_results.txt

clean:
	rm -rf example_dir

clean_after:
	rm -f program
	rm -f program.o
	rm -rf example_dir
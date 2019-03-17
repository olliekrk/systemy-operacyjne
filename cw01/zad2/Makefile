CC=gcc
CFLAGS=-Wall

all: clean dll_build

dll_build: finder_api.c
	$(CC) $(CFLAGS) finder_api.c -o main_dll -L. -ldl -D DLL -I.

logs_dll_build: finder_api.c
	$(CC) $(CFLAGS) finder_api.c -o main_dll -L. -ldl -D DLL -D LOGS -I.

test: heavy_run mid_run light_run

heavy_run:
	printf "\nHEAVY\n" > raport2.txt
	./main_dll create_table 1000 search_directory / "*" heavy_tmp \
	save_results save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 \
	save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 \
	save_results save_results save_results save_results save_results >> raport2.txt

mid_run:
	printf "\nMID\n" >> raport2.txt
	./main_dll create_table 200 search_directory /home/ "*" mid_tmp \
	save_results save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 \
	save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 \
	save_results save_results save_results save_results save_results >> raport2.txt

light_run:
	printf "\nLIGHT\n" >> raport2.txt
	./main_dll create_table 50 search_directory /usr "*" light_tmp \
	save_results save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 \
	save_results save_results save_results save_results \
	remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 \
	save_results save_results save_results save_results save_results >> raport2.txt

clean:
	rm -f main_dll *_tmp
	rm -rf *.o *.so *.a *.so.*

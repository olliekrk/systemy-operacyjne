CC=gcc
CFLAGS=-Wall
STATIC_BASE=$(CC) $(CFLAGS) main_static.o ../zad1/libfinder.a
SHARED_BASE=$(CC) $(CFLAGS) ../zad2/finder_api.c -lfinder -L ../zad1 -Wl,-rpath,../zad1
DLL_BASE=$(CC) $(CFLAGS) ../zad2/finder_api.c -L. -ldl -D DLL -I.

OPERATIONS=save_results save_results save_results save_results save_results remove_block 0 remove_block 1 remove_block 2 save_results save_results save_results save_results remove_block 0 remove_block 1 remove_block 2 remove_block 3 remove_block 4 remove_block 5 save_results save_results save_results save_results save_results

HEAVY_TASKS=create_table 1000 search_directory / "*" heavy_tmp
MEDIUM_TASKS=create_table 200 search_directory /home/ "*" mid_tmp
LIGHT_TASKS=create_table 50 search_directory /usr "*" light_tmp



# Complete run

all: clean test_3a test_3b



# (3a) Different methods to load the library

static_build:
	$(CC) $(CFLAGS) -c ../zad2/finder_api.c -o main_static.o
	$(STATIC_BASE) -o main_static

shared_build:
	$(SHARED_BASE) -o main_shared

dll_build:
	$(DLL_BASE) -o main_dll

test_3a: static_build shared_build dll_build
	printf "\nSTATIC LIBRARY\n" > results3a.txt
	./main_static $(HEAVY_TASKS) $(OPERATIONS)  >> results3a.txt
	printf "\nSHARED LIBRARY\n" >> results3a.txt
	./main_shared $(HEAVY_TASKS) $(OPERATIONS) >> results3a.txt
	printf "\nDL LIBRARY\n" >> results3a.txt
	./main_dll $(HEAVY_TASKS) $(OPERATIONS) >> results3a.txt



# (3b) Different optimization

optimized_all: static_opt shared_opt dll_opt

static_opt: static_O0 static_O2 static_Os

shared_opt: shared_O0 shared_O2 shared_Os

dll_opt: dll_O0 dll_O2 dll_Os

static_O0: static_build
	$(STATIC_BASE) -O0 -o main_staticO0

static_O2: static_build
	$(STATIC_BASE) -O2 -o main_staticO2

static_Os: static_build
	$(STATIC_BASE) -Os -o main_staticOs

shared_O0: shared_build
	$(SHARED_BASE) -O0 -o main_sharedO0

shared_O2: shared_build
	$(SHARED_BASE) -O2 -o main_sharedO2

shared_Os: shared_build
	$(SHARED_BASE) -Os -o main_sharedOs

dll_O0: dll_build
	$(DLL_BASE) -O0 -o main_dllO0

dll_O2: dll_build
	$(DLL_BASE) -O2 -o main_dllO2

dll_Os: dll_build
	$(DLL_BASE) -Os -o main_dllOs

test_3b: optimized_all
	printf "\n------------- OPTIMIZATION O0 -------------\n" > results3b.txt
	printf "\n------ HEAVY ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO0 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO0 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO0 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ MID ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO0 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO0 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO0 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ LIGHT ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO0 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO0 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO0 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt

	printf "\n------------- OPTIMIZATION O2 -------------\n" >> results3b.txt
	printf "\n------ HEAVY ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO2 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO2 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO2 $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ MID ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO2 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO2 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO2 $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ LIGHT ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticO2 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedO2 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllO2 $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt

	printf "\n------------- OPTIMIZATION Os -------------\n" >> results3b.txt
	printf "\n------ HEAVY ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticOs $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedOs $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllOs $(HEAVY_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ MID ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticOs $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedOs $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllOs $(MEDIUM_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n------ LIGHT ------\n" >> results3b.txt
	printf "\n--- STATIC ---\n" >> results3b.txt
	./main_staticOs $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- SHARED ---\n" >> results3b.txt
	./main_sharedOs $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt
	printf "\n--- DLL ---\n" >> results3b.txt
	./main_dllOs $(LIGHT_TASKS) $(OPERATIONS) >> results3b.txt



# Clean unnecessary files

clean:
	rm -f main_dll *_tmp main_*
	rm -rf *.o *.so *.a *.so.*
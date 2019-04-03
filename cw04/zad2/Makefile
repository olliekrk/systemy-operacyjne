CC=gcc
CFLAGS=-Wall

# to run process in the background
BG_RUN=> /dev/null 2>&1 &

TEST_FILE1=slow_test
TEST_FILE2=fast_test
ARCHIVE=archiwum

all: clean build test

build: build_program build_tester

build_program:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) $(CFLAGS) program.o -o program

build_tester:
	$(CC) $(CFLAGS) -c tester.c -o tester.o
	$(CC) $(CFLAGS) tester.o -o tester

test:
	./tester $(TEST_FILE1) 5 10 30 120 $(BG_RUN)
	./tester $(TEST_FILE2) 1 3 30 60 $(BG_RUN)
	echo "$(TEST_FILE1) 1\n$(TEST_FILE2) 1" > list.txt

clean:
	rm -f program tester
	rm -f program.o tester.o

clean_after: clean
	rm -f list.txt
	rm -f $(TEST_FILE1) $(TEST_FILE2)
	rm -f $(TEST_FILE1)* $(TEST_FILE2)*
	rm -rf $(ARCHIVE)


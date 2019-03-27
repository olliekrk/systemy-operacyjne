CC=gcc
CFLAGS=-Wall

# to run process in the background
BG_RUN=> /dev/null 2>&1 &

TEST_FILE1=slow_test
TEST_FILE2=fast_test
ARCHIVE=archiwum
LIST=list.txt

all: clean build prepare test

build: build_program build_tester

build_program:
	$(CC) $(CFLAGS) -c program.c -o program.o
	$(CC) $(CFLAGS) program.o -o program

build_tester:
	$(CC) $(CFLAGS) -c tester.c -o tester.o
	$(CC) $(CFLAGS) tester.o -o tester

prepare:
	./tester $(TEST_FILE1) 1 1 10000 150 $(BG_RUN)
	./tester $(TEST_FILE2) 1 1 10000 150 $(BG_RUN)
	echo "$(TEST_FILE1) 1\n$(TEST_FILE2) 1" > $(LIST)

test:
	./program $(LIST) 60 a 30 100
#	./program $(LIST) 60 e 30 100

clean:
	rm -f program tester
	rm -f program.o tester.o

clean_after: clean
	rm -f $(LIST)
	rm -f $(TEST_FILE1) $(TEST_FILE2)
	rm -f $(TEST_FILE1)* $(TEST_FILE2)*
	rm -rf $(ARCHIVE)


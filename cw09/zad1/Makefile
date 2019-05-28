all: build run clean

clean:
	rm -f rollercoaster.o rollercoaster

build: rollercoaster.c rollercoaster.h
	gcc -c rollercoaster.c -o rollercoaster.o
	gcc rollercoaster.o -o rollercoaster -lpthread -lm

run:
	./rollercoaster 12 2 5 2
# <TOTAL_PASSENGERS> <TOTAL_TROLLEYS> <TOTAL_CAPACITY> <TOTAL_LOOPS>
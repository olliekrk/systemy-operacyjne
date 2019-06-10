CC = gcc
CFLAGS = -Wall

TARGETS = master slave

.PHONY: all clean run

all: $(TARGETS)

run:
	./master rurka & ./slave rurka 10 & ./slave rurka 10

clean:
	$(RM) $(TARGETS) rurka
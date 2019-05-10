#include "common.h"

/*
 * it has to run after trucker.c has started
 * */

int ITEM_WEIGHT = 0;

// -1 is infinity
int CYCLES_LEFT = -1;

int main(int argc, char **argv) {
    // args parsing
    if (argc < 3) show_error("Invalid number of arguments");
    if (argc > 3) CYCLES_LEFT = strtol(argv[2], NULL, 10);
    ITEM_WEIGHT = strtol(argv[1], NULL, 10);

}
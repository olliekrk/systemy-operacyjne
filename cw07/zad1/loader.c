#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "utils7.h"

int ITEM_WEIGHT = 0;

// -1 is infinity
int CYCLES_LEFT = -1;

int main(int argc, char **argv){
    if (argc < 2) show_error("Invalid number of arguments");

    if(argc > 2) CYCLES_LEFT = strtol(argv[2],NULL,10);

    ITEM_WEIGHT = strtol(argv[1], NULL, 10);

}
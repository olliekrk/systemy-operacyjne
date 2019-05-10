/*
responsibilities:
- managing common memory (conveyor belt CB unloading)
- SIGINT handler for cleanup
- has to be run before loader.c (handle this case in loader.c)
- before closing it should:
    block semaphore,
    clean up CB load
    + in POSIX notify workers to perform their cleanup
*/

#include <stdio.h>
#include "utils7.h"

// weight related
int CONVEYOR_BELT_LOAD = 0;
int TRUCK_LOAD = 0;

// capacity related
int CONVEYOR_BELT_CAP = 0;

int main(int argc, char **argv){
    if (argc < 4)
        show_error("Invalid number of arguments");

    TRUCK_LOAD = strtol(argv[1], NULL, 10);
    CONVEYOR_BELT_LOAD = strtol(argv[2], NULL, 10);
    CONVEYOR_BELT_CAP = strtol(argv[3], NULL, 10);


}
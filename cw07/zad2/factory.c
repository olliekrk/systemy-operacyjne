#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <wait.h>

pid_t *loaders;
int number_of_loaders = 0;

void show_error(const char *message) {
    perror(message);
    exit(1);
}

void cleanup() {
    for (int i = 0; i < number_of_loaders; ++i) {
        wait(NULL);
    }
    free(loaders);
    exit(0);
}

void interrupt_handler(int sig) {
    printf("(!)\tFactory interrupted by SIGINT\n");
    for (int i = 0; i < number_of_loaders; ++i) {
        kill(loaders[i], SIGINT);
    }
    cleanup();
}

int main(int argc, char **argv) {
    if (argc < 3)
        show_error("Invalid number of arguments:\n\t<NUMBER_OF_LOADERS> <LOADER_ITEM_WEIGHT> [<NUMBER_OF_CYCLES>]");

    number_of_loaders = strtol(argv[1], NULL, 10);

    int number_of_cycles = -1;
    if (argc > 3) number_of_cycles = strtol(argv[3], NULL, 10);
    char *cycles;
    if (number_of_cycles == -1) cycles = "-1";
    else cycles = argv[3];

    signal(SIGINT, interrupt_handler);
    atexit(cleanup);

    loaders = calloc(number_of_loaders, sizeof(pid_t));
    for (int i = 0; i < number_of_loaders; i++) {
        pid_t loader = fork();
        if (loader == 0) {
            if (execlp("./loader", "loader", argv[2], cycles, NULL) == -1)
                show_error("Failed to initialize loader(s)");
            return 0;
        } else {
            printf("Starting loader no. %d with PID %d\n", i, loader);
            loaders[i] = loader;
        }
    }

    while (1);
}
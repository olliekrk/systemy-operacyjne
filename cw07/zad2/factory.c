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
    if (argc < 2)
        show_error("Invalid number of arguments:\n\t<NUMBER_OF_LOADERS> [<NUMBER_OF_CYCLES>]");

    int number_of_cycles = -1;
    number_of_loaders = strtol(argv[1], NULL, 10);
    if (argc > 2) number_of_cycles = strtol(argv[2], NULL, 10);

    char *cycles_arg = calloc(10, sizeof(char));
    sprintf(cycles_arg, "%d", number_of_cycles);

    signal(SIGINT, interrupt_handler);
    atexit(cleanup);

    loaders = calloc(number_of_loaders, sizeof(pid_t));
    for (int i = 0; i < number_of_loaders; i++) {
        pid_t loader = fork();
        if (loader == 0) {
            execl("./loader", "loader", cycles_arg, (char *) NULL);
            return 0;
        } else {
            printf("Starting loader no. %d with PID %d\n", i, loader);
            loaders[i] = loader;
        }
    }

    while (loaders) pause();

    return 0;
}
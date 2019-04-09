//
// Created by olliekrk on 09.04.19.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define REQUIRED_ARGS 2
#define PERM_MASK 0666
#define MAX_LENGTH 256

void show_error(char *msg);

int main(int argc, char **argv) {
    if (argc != REQUIRED_ARGS)
        show_error("Invalid number of arguments");

    char *fifo_name = argv[1];
    mkfifo(fifo_name, PERM_MASK);

    FILE *fifo = fopen(fifo_name, "r");
    if (!fifo)
        show_error("Failed to open FIFO");

    char *single_line = calloc(MAX_LENGTH, 1);
    while (fgets(single_line, MAX_LENGTH, fifo))
        write(1, single_line, strlen(single_line));

    fclose(fifo);
    free(single_line);
    return 0;
}

void show_error(char *msg) {
    perror(msg);
    exit(1);
}

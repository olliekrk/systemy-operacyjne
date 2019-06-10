//
// Created by olliekrk on 09.04.19.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define REQUIRED_ARGS 3
#define MAX_LENGTH 256

void show_error(char *msg);

int main(int argc, char **argv) {
    srand(time(NULL));
    if (argc != REQUIRED_ARGS)
        show_error("Invalid number of arguments");

    char *fifo_name = argv[1];
    int N = atoi(argv[2]);


    FILE *fifo = fopen(fifo_name, "w");
    if (!fifo)
        show_error("Failed to open FIFO");

    printf("pid: %d ", getpid());

    char *date_line = calloc(MAX_LENGTH, 1);
    char *single_line = calloc(MAX_LENGTH * 2, 1);
    for (int i = 0; i < N; i++) {
        FILE *date_stream = popen("date", "r");
        fgets(date_line, MAX_LENGTH, date_stream);

        sprintf(single_line, "pid: %d\tdate: %s", getpid(), date_line);
        write(fileno(fifo), single_line, strlen(single_line));
        sleep(1 + rand() % 5);
    }

    fclose(fifo);
    return 0;
}

void show_error(char *msg) {
    perror(msg);
    exit(1);
}

//
// Created by olliekrk on 26.03.19.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <sys/wait.h>

#define SIGNS "abcdefghijklmnopqrstuvwxyz"
#define NO_SIGNS 26
#define DATE_FORMAT "%Y-%m-%d %H:%M:%S"
#define FORMAT_SIZE 19

#define ARGS_REQUIRED 6

void show_error(char *error_message);

void show_errno();

int main(int argc, char **argv) {
    if (argc < ARGS_REQUIRED) show_error("Not enough arguments");
    srand((unsigned int) time(NULL));

    char *f_name = argv[1];
    int pmin = atoi(argv[2]);
    int pmax = atoi(argv[3]);
    size_t bytes = (size_t) atoi(argv[4]);
    int max_runtime = atoi(argv[5]);

    char *random_chars = malloc(bytes + 1);
    char *time_stamp = malloc(FORMAT_SIZE + 1);
    if (!random_chars || !time_stamp) show_error("Error while allocating memory");

    for (int i = 0; i < bytes; i++) {
        random_chars[i] = SIGNS[rand() % NO_SIGNS];
    }
    random_chars[bytes] = '\0';

    int time_elapsed = 0;
    while (time_elapsed < max_runtime) {
        int freq = rand() % ((pmax - pmin) + 1) + pmin;
        sleep((unsigned int) freq);
        time_elapsed += freq;

        FILE *test_file = fopen(f_name, "a");
        if (!test_file) show_errno();

        time_t timer = time(NULL);

        strftime(time_stamp, FORMAT_SIZE, DATE_FORMAT, gmtime(&timer));
        fprintf(test_file, "%d %d %s %s\n", (int) getpid(), freq, time_stamp, random_chars);
        fclose(test_file);
    }
    return 0;
}

void show_errno() {
    show_error(NULL);
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}
//
// Created by olliekrk on 05.04.19.
//
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    char *f1_name = NULL;
    char *f2_name = NULL;
    FILE *destination_file = NULL;
    FILE *source_file = NULL;

    long total_size;

    if (argc > 1) {
        f1_name = argv[1];
        source_file = fopen(f1_name, "r");
        total_size = lseek(fileno(source_file), 0, SEEK_END);
    } else {
        printf("Invalid number of args!\n");
        exit(1);
    }
    if (argc > 2) {
        f2_name = argv[2];
        destination_file = fopen(f2_name, "w");
    }

    // 1 - wlot
    // 0 - wylot
    int fd[2];
    pipe(fd);
    pid_t child_pid = fork();

    if (child_pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execlp("sort", "sort", f1_name, NULL);
    } else {
        close(fd[1]);
        wait(NULL);
        char *content = malloc(sizeof(char) * total_size);
        read(fd[0], content, total_size);

        if (argc > 2)
            fwrite(content, total_size, 1, destination_file);
        else
            printf("%s\n", content);
    }

    if (source_file) fclose(source_file);
    if (destination_file) fclose(destination_file);
}

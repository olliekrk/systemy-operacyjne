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
#include <dirent.h>
#include <errno.h>

#define DATE_FORMAT "_%Y-%m-%d_%H-%M-%S"
#define FORMAT_SIZE 21
#define ARCHIVE_NAME "archiwum"

#define ARCHIVE_MODE 0
#define EXEC_MODE 1

#define ARGS_REQUIRED 4

void show_error(char *error_message);

void show_errno();

char *read_file_content(char *f_name);

int make_monitor(char *f_name, int time_interval, int total_time, int mode);

void archive_copy(char *f_name, int time_interval, int total_time);

void exec_copy(char *f_name, int time_interval, int total_time);

void monitoring(char *list, int time, int mode);

void print_raport(int children);

int main(int argc, char **argv) {
    if (argc < ARGS_REQUIRED) show_error("Not enough arguments");

    char *list = argv[1];
    int total_time = atoi(argv[2]);
    char *mode_name = argv[3];

    int mode = -1;
    if (strcmp(mode_name, "a") == 0) mode = ARCHIVE_MODE;
    else if (strcmp(mode_name, "e") == 0) mode = EXEC_MODE;

    monitoring(list, total_time, mode);

    return 0;
}

void show_errno() {
    show_error(NULL);
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}

char *read_file_content(char *f_name) {
    struct stat *buffer = calloc(1, sizeof(struct stat));
    if (lstat(f_name, buffer) != 0) show_errno();

    FILE *file = fopen(f_name, "r");
    if (!file) show_error("Opening file was unsuccessful");

    char *file_content = malloc((size_t) buffer->st_size + 1);
    if (!file_content) show_error("Error while allocating memory for file content");

    if (fread(file_content, (size_t) buffer->st_size, 1, file) != 1) show_error("Error while reading file");

    // we need to add EOF manually
    file_content[buffer->st_size] = '\0';

    fclose(file);
    free(buffer);
    return file_content;
}

int make_monitor(char *f_name, int time_interval, int total_time, int mode) {
    pid_t child_pid = fork();

    if (child_pid < 0)
        show_error("Creation of child process was unsuccessful");

    if (child_pid == 0) {
        // inside child
        switch (mode) {
            case ARCHIVE_MODE:
                archive_copy(f_name, time_interval, total_time);
                break;
            case EXEC_MODE:
                exec_copy(f_name, time_interval, total_time);
                break;
            default:
                show_error("Undefined copying mode was requested!");
        }
    }

    return 1;
}

void archive_copy(char *f_name, int time_interval, int total_time) {
    int time_elapsed = 0;
    int no_copies = 0;
    struct stat buffer;

    if (lstat(f_name, &buffer) == -1) {
        fprintf(stderr, "Error: '%s' not found\n", f_name);
        exit(0);
    }

    // first time when monitor is created
    time_t time_of_modification = buffer.st_mtime;

    char *file_content = read_file_content(f_name);
    if (!file_content) {
        fprintf(stderr, "Error while reading file\n");
        exit(0);
    }

    char *copy_name = malloc(strlen(f_name) + FORMAT_SIZE + 1);
    if (!copy_name) {
        fprintf(stderr, "Error while allocating memory\n");
        exit(0);
    }

    strcpy(copy_name, f_name);

    // code responsible for creating archive
    DIR *dir = opendir(ARCHIVE_NAME);
    if (!dir && ENOENT == errno) {
        int status = mkdir(ARCHIVE_NAME, 0777);
        if (status != 0) show_errno();
    }

    while ((time_elapsed += time_interval) <= total_time) {
        sleep((unsigned int) time_interval);

        // creating back-up copy
        if (buffer.st_mtime > time_of_modification) {
            strftime(&copy_name[strlen(f_name)], FORMAT_SIZE, DATE_FORMAT, gmtime(&time_of_modification));

            char *copy_location = malloc(strlen(ARCHIVE_NAME) + 1 + strlen(copy_name) + 1);
            sprintf(copy_location, "%s/%s", ARCHIVE_NAME, copy_name);

            FILE *copy = fopen(copy_location, "w");
            fwrite(file_content, 1, strlen(file_content), copy);
            fclose(copy);
            free(copy_location);
            free(file_content);

            // refresh content: save current content to memory)
            file_content = read_file_content(f_name);
            if (!file_content) {
                fprintf(stderr, "Error while reading file\n");
                exit(no_copies);
            }

            // check time of modification
            time_of_modification = buffer.st_mtime;
            no_copies++;
        }

        if (lstat(f_name, &buffer) == -1) {
            fprintf(stderr, "Original file: '%s' not found\n", f_name);
            break;
        }
    }

    free(file_content);
    free(copy_name);
    exit(no_copies);
}

void exec_copy(char *f_name, int time_interval, int total_time) {
    int time_elapsed = 0;
    int no_copies = 0;
    struct stat buffer;

    if (lstat(f_name, &buffer) == -1) {
        fprintf(stderr, "Error: '%s' not found\n", f_name);
        exit(0);
    }

    // first time when monitor is created
    time_t time_of_modification = buffer.st_mtime;

    char *copy_name = malloc(strlen(f_name) + FORMAT_SIZE + 1);
    if (!copy_name) {
        fprintf(stderr, "Error while allocating memory\n");
        exit(0);
    }

    strcpy(copy_name, f_name);

    while ((time_elapsed += time_interval) <= total_time) {
        strftime(&copy_name[strlen(f_name)], FORMAT_SIZE, DATE_FORMAT, gmtime(&time_of_modification));

        // modification has been made or it is first loop
        if (buffer.st_mtime > time_of_modification || no_copies == 0) {
            time_of_modification = buffer.st_mtime;

            // making copying process
            pid_t child_pid = vfork();
            if (child_pid < 0) {
                fprintf(stderr, "Failed to create child process to execute `cp`\n");
            } else if (child_pid == 0) {
                execlp("cp", "cp", f_name, copy_name, NULL);
            } else {
                int status;
                wait(&status);
                if (status != 0) {
                    fprintf(stderr, "Error while executing copy command\n");
                } else {
                    no_copies++;
                }
            }
        }

        if (lstat(f_name, &buffer) == -1) {
            fprintf(stderr, "Original file: '%s' not found\n", f_name);
            break;
        }

        sleep((unsigned int) time_interval);
    }

    free(copy_name);
    exit(no_copies);
}

void monitoring(char *list, int time, int mode) {
    char *list_content = read_file_content(list);
    if (!list_content) show_error("Failed to load list content");

    int time_interval;
    char *f_name = malloc(255);
    if (!f_name) show_errno();

    int no_children = 0;
    char *line = list_content;
    while (line) {
        // to get one by one line from list_content
        char *next_line = strchr(line, '\n');
        if (next_line) next_line[0] = '\0';

        if (sscanf(line, "%255s %d", f_name, &time_interval) == 2) {
            no_children += make_monitor(f_name, time_interval, time, mode);
        } else {
            fprintf(stderr, "Invalid line: %s, incorrect format", line);
        }

        if (next_line) next_line[0] = '\n';
        if (next_line && strcmp(next_line, "\n") != 0) {
            line = &next_line[1];
        } else {
            line = NULL;
        }
    }
    free(list_content);
    free(f_name);

    print_raport(no_children);
}

void print_raport(int no_children) {
    for (int i = 0; i < no_children; i++) {
        int exit_status;
        pid_t finished_pid = wait(&exit_status);

        // non zero if child terminated normally, non zero is true
        if (WIFEXITED(exit_status))
            printf("Process of PID: %d has made %d back-up copies\n", (int) finished_pid, WEXITSTATUS(exit_status));
        else
            printf("Process of PID: %d has made unknown number of back-up copies\n", (int) finished_pid);
    }
}

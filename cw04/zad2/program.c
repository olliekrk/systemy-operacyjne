//
// Created by olliekrk on 02.04.19.
//
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

#define DATE_FORMAT "_%Y-%m-%d_%H-%M-%S"
#define FORMAT_SIZE 21
#define ARCHIVE_NAME "archiwum"
#define MAX_INPUT_LENGTH 20

#define ARGS_REQUIRED 2

int local_end_active = 0;
int local_stop_active = 0;

typedef struct {
    char *f_name;
    pid_t child_pid;
    int time_interval;
    int stop_active;
} child_process;

typedef struct {
    child_process *children;
    int no_children;
} children_set;

//new functions
static void receive_SIGINT(int sig);

static void receive_SIGUSR(int sig);

void modify_SIGINT_behaviour();

void modify_SIGUSR_behaviour();

//previous functions to monitor
int activate_monitor(char *f_name, int time_interval);

//monitoring logic
void archive_copy(char *f_name, int time_interval);

//read list file and start monitoring each file
children_set *activate_monitoring(char *list);

//print summary raport
void print_raport(children_set *childrenSet);

//reading files
int count_lines(char *file_content);

char *read_file_content(char *f_name);

//interface
void list_request(children_set *childrenSet);

void stop_one_request(children_set *childrenSet, pid_t pid);

void stop_all_request(children_set *childrenSet);

void start_one_request(children_set *childrenSet, pid_t pid);

void start_all_request(children_set *childrenSet);

void end_request(children_set *childrenSet);

//errors & cleanups
void free_children_set(children_set *childrenSet);

void show_error(char *error_message);

void show_errno();

int main(int argc, char **argv) {
    if (argc < ARGS_REQUIRED) show_error("Not enough arguments");
    char *list = argv[1];
    modify_SIGINT_behaviour();

    children_set *childrenSet = activate_monitoring(list);
    list_request(childrenSet);

    while (!local_end_active) {
        int pid;
        printf("Waiting for input:\n");
        char *next_request = calloc(MAX_INPUT_LENGTH, sizeof(char));
        fgets(next_request, MAX_INPUT_LENGTH, stdin);

        if (strcmp(next_request, "LIST\n") == 0) {
            list_request(childrenSet);
        } else if (strcmp(next_request, "STOP ALL\n") == 0) {
            stop_all_request(childrenSet);
        } else if (strncmp(next_request, "STOP ", 5) == 0) {
            pid = atoi(next_request + 5);
            stop_one_request(childrenSet, pid);
        } else if (strcmp(next_request, "START ALL\n") == 0) {
            start_all_request(childrenSet);
        } else if (strncmp(next_request, "START ", 6) == 0) {
            pid = atoi(next_request + 6);
            start_one_request(childrenSet, pid);
        } else if (local_end_active || strcmp(next_request, "END\n") == 0) {
            end_request(childrenSet);
        } else {
            fprintf(stderr, "Invalid input command: %s\n", next_request);
        }
    }
    return 0;
}

void list_request(children_set *childrenSet) {
    for (int i = 0; i < childrenSet->no_children; i++) {
        printf("PID: %d\tACTIVE: %d\tFILE: %s\tINTERVAL: %d s\n",
               childrenSet->children[i].child_pid,
               !childrenSet->children[i].stop_active,
               childrenSet->children[i].f_name,
               childrenSet->children[i].time_interval);
    }
}

void stop_one_request(children_set *childrenSet, pid_t pid) {
    child_process *child = NULL;
    for (int i = 0; i < childrenSet->no_children; i++) {
        if (childrenSet->children[i].child_pid == pid) {
            child = &childrenSet->children[i];
            break;
        }
    }

    if (child) {
        kill(child->child_pid, SIGUSR1);
        child->stop_active = 1;
        printf("Process of PID: %d has been stopped.\n", pid);
    } else {
        fprintf(stderr, "None active child process has PID: %d.\n", pid);
    }
}

void stop_all_request(children_set *childrenSet) {
    for (int i = 0; i < childrenSet->no_children; i++) {
        child_process *child = &childrenSet->children[i];

        if (child) {
            kill(child->child_pid, SIGUSR1);
            child->stop_active = 1;
            printf("Process of PID: %d has been stopped.\n", child->child_pid);
        }
    }
}

void start_one_request(children_set *childrenSet, pid_t pid) {
    child_process *child = NULL;
    for (int i = 0; i < childrenSet->no_children; i++) {
        if (childrenSet->children[i].child_pid == pid) {
            child = &childrenSet->children[i];
            break;
        }
    }

    if (child) {
        kill(child->child_pid, SIGUSR1);
        child->stop_active = 0;
        printf("Process of PID: %d has started.\n", pid);
    }
}

void start_all_request(children_set *childrenSet) {
    for (int i = 0; i < childrenSet->no_children; i++) {
        child_process *child = &childrenSet->children[i];

        if (child) {
            kill(child->child_pid, SIGUSR1);
            child->stop_active = 0;
            printf("Process of PID: %d has started.\n", child->child_pid);
        }
    }
}

void end_request(children_set *childrenSet) {
    print_raport(childrenSet);
    free_children_set(childrenSet);
    exit(0);
}

static void receive_SIGINT(int sig) {
    if (sig == SIGINT)
        local_end_active = 1;
}

static void receive_SIGUSR(int sig) {
    switch (sig) {
        case SIGUSR1:
            local_stop_active = !local_stop_active;
            break;
        case SIGUSR2:
            local_end_active = 1;
            break;
        default:
            show_error("Invalid signal received");
    }
}

void modify_SIGINT_behaviour() {
    struct sigaction action_SIGINT;
    sigemptyset(&action_SIGINT.sa_mask);
    action_SIGINT.sa_flags = 0;
    action_SIGINT.sa_handler = receive_SIGINT;
    sigaction(SIGINT, &action_SIGINT, NULL);
}

void modify_SIGUSR_behaviour() {
    struct sigaction action_SIGUSR;
    action_SIGUSR.sa_handler = receive_SIGUSR;
    action_SIGUSR.sa_flags = 0;
    sigemptyset(&action_SIGUSR.sa_mask);

    sigaction(SIGUSR1, &action_SIGUSR, NULL);
    sigaction(SIGUSR2, &action_SIGUSR, NULL);
}

pid_t activate_monitor(char *f_name, int time_interval) {
    pid_t child_pid = fork();

    if (child_pid < 0)
        show_error("Creation of child process was unsuccessful");

    if (child_pid == 0) {
        // inside child
        modify_SIGUSR_behaviour();
        archive_copy(f_name, time_interval);
    }

    return child_pid;
}

children_set *activate_monitoring(char *list) {
    char *list_content = read_file_content(list);

    if (!list_content) show_error("Failed to load list content");

    //calculating how many children do we need
    int processes_needed = count_lines(list_content);
    child_process *children = calloc(processes_needed, sizeof(child_process));
    if (!children) show_error("Error while allocating memory for child process structures");

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
            pid_t child_pid = activate_monitor(f_name, time_interval);
            if (child_pid != 0) {
                //getting next child from the set
                child_process *child = &children[no_children];
                child->f_name = calloc(strlen(f_name) + 1, sizeof(char));

                //setting child fields
                strcpy(child->f_name, f_name);
                child->child_pid = child_pid;
                child->time_interval = time_interval;

                no_children++;
            }
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

    //in case we created less than we assumed (some lines were incorrect)
    children = realloc(children, no_children * sizeof(child_process));

    free(list_content);
    free(f_name);

    //setting result set fields
    children_set *childrenSet = calloc(1, sizeof(childrenSet));
    childrenSet->children = children;
    childrenSet->no_children = no_children;
    return childrenSet;
}

void archive_copy(char *f_name, int time_interval) {
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
        mkdir(ARCHIVE_NAME, 0777);
    }
    closedir(dir);

    while (1) {
        sleep((unsigned int) time_interval);

        if (local_end_active) break;
        if (local_stop_active) continue;

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

void print_raport(children_set *childrenSet) {
    for (int i = 0; i < childrenSet->no_children; i++) {
        int exit_status;
        kill(childrenSet->children[i].child_pid, SIGINT);
        pid_t finished_pid = waitpid(childrenSet->children[i].child_pid, &exit_status, 0);
        // non zero if child terminated normally, non zero is true
        if (WIFEXITED(exit_status))
            printf("Process of PID: %d has made %d back-up copies\n", (int) finished_pid, WEXITSTATUS(exit_status));
        else
            printf("Process of PID: %d has made unknown number of back-up copies\n", (int) finished_pid);
    }
}

int count_lines(char *file_content) {
    int lines = 0;
    char *n_line = file_content;
    while (n_line != NULL) {
        n_line = strchr(n_line, '\n');
        if (n_line != NULL) {
            lines++;
            n_line++;
        }
    }
    return lines;
}

char *read_file_content(char *f_name) {
    struct stat *buffer = calloc(1, sizeof(struct stat));
    if (lstat(f_name, buffer) != 0) show_errno();

    FILE *file = fopen(f_name, "r");
    if (!file) show_error("Opening file was unsuccessful");

    char *file_content = malloc(buffer->st_size + 1);
    if (!file_content) show_error("Error while allocating memory for file content");

    if (fread(file_content, (size_t) buffer->st_size, 1, file) != 1) show_error("Error while reading file");

    // we need to add end-of-line manually
    file_content[buffer->st_size] = '\0';

    fclose(file);
    free(buffer);
    return file_content;
}

void free_children_set(children_set *childrenSet) {
    for (int i = 0; i < childrenSet->no_children; i++)
        free(childrenSet->children[i].f_name);
    free(childrenSet);
}

void show_errno() {
    show_error(NULL);
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}

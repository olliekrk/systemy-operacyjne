#define NO_REQUIRED_ARGS 2
#define BASE_PATH "."

#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// processes
#include <unistd.h>
#include <sys/wait.h>

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}

void show_errno() {
    perror(NULL);
    exit(1);
}

void child_search(const char *path, const char *relative_path) {
    pid_t child = fork();
    if (child < 0) show_error("Creation of a child process was unsuccessful");
    else if (child == 0) {
        printf("PPID: %d\tPID: %d\tPATH: %s\n", getppid(), getpid(), relative_path);
        execl("/bin/ls", "ls", "-l", "--color=auto", path, (char *) NULL);
    } else {
        wait(NULL);
    }
}

void dir_search(const char *dir_path, const char *relative_path) {
    child_search(dir_path, relative_path);

    DIR *dir = opendir(dir_path);
    if (!dir) show_error("Error while opening directory stream");

    struct dirent *current;
    struct stat buffer;

    while ((current = readdir(dir))) {
        if (strcmp(current->d_name, ".") == 0 || strcmp(current->d_name, "..") == 0)
            continue;

        char *d_path = calloc(strlen(dir_path) + strlen(current->d_name) + 2, sizeof(char));
        char *rel_path = calloc(strlen(relative_path) + strlen(current->d_name) + 2, sizeof(char));
        if (!d_path || !rel_path) show_error("Error while allocating memory for buffers");

        if (strcmp(".", relative_path) == 0) {
            sprintf(rel_path, "%s", current->d_name);
            sprintf(d_path, "%s/%s", dir_path, current->d_name);
        } else {
            sprintf(rel_path, "%s/%s", rel_path, current->d_name);
            sprintf(d_path, "%s/%s", dir_path, current->d_name);
        }

        if (lstat(d_path, &buffer) < 0) show_errno();

        if (S_ISDIR(buffer.st_mode))
            dir_search(d_path, rel_path);

        free(d_path);
        free(rel_path);
    }

    if (closedir(dir) != 0) show_errno();
}

int main(int argc, char *argv[]) {
    if (argc < NO_REQUIRED_ARGS)
        show_error("Not enough arguments were passes into program");

    char *dir_path = realpath(argv[1], NULL);
    if (!dir_path) show_errno();

    dir_search(dir_path, BASE_PATH);

    free(dir_path);
    return 0;
}

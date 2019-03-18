//
// Created by olliekrk on 17.03.19.
//
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <memory.h>

#define DATE_FORMAT "%d-%d-%d %d:%d:%d"

#define EARLIER 0
#define NOW 1
#define LATER 2

int check_time(time_t modification_time, time_t checked_time, int i);

void print_info(FILE *stream, struct stat *buffer, char *f_path) {
    char *type = "unknown type";
    if (S_ISREG(buffer->st_mode)) {
        type = "file";
    } else if (S_ISDIR(buffer->st_mode)) {
        type = "dir";
    } else if (S_ISCHR(buffer->st_mode)) {
        type = "char dev";
    } else if (S_ISBLK(buffer->st_mode)) {
        type = "block dev";
    } else if (S_ISFIFO(buffer->st_mode)) {
        type = "fifo";
    } else if (S_ISLNK(buffer->st_mode)) {
        type = "slink";
    } else if (S_ISSOCK(buffer->st_mode)) {
        type = "sock";
    }

    fprintf(stream, "%s\t%s\t%ld\t%ld\t%ld\n", f_path, type, buffer->st_size, buffer->st_atime, buffer->st_ctime);
}

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}

void show_errno() {
    perror(NULL);
    exit(1);
}

void search_dir(char *dir_path, int comparision_type, time_t time) {
    DIR *dir = opendir(dir_path);
    if (!dir) show_error("Error while opening directory stream");

    struct dirent *current = readdir(dir);
    struct stat buffer;

    while (current != NULL) {
        if (strcmp(current->d_name, ".") == 0 || strcmp(current->d_name, "..") == 0) {
            current = readdir(dir);
            continue;
        }

        char *f_path = calloc(strlen(dir_path) + strlen(current->d_name) + 2, sizeof(char));
        if (!f_path) show_error("Error while allocating memory for buffer");
        sprintf(f_path, "%s/%s", dir_path, current->d_name);

        if (lstat(f_path, &buffer) < 0)
            show_errno();

        printf("mod time: %ld\n", buffer.st_mtime);
        if (check_time(buffer.st_mtime, time, comparision_type) == 0)
            print_info(stdout, &buffer, f_path);

        if (S_ISDIR(buffer.st_mode))
            search_dir(f_path, comparision_type, time);

        free(f_path);
        current = readdir(dir);
    }

    if (closedir(dir) != 0) show_errno();
}

int check_time(time_t modification_time, time_t checked_time, int comparision_type) {
    switch (comparision_type) {
        case EARLIER:
            if (difftime(modification_time, checked_time) < 0) return 0;
            else return -1;
        case NOW:
            if (difftime(modification_time, checked_time) == 0) return 0;
            else return -1;
        case LATER:
            if (difftime(modification_time, checked_time) > 0) return 0;
            else return -1;
        default:
            show_error("Invalid time comparision type");
    }
}

void search_dir_nftw(char *dir_path, int comparision_type, time_t time) {
    //todo search with nftw
}

time_t convert_to_time(char *timestamp) {
    struct tm time = {0};
    sscanf(timestamp, DATE_FORMAT,
           &time.tm_year,
           &time.tm_mon,
           &time.tm_mday,
           &time.tm_hour,
           &time.tm_min,
           &time.tm_sec);

    //todo timestamps works wrong
    time.tm_mon -= 1; //because months are from range 0-11
    time.tm_year -= 1990; //because tm_year is counted since year 1990

    return mktime(&time);
}

int main(int argc, char *argv[]) {
//    char *dir_path = argv[0];
//    char *comparision_sign = argv[1];
//    char *timestamp = argv[2];
    search_dir(".", LATER, convert_to_time("2018-03-20 00:00:00"));

    return 0;
}

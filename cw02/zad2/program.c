//
// Created by olliekrk on 17.03.19.
//
#define _XOPEN_SOURCE 500

#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATE_FORMAT "%d-%m-%Y %H:%M:%S"
#define OPENFD_LIMIT 20
#define NO_REQUIRED_ARGS 5

#define EARLIER 0
#define NOW 1
#define LATER 2

int global_comparision_time;
time_t global_time;

void show_error(char *error_message) {
    perror(error_message);
    exit(1);
}

void show_errno() {
    perror(NULL);
    exit(1);
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
    return -1;
}

time_t convert_to_time(char *timestamp) {
    struct tm time;
    if (strptime(timestamp, DATE_FORMAT, &time) == NULL) show_error("Error while parsing timestamp");
    return mktime(&time);
}

char *convert_to_str(time_t time) {
    time_t arg = time;
    char *converted = calloc(20, sizeof(char));
    strftime(converted, 20, DATE_FORMAT, localtime(&arg));
    return converted;
}

void print_info(FILE *stream, const struct stat *buffer, const char *f_path) {
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

    fprintf(stream, "%-100s\t%-10s\t%-10ld\t%-20s\t%-20s\n",
            f_path, type, buffer->st_size,
            convert_to_str(buffer->st_atime),
            convert_to_str(buffer->st_ctime));
}

void search_dir(char *dir_path) {
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

        if (check_time(buffer.st_mtime, global_time, global_comparision_time) == 0)
            print_info(stdout, &buffer, f_path);

        if (S_ISDIR(buffer.st_mode))
            search_dir(f_path);

        free(f_path);
        current = readdir(dir);
    }

    if (closedir(dir) != 0) show_errno();
}

int nftw_fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    // the only safe way to exit out of a tree walk is to return a nonzero value from fn
    if (ftwbuf->level == 0) return 0;
    if (check_time(sb->st_mtime, global_time, global_comparision_time) == -1) return 0;
    print_info(stdout, sb, fpath);
    return 0;
}

void search_dir_nftw(char *dir_path) {
    nftw(dir_path, nftw_fn, OPENFD_LIMIT, FTW_PHYS); //FTW_PHYS is a flag that tells not to follow symlinks
}

int main(int argc, char *argv[]) {
    if (argc < NO_REQUIRED_ARGS) {
        show_error(
                "Not enough arguments were passes into program.\n"
                "Please follow syntax as below:\n"
                "./program <directory_path> <comparision_char> <date> <search_type: nftw / dir>\n");
    }
    char *dir_path = realpath(argv[1], NULL);
    if(!dir_path) show_errno();
    char *comparision_type = argv[2];
    global_time = convert_to_time(argv[3]);
    char *search_type = argv[4];

    if (strcmp(comparision_type, "<") == 0) {
        global_comparision_time = EARLIER;
    } else if (strcmp(comparision_type, "=") == 0) {
        global_comparision_time = NOW;
    } else if (strcmp(comparision_type, ">") == 0) {
        global_comparision_time = LATER;
    } else show_error("Unknown comparision operator");

    if (strcmp(search_type, "nftw") == 0) {
        search_dir_nftw(dir_path);
    } else if (strcmp(search_type, "dir") == 0) {
        search_dir(dir_path);
    } else show_error("Unknown search type");

    free(dir_path);
    return 0;
}

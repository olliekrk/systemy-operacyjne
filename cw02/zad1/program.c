#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#define RANDOMS "abcdefghijklmnoprstuvwxyzABCDEFGHIJKLMNOPRSTUVWXYZ0123456789"
#define RSIZE 60

typedef struct {
    clock_t real_start;
    struct tms start;
    double t_real;
    double t_user;
    double t_sys;
} t_measurement;

void show_errno();

void show_error(char *error_msg);

void generate_records(char *file_path, unsigned int no_records, unsigned int record_length) {
    FILE *records_file = fopen(file_path, "w");
    if (!records_file) show_errno();

    FILE *generator = fopen("/dev/urandom", "r");
    if (!generator) show_errno();

    time_t t;
    unsigned int seed = (unsigned int) time(&t);
    srand(seed);

    for (int i = 0; i < no_records; i++) {
        for (int j = 0; j < record_length - 1; j++) {
            fputc(RANDOMS[rand() % RSIZE], records_file);
        }
        fputc('\n', records_file);
    }

    if (fclose(records_file) != 0) show_errno();
    if (fclose(generator) != 0) show_errno();
}

void sort_records_sys(char *file_path, unsigned int no_records, unsigned int record_length) {
    int records_fd = open(file_path, O_RDWR);
    if (records_fd < 0) show_error("Error while opening file with records");

    unsigned char *smallest_record = calloc(record_length, sizeof(char));
    unsigned char *current_record = calloc(record_length, sizeof(char));
    if (!smallest_record || !current_record) show_error("Error while allocating memory for buffers");


    int smallest_i;
    for (int i = 0; i < no_records; i++) {
        smallest_i = i;
        lseek(records_fd, i * record_length, SEEK_SET);
        read(records_fd, smallest_record, record_length * sizeof(char));
        if (!smallest_record) {
            show_errno();
            exit(1);
        }
        for (int j = i + 1; j < no_records; j++) {
            lseek(records_fd, j * record_length, SEEK_SET);
            read(records_fd, current_record, record_length * sizeof(char));
            if (current_record[0] < smallest_record[0]) {
                smallest_i = j;
                lseek(records_fd, smallest_i * record_length, SEEK_SET);
                read(records_fd, smallest_record, record_length * sizeof(char));
            }
        }

        // save overridden record to a buffer
        lseek(records_fd, i * record_length, SEEK_SET);
        read(records_fd, current_record, record_length * sizeof(char));

        // write down smallest record
        lseek(records_fd, i * record_length, SEEK_SET);
        write(records_fd, smallest_record, record_length * sizeof(char));

        // save overridden record under free index
        lseek(records_fd, smallest_i * record_length, SEEK_SET);
        write(records_fd, current_record, record_length * sizeof(char));

    }

    free(smallest_record);
    free(current_record);
    close(records_fd);
}

void sort_records_lib(char *file_path, unsigned int no_records, unsigned int record_length) {
    FILE *records = fopen(file_path, "r+");
    if (!records) show_error("Error while opening file with records");

    unsigned char *smallest_record = calloc(record_length, sizeof(char));
    unsigned char *current_record = calloc(record_length, sizeof(char));
    if (!smallest_record || !current_record) show_error("Error while allocating memory for buffers");

    int smallest_i;
    for (int i = 0; i < no_records; i++) {
        smallest_i = i;
        fseek(records, i * record_length, SEEK_SET);
        fread(smallest_record, record_length * sizeof(char), 1, records);

        for (int j = i + 1; j < no_records; j++) {
            fseek(records, j * record_length, SEEK_SET);
            fread(current_record, record_length * sizeof(char), 1, records);
            if (current_record[0] < smallest_record[0]) {
                smallest_i = j;
                fseek(records, smallest_i * record_length, SEEK_SET);
                fread(smallest_record, record_length * sizeof(char), 1, records);
            }
        }

        // save overridden record to a buffer
        fseek(records, i * record_length, SEEK_SET);
        fread(current_record, record_length * sizeof(char), 1, records);

        // write down smallest record
        fseek(records, i * record_length, SEEK_SET);
        fwrite(smallest_record, record_length * sizeof(char), 1, records);

        // save overridden record under free index
        fseek(records, smallest_i * record_length, SEEK_SET);
        fwrite(current_record, record_length * sizeof(char), 1, records);

    }

    free(current_record);
    free(smallest_record);
    fclose(records);
}

void copy_records_sys(char *file_path, char *copy_path, unsigned int no_records, unsigned int record_length) {
    mode_t mode = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;
    int records_fd = open(file_path, O_RDONLY);
    int copy_fd = open(copy_path, mode);
    if (records_fd < 0 || copy_fd < 0)
        show_error("Error while opening files before copying");

    unsigned char *buffer = calloc(record_length, sizeof(char));
    if (!buffer) show_error("Error while allocating memory for buffer");

    for (int i = 0; i < no_records; i++) {
        read(records_fd, buffer, record_length * sizeof(char));
        write(copy_fd, buffer, record_length * sizeof(char));
    }

    free(buffer);
    close(records_fd);
    close(copy_fd);
}

void copy_records_lib(char *file_path, char *copy_path, unsigned int no_records, unsigned int record_length) {
    FILE *records = fopen(file_path, "r");
    FILE *copy = fopen(copy_path, "w");

    if (!records || !copy)
        show_error("Error while opening files before copying");

    unsigned char *buffer = calloc(record_length, sizeof(char));
    if (!buffer) show_error("Error while allocating memory for buffer");

    for (int i = 0; i < no_records; i++) {
        fread(buffer, record_length * sizeof(char), 1, records);
        fwrite(buffer, record_length * sizeof(char), 1, copy);
    }

    free(buffer);
    fclose(records);
    fclose(copy);
}

int main(int argc, char *argv[]) {
    for (int arg_index = 1; arg_index < argc; arg_index++) {
        if (strcmp("generate", argv[arg_index]) == 0) {
            char *file_path = argv[++arg_index];
            unsigned int no_records = (unsigned int) atoi(argv[++arg_index]);
            unsigned int record_length = (unsigned int) atoi(argv[++arg_index]);
            generate_records(file_path, no_records, record_length);

        } else if (strcmp("sort", argv[arg_index]) == 0) {
            char *file_path = argv[++arg_index];
            unsigned int no_records = (unsigned int) atoi(argv[++arg_index]);
            unsigned int record_length = (unsigned int) atoi(argv[++arg_index]);
            char *type = argv[++arg_index];

            if (strcmp("sys", type) == 0) sort_records_sys(file_path, no_records, record_length);
            else if (strcmp("lib", type) == 0) sort_records_lib(file_path, no_records, record_length);
            else show_error("Unknown type of operation!");

        } else if (strcmp("copy", argv[arg_index]) == 0) {
            char *file_path = argv[++arg_index];
            char *copy_path = argv[++arg_index];
            unsigned int no_records = (unsigned int) atoi(argv[++arg_index]);
            unsigned int record_length = (unsigned int) atoi(argv[++arg_index]);
            char *type = argv[++arg_index];

            if (strcmp("sys", type) == 0) copy_records_sys(file_path, copy_path, no_records, record_length);
            else if (strcmp("lib", type) == 0) copy_records_lib(file_path, copy_path, no_records, record_length);
            else show_error("Unknown type of operation!");

        } else show_error("Unknown command!");
    }
    return 0;
}

void show_errno() {
    show_error(strerror(errno));
}

void show_error(char *error_msg) {
    fprintf(stderr, "%s\n", error_msg);
    exit(1);
}

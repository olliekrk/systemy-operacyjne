//
// Created by olliekrk on 12.03.19.
//

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

//static and shared libraries need this header file to be included
#ifndef DLL

#include "../zad1/finder.h"

#endif

typedef struct {
    clock_t real_start;
    struct tms start;
    double t_real;
    double t_user;
    double t_sys;
} t_measurement;

double ticks_to_seconds(clock_t t_start, clock_t t_end);

void reset_measurements(t_measurement *measurement, int n);

void start_measurement(t_measurement *tm);

void stop_measurement(t_measurement *tm);

void save_measurement(FILE *stream, t_measurement *tm, char *operation_name);

void add_measurements(t_measurement *tm, t_measurement *save_tm, t_measurement *dealloc_tm);

int main(int arg_count, char **argv) {
#ifdef DLL
    dlerror();
    void *library = dlopen("../zad1/libfinder.so", RTLD_LAZY);

    if (!library) {
        fprintf(stderr, "error while opening DLL library!\n");
        exit(1);
    }
    void (*set_directory)(char *) = dlsym(library, "set_directory");

    void (*set_temporary_file)(char *) = dlsym(library, "set_temporary_file");

    void (*set_searched_file)(char *) = dlsym(library, "set_searched_file");

    void (*configure_environment)(char *, char *, char *) = dlsym(library, "configure_environment");

    void (*allocate_result_blocks)(size_t) = dlsym(library, "allocate_result_blocks");

    void (*free_environment)() = dlsym(library, "free_environment");

    void (*search)() = dlsym(library, "search");

    unsigned int (*save_results)() = dlsym(library, "save_results");

    void (*delete_block)(unsigned int) = dlsym(library, "delete_block");

    char* (*get_block)(unsigned int) = dlsym(library, "get_block");

    if(dlerror()!=NULL){
        fprintf(stderr, "error while loading DLL library content!\n");
        exit(1);
    }
#endif
    // default parameters
    configure_environment(".", "*", "default_tmp");

    t_measurement measurement[4];
    reset_measurements(measurement, 4);

    t_measurement *search_tm = &measurement[0];
    t_measurement *save_tm = &measurement[1];
    t_measurement *dealloc_tm = &measurement[2];
    t_measurement *seq_tm = &measurement[3];

    int arg_index = 1;
    while (arg_index < arg_count) {
        char *arg = argv[arg_index];
        if (strcmp(arg, "create_table") == 0) {
            if (arg_index + 1 >= arg_count) { goto invalid_number_of_arguments; }

            long blocks_size = strtol(argv[++arg_index], NULL, 10);
            if (blocks_size < 0) { goto invalid_argument; }

            allocate_result_blocks((size_t) blocks_size);

#ifdef LOGS
            printf("successfully allocated %d memory blocks\n", blocks_size);
#endif
        } else if (strcmp(arg, "search_directory") == 0) {
            if (arg_index + 3 >= arg_count) { goto invalid_number_of_arguments; }

            set_directory(argv[++arg_index]);
            set_searched_file(argv[++arg_index]);
            set_temporary_file(argv[++arg_index]);

            start_measurement(search_tm);
            search();
            stop_measurement(search_tm);

#ifdef LOGS
            printf("search completed successfully\n");
#endif

        } else if (strcmp(arg, "save_results") == 0) {

            start_measurement(save_tm);
            save_results();
            stop_measurement(save_tm);

#ifdef LOGS
            printf("search results successfully saved\n");
#endif
        } else if (strcmp(arg, "remove_block") == 0) {
            if (arg_index + 1 >= arg_count) { goto invalid_number_of_arguments; }

            long remove_index = strtol(argv[++arg_index], NULL, 10);
            if (remove_index < 0) { goto invalid_argument; }

            start_measurement(dealloc_tm);
            delete_block((unsigned int) remove_index);
            stop_measurement(dealloc_tm);

#ifdef LOGS
            printf("block successfully deleted from index: %d\n", remove_index);
#endif
        } else if (strcmp(arg, "get_block") == 0) {
            if (arg_index + 1 >= arg_count) { goto invalid_number_of_arguments; }

            long get_index = strtol(argv[++arg_index], NULL, 10);
            if (get_index < 0) { goto invalid_argument; }

            printf("%s", get_block((unsigned int) get_index));
#ifdef LOGS
            printf("block successfully accessed at index: %d\n", get_index);
#endif
        } else {
            fprintf(stderr, "unknown operation: %s!\n", arg);
            exit(1);
        }
        arg_index++;

        if (false) {
            invalid_argument:
            {
                fprintf(stderr, "invalid argument was passed!\n");
                exit(1);
            }
            invalid_number_of_arguments:
            {
                fprintf(stderr, "invalid number of arguments were passed!\n");
                exit(1);
            }
        }
    }
    add_measurements(seq_tm, save_tm, dealloc_tm);
    save_measurement(stdout, search_tm, "SEARCH");
    save_measurement(stdout, save_tm, "SAVE");
    save_measurement(stdout, dealloc_tm, "DEALLOCATE");
    save_measurement(stdout, seq_tm, "SEQUENTIAL OPERATIONS");
    free_environment();
#ifdef DLL
    dlclose(library);
#endif
    return 0;
}

double ticks_to_seconds(clock_t t_start, clock_t t_end) {
    return (double) (t_end - t_start) / sysconf(_SC_CLK_TCK);
}

void reset_measurements(t_measurement *measurement, int n) {
    for (int i = 0; i < n; i++) {
        measurement[i].t_real = 0;
        measurement[i].t_sys = 0;
        measurement[i].t_user = 0;
    }
}

void start_measurement(t_measurement *tm) {
    tm->real_start = times(&(tm->start));
}

void stop_measurement(t_measurement *tm) {
    struct tms end;
    clock_t real_end = times(&end);
    tm->t_real += ticks_to_seconds(tm->real_start, real_end);
    tm->t_sys += ticks_to_seconds(tm->start.tms_stime + tm->start.tms_cstime, end.tms_stime + end.tms_cstime);
    tm->t_user += ticks_to_seconds(tm->start.tms_utime + tm->start.tms_cutime, end.tms_utime + end.tms_cutime);
}

void save_measurement(FILE *stream, t_measurement *tm, char *operation_name) {
    fprintf(stream, "\n%s\n", operation_name);
    fprintf(stream, "\tREAL TIME\t%e\ts\n", tm->t_real);
    fprintf(stream, "\tUSER TIME\t%e\ts\n", tm->t_user);
    fprintf(stream, "\tSYS TIME\t%e\ts\n", tm->t_sys);
}

void add_measurements(t_measurement *tm, t_measurement *save_tm, t_measurement *dealloc_tm) {
    tm->t_real = save_tm->t_real + dealloc_tm->t_real;
    tm->t_sys = save_tm->t_sys + dealloc_tm->t_sys;
    tm->t_user = save_tm->t_user + dealloc_tm->t_user;
}
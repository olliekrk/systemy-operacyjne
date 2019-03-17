//
// Created by olliekrk on 10.03.19.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "finder.h"

/* TODO
    gdyby dostęp do zmiennej globalnej nie działał zrefactoryzować, aby
    configure_environment zwracało structa, a inne funkcje na nim operowały */

struct finder_properties *environment;

void set_directory(char *directory_path_arg) {
    environment->directory_path = directory_path_arg;
}

void set_temporary_file(char *temporary_file_path_arg) {
    environment->temporary_file_path = temporary_file_path_arg;
}

void set_searched_file(char *seeked_file_name_arg) {
    environment->searched_file_name = seeked_file_name_arg;
}

void configure_environment(char *directory_path_arg, char *seeked_file_name_arg, char *temporary_file_path_arg) {
    environment = calloc(1, sizeof(struct finder_properties));
    set_directory(directory_path_arg);
    set_searched_file(seeked_file_name_arg);
    set_temporary_file(temporary_file_path_arg);
}

void allocate_result_blocks(size_t blocks_size) {
    if (blocks_size < 1) {
        fprintf(stderr, "error caused by allocating invalid blocks size!\n");
        exit(1);
    }
    environment->number_of_blocks = blocks_size;
    environment->result_blocks = calloc(environment->number_of_blocks, sizeof(char *));
    if (!environment->result_blocks) {
        fprintf(stderr, "error while allocating memory for blocks!\n");
        exit(1);
    }
}

void free_environment() {
    int index = 0;
    while (index < environment->number_of_blocks) {
        free(environment->result_blocks[index]);
        environment->result_blocks[index] = NULL;
        index++;
    }
    free(environment);
}

void search() {
    size_t command_length = (strlen("find  -name "" > ") +
                             strlen(environment->directory_path) +
                             strlen(environment->searched_file_name) +
                             strlen(environment->temporary_file_path)) + 1;

    char *executable_command = calloc(command_length, sizeof(char));
    if (!executable_command) {
        fprintf(stderr, "error while allocating memory for executable command!\n");
        exit(1);
    }

    int result = sprintf(
            executable_command,
            "find %s -name \"%s\" > %s",
            environment->directory_path, environment->searched_file_name, environment->temporary_file_path);
    if (result < 0) {
        fprintf(stderr, "error: finder configuration was invalid!\n");
        exit(1);
    }

    system(executable_command);
    free(executable_command);
}

unsigned int save_block(char *block) {
    unsigned int index = 0;
    while (index < environment->number_of_blocks) {
        if (environment->result_blocks[index] == 0) {
            environment->result_blocks[index] = block;
            return index;
        }
        index++;
    }
    fprintf(stderr, "error caused by running out of memory blocks!\n");
    exit(1);
}

unsigned int save_results() {
    FILE *results_file = fopen(environment->temporary_file_path, "r");
    if (results_file == NULL) {
        fprintf(stderr, "error while opening temporary file!\n");
        exit(1);
    }

    if (fseek(results_file, 0, SEEK_END) != 0) {
        fprintf(stderr, "an error while seeking temporary file has occurred!\n");
        exit(1);
    }

    long file_size = ftell(results_file);
    rewind(results_file);

    char *results = calloc((size_t) (file_size + 1), sizeof(char));
    if (!results) {
        fprintf(stderr, "error while allocating memory for find results!\n");
        exit(1);
    }

    if (fread(results, (size_t) file_size, 1, results_file) == 0) {
        fprintf(stderr, "no files named: %s were found in directory: %s \n",
                environment->searched_file_name, environment->directory_path);
    }
    fclose(results_file);
    return save_block(results);
}

void delete_block(unsigned int index) {
    if (index < environment->number_of_blocks) {
        free(environment->result_blocks[index]);
        environment->result_blocks[index] = NULL;
    } else {
        fprintf(stderr, "error caused by running out of array while deleting block!\n");
        exit(1);
    }
}

char *get_block(unsigned int index) {
    if (index < environment->number_of_blocks && environment->result_blocks[index] != NULL) {
        return environment->result_blocks[index];
    }
    fprintf(stderr, "error caused by accessing an empty block!");
    exit(1);
}




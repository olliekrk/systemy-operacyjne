//
// Created by olliekrk on 10.03.19.
//

#ifndef ZAD1_FINDLIB_H
#define ZAD1_FINDLIB_H

struct finder_properties {
    size_t number_of_blocks;

    char **result_blocks;

    char *directory_path;

    char *searched_file_name;

    char *temporary_file_path;
};

void set_directory(char *directory_path_arg);

void set_temporary_file(char *temporary_file_path_arg);

void set_searched_file(char *seeked_file_name_arg);

void configure_environment(char *directory_path_arg, char *seeked_file_name_arg, char *temporary_file_path_arg);

void allocate_result_blocks(size_t block_size);

void free_environment();

void search();

unsigned int save_block(char *block);

unsigned int save_results();

void delete_block(unsigned int index);

char *get_block(unsigned int index);

#endif //ZAD1_FINDLIB_H

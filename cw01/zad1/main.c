//
// Created by olliekrk on 11.03.19.
//
#include <stdio.h>

#include "finder.h"

int main() {
    configure_environment(".", "*e*", "./tmp");
    allocate_result_blocks((size_t) 1e3);

    search();
    unsigned int s1 = save_results();
    printf("\n%d\n%s\n", s1, get_block(s1));

    set_searched_file("*f*");
    search();
    unsigned int s2 = save_results();
    printf("\n%d\n%s\n", s2, get_block(s2));

    set_searched_file("*lib*");
    search();
    unsigned int s3 = save_results();
    printf("\n%d\n%s\n", s3, get_block(s3));

    set_searched_file("*m*");
    search();
    unsigned int s4 = save_results();
    printf("\n%d\n%s\n", s4, get_block(s4));

    set_searched_file("*main*");
    search();
    unsigned int s5 = save_results();
    printf("\n%d\n%s\n", s5, get_block(s5));

    delete_block(0);
    delete_block(2);
    delete_block(4);

    set_searched_file("\"*.xml*\"");
    search();

    unsigned int s6 = save_results();
    printf("\n%d\n%s\n", s6, get_block(s6));

    free_environment();
}
// https://ugurkoltuk.wordpress.com/2010/03/04/an-extreme-simple-pgm-io-api/
#ifndef CW8_COMMONS
#define CW8_COMMONS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BLOCK_FILTER 0
#define INTERLEAVED_FILTER 1

//data structures
typedef struct PGMImage{
    int rows;
    int columns;
    int max_gray;
    int *matrix; // this will be rows*columns image matrix
} PGMImage;

// parsing input file 
void allocate_image_matrix(PGMImage *image){
    if(!image) show_error("Received NULL instead of image pointer")
    image->matrix = malloc(sizeof(int) * image->rows * image-> columns);
} 

void deallocate_image_matrix(PGMImage *image){
    if(image && image->matrix) free(image->matrix);
}

void skip_comment_lines(FILE *fp){
    int ch;
    char line[100];
    while ((ch = fgetc(fp)) != EOF && isspace(ch));
 
    if (ch == '#') {
        fgets(line, sizeof(line), fp);
        skip_comment_lines(fp);
    } else {
        fseek(fp, -1, SEEK_CUR);
    }
} 

void read_header(FILE *fp, PGMImage *image){
    char version[3];
    fgets(version, sizeof(version), fp);
    if (strcmp(version, "P2")==0)
        show_error("File should have magic number P2 in header defined")

    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->columns);
    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->rows);
    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->max_gray);
    fgetc(fp);
 
    allocate_image_matrix(image);
}

PGMImage *read_from_file(char *image_path){
    FILE *fp = fopen(image_path, "r");
    if(!fp) show_error("Failed to open an image");

    PGMImage *image = calloc(sizeof(PGMImage)*1);
    if(!image) show_error("Failed to allocate memory for file");

    //assign rows, columns, max_gray, allocate matrix
    read_header(fp, image);

    int value;
    int offset = 0;
    while(!feof(fp)){
        fscanf(fp, "%hu", &value);
        image->matrix[offset++] = value;
    }

    fclose(fp);
    return image;
}

// utility
void show_error(const char *message) {
    perror(message);
    exit(1);
}

// time measuring
typedef struct timeval tv;

void get_current_time(tv *buffer) {
    gettimeofday(buffer, NULL);
}

tv get_time_difference(tv *start, tv *end){
    tv time;
    timersub(end, start, &time);
    return time;
}

#endif //CW8_COMMONS
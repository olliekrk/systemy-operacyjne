#ifndef CW08_IMAGES
#define CW08_IMAGES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>


#define BLOCK_FILTER 0
#define INTERLEAVED_FILTER 1

//data structures
typedef struct PGMImage {
    int rows;
    int columns;
    int max_gray;
    int *matrix; // this will be rows*columns image matrix
} PGMImage;

typedef struct Filter {
    int size;
    double *matrix;
} Filter;

// utility
void show_error(const char *message) {
    perror(message);
    exit(1);
}

// parsing image file
void allocate_image_matrix(PGMImage *image) {
    if (!image) show_error("Received NULL instead of image pointer");
    else image->matrix = malloc(sizeof(int) * image->rows * image->columns);
    if (!image->matrix) show_error("Failed to allocate memory");
}

void deallocate_image_matrix(PGMImage *image) {
    if (image && image->matrix) free(image->matrix);
}

void skip_comment_lines(FILE *fp) {
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

void read_header(FILE *fp, PGMImage *image) {
    char version[3];
    fgets(version, sizeof(version), fp);
    if (strcmp(version, "P2") != 0)
        show_error("File should have magic number P2 in header defined");

    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->columns);
    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->rows);
    skip_comment_lines(fp);
    fscanf(fp, "%d", &image->max_gray);
    fgetc(fp);

    allocate_image_matrix(image);
}

PGMImage *read_image_from_pgm(char *image_path) {
    FILE *fp = fopen(image_path, "r");
    if (!fp) show_error("Failed to open an image");

    PGMImage *image = calloc(1, sizeof(PGMImage));
    if (!image) show_error("Failed to allocate memory for file");

    //assign rows, columns, max_gray, allocate matrix
    read_header(fp, image);

    unsigned int value;
    int offset = 0;
    int max_offset = image->columns * image->rows;
    while (offset < max_offset && !feof(fp)) {
        fscanf(fp, "%d", &value);
        image->matrix[offset++] = value;
    }

    fclose(fp);
    return image;
}

void write_image_to_pgm(char *output_path, PGMImage *image) {
    FILE *fp = fopen(output_path, "w+");
    if (!fp) show_error("Failed to create output file");

    fprintf(fp, "P2\n"
                "%d %d\n"
                "%d\n", image->columns, image->rows, image->max_gray);

    int offset = 0;
    for (int i = 0; i < image->rows; i++) {
        for (int j = 0; j < image->columns; j++) fprintf(fp, "%d ", image->matrix[offset++]);
        fprintf(fp, "\n");
    }
}

// filtering

void allocate_filter_matrix(Filter *filter) {
    if (!filter) show_error("Received NULL instead of filter pointer");
    filter->matrix = malloc(sizeof(double) * filter->size * filter->size);
    if (!filter->matrix) show_error("Failed to allocate memory");
}

void deallocate_filter_matrix(Filter *filter) {
    if (filter && filter->matrix) free(filter->matrix);
}

Filter *read_filter(char *filter_path) {
    FILE *fp = fopen(filter_path, "r");
    if (!fp) show_error("Failed to open an filter file");

    Filter *filter = calloc(1, sizeof(Filter));
    if (!filter) show_error("Failed to allocate memory for filter");

    fscanf(fp, "%d", &filter->size);
    allocate_filter_matrix(filter);

    double value;
    int offset = 0;
    int max_offset = filter->size * filter->size;
    while (offset < max_offset && !feof(fp)) {
        fscanf(fp, "%lf", &value);
        filter->matrix[offset++] = value;
    }

    fclose(fp);
    return filter;
}

// row and column must be from range [1..Inf]
double s_filter(PGMImage *image, Filter *filter, int row, int column) {
    double s = 0;
    double multiplier;
    int i_row, j_column, max_row, max_column;
    int ceiling = (int) ceil(filter->size / 2.0);

    for (int i = 1; i <= filter->size; i++)
        for (int j = 1; j <= filter->size; j++) {
            i_row = row - ceiling + i;
            j_column = column - ceiling + j;

            max_row = (int) fmax(i_row, 1) - 1;
            max_column = (int) fmax(j_column, 1) - 1;

            multiplier = filter->matrix[filter->size * (i - 1) + (j - 1)];
            s += image->matrix[max_row * image->columns + max_column] * multiplier;
        }
    return s;
}

// time measuring
typedef struct timeval tv;

void get_current_time(tv *buffer) {
    gettimeofday(buffer, NULL);
}

tv get_time_difference(tv *start, tv *end) {
    tv time;
    timersub(end, start, &time);
    return time;
}

tv add_times(tv *total, tv *added) {
    tv result;
    timeradd(total, added, &result);
    return result;
}

#endif //CW08_IMAGES
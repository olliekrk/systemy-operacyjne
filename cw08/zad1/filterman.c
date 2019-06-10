#include <pthread.h>
#include <math.h>
#include "images.h"

#define TIMES_FILE_NAME "times.txt"

pthread_t *THREADS;
int THREADS_NO;

PGMImage *image, *output;
Filter *filter;

void *block_filtering(void *thread_no) {
    int *kp = (int *) thread_no;
    int k = *kp;
    int c = ceil((double) image->columns / THREADS_NO);

    int start_x = (k - 1) * c;
    int end_x = k * c;
    int index;

    tv time_start, time_end, diff;
    get_current_time(&time_start);

    for (int x = start_x; x < end_x; x++)
        for (int row = 0; row < image->rows; row++) {
            index = row * image->columns + x;
            output->matrix[index] = (int) round(s_filter(image, filter, row, x));
        }

    get_current_time(&time_end);
    diff = get_time_difference(&time_start, &time_end);

    tv *diff_pointer = malloc(sizeof(tv));
    diff_pointer->tv_sec = diff.tv_sec;
    diff_pointer->tv_usec = diff.tv_usec;

    free(kp);
    pthread_exit(diff_pointer);
}

void *interleaved_filtering(void *thread_no) {
    int *kp = (int *) thread_no;
    int k = *kp;
    int index;

    tv time_start, time_end, diff;
    get_current_time(&time_start);

    for (int x = k - 1; x < image->columns; x += THREADS_NO)
        for (int row = 0; row < image->rows; row++) {
            index = row * image->columns + x;
            output->matrix[index] = (int) round(s_filter(image, filter, row, x));
        }


    get_current_time(&time_end);
    diff = get_time_difference(&time_start, &time_end);

    tv *diff_pointer = malloc(sizeof(tv));
    diff_pointer->tv_sec = diff.tv_sec;
    diff_pointer->tv_usec = diff.tv_usec;

    free(kp);
    pthread_exit(diff_pointer);
}

int main(int argc, char **argv) {
    if (argc < 6)
        show_error(
                "Invalid number of arguments:\n\t<THREADS_NO> <FILTER_TYPE> <IMAGE_PATH> <FILTER_PATH> <OUTPUT_PATH>");

    // parsing command line arguments
    THREADS_NO = strtol(argv[1], NULL, 10);
    char *filter_name = argv[2];
    char *image_path = argv[3];
    char *filter_path = argv[4];
    char *output_path = argv[5];

    int filter_type = -1;
    if (strcmp(filter_name, "block") == 0)
        filter_type = BLOCK_FILTER;
    if (strcmp(filter_name, "interleaved") == 0)
        filter_type = INTERLEAVED_FILTER;
    if (filter_type == -1)
        show_error("Invalid filter type argument was given");

    // reading input data
    filter = read_filter(filter_path);
    image = read_image_from_pgm(image_path);

    // initialize output
    output = calloc(1, sizeof(PGMImage));
    if (!output) show_error("Failed to allocate memory for output image");
    output->rows = image->rows;
    output->columns = image->columns;
    output->max_gray = image->max_gray;
    allocate_image_matrix(output);

    // threads creation
    THREADS = calloc(THREADS_NO, sizeof(pthread_t));
    if (!THREADS) show_error("Failed to allocate memory for TID(s)");

    void *filtering_function = NULL;
    if (filter_type == BLOCK_FILTER)
        filtering_function = block_filtering;
    else if (filter_type == INTERLEAVED_FILTER)
        filtering_function = interleaved_filtering;
    if (filtering_function == NULL)
        show_error("Invalid filtering function");

    tv time_a, time_b;
    tv total_time;
    tv total_time_threads = {0, 0};

    get_current_time(&time_a);

    // running threads
    for (int i = 1; i <= THREADS_NO; i++) {
        int *k = calloc(1, sizeof(int));
        *k = i;
        if (pthread_create(&THREADS[i - 1], NULL, filtering_function, k)) show_error("Failed to create thread(s)");
    }

    // join threads
    tv *time_spent;
    for (int i = 1; i <= THREADS_NO; i++) {

        pthread_join(THREADS[i - 1], (void **) &time_spent);

        printf("Thread ID: %ld\n"
               "Time spent: %ld [s] %ld [us]\n"
               "=\n", THREADS[i - 1],
               time_spent->tv_sec, time_spent->tv_usec);

        total_time_threads = add_times(&total_time_threads, time_spent);
        free(time_spent);
    }

    get_current_time(&time_b);
    total_time = get_time_difference(&time_a, &time_b);
    total_time = add_times(&total_time, &total_time_threads);

    // calculate and print total time spent on filtering
    printf("Total operation time:\t%ld [s] %ld [us]\n", total_time.tv_sec, total_time.tv_usec);

    // save output image
    write_image_to_pgm(output_path, output);

    // save time results
    FILE *times = fopen(TIMES_FILE_NAME, "a+");
    if (!times) show_error("Failed to open time results file");
    fprintf(times, "=\n"
                   "Threads:\t %d\n"
                   "Filter:\t %s\n"
                   "Image:\t %d x %d pixels\n"
                   "Filter:\t %d x %d\n"
                   "Total time:\t %ld [s]\t%ld [us]\n",
            THREADS_NO, filter_name, image->rows, image->columns, filter->size, filter->size,
            total_time.tv_sec, total_time.tv_usec);

    // cleanup
    fclose(times);
    deallocate_filter_matrix(filter);
    deallocate_image_matrix(image);
    deallocate_image_matrix(output);
    free(THREADS);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>

void generate_filter(int size) {
    double *numbers = calloc(size, sizeof(double));

    double number, sum = 0;
    for (int i = 0; i < size; i++) {
        number = drand48();
        numbers[i] = number;
        sum += number;
    }

    char *filename = calloc(255, sizeof(char));
    sprintf(filename, "filters_dir/filter-%d.txt", size);

    FILE *filter = fopen(filename, "w+");
    if (!filter) {
        perror("Failed to create filter file");
        exit(1);
    }

    fprintf(filter, "%d\n", size);
    for (int i = 0; i < size; i++)
        fprintf(filter, "%lf ", numbers[i] / sum);

    free(numbers);
    free(filename);
    fclose(filter);
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++)
        generate_filter(strtol(argv[i], NULL, 10));

    return 0;
}
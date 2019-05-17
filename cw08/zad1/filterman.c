#include "commons.h"

int main(int argc, char **argv){
    if(argc<6)
        show_error("Invalid number of arguments:\n\t<THREADS_NO> <FILTER_TYPE> <IMAGE_PATH> <FILTER_PATH> <OUTPUT_PATH>");

    int threads_no = strtol(argv[1], NULL, 10);
    char *filter = argv[2];
    char *image_path = argv[3];
    char *filter_path = argv[4];
    char *output_path = argv[5];

    int filter_type=-1;
    if(strcmp(filter, "block")==0)
        filter_type = BLOCK_FILTER;
    if(strcmp(filter, "interleaved")==0)
        filter_type = INTERLEAVED_FILTER;
    if(filter_type==-1)
        show_error("Invalid filter type argument was given");

    return 0;
}
#include "pngz.h"
#include "load.h"
#include "colortypes.h"
#include "filter.h"
#include "compress.h"
#include "save.h"
#include "helpers.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#define PNGZ_VERSION "0.0.1"

static const char *msg_help =
        "pngz "PNGZ_VERSION" - the (nearly) optimal lossless PNG optimizer\r\n"
        "Usage: pngz <input_file> <output_file>\r\n"
        "       -h, --help        show this info\r\n"
        "       -v, --version     print version\r\n";

static void colortype_callback(pngz_t*, void*);
static void filter_callback(pngz_t*, void*, size_t);
static void compress_callback(pngz_t*);
static void print_results(const pngz_t*);
static size_t compute_output_size(const pngz_t*);

static void parse_opts(pngz_options *options, int argc, char *argv[]) {

    if(argc == 1) {
        printf("%s\n", msg_help);
        exit(0);
    }
    if(argc == 2) {
        if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            printf("%s\n", msg_help);
            exit(0);
        }
        if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")){
            printf("%s\n", PNGZ_VERSION);
            exit(0);
        }
        printf("Invalid number of arguments\r\n%s\r\n", msg_help);
        exit(0);
    }
    options->input_filename = argv[1];
    options->output_filename = argv[2];
}

int main(int argc, char *argv[]) {

    pngz_options options;
    parse_opts(&options, argc, argv);

    printf("pngz %s\r\n", PNGZ_VERSION);

    pngz_t png;
    png.options = &options;

    png.plte_size = 0;
    png.trns_size = 0;

    load_png(&png);
    colortype_dispatch(&png, &colortype_callback);

    print_results(&png);

    // Cleanup
    free(png.raw_pixels);

    return 0;
}

// callback passed to colortype methods
static void colortype_callback(pngz_t *png, void *unfiltered) {
    filter(png, unfiltered, &filter_callback);
}

// callback passed to filter method
static void filter_callback(pngz_t *png, void *filtered, size_t filtered_size) {
    compress(png, filtered, filtered_size, &compress_callback);
}

// callback passed to compress method
static void compress_callback(pngz_t *png) {
    size_t output_size = compute_output_size(png);
    //printf("outsize %dB\n", output_size);
    if(output_size < png->best_size) {
        save_png(png);
        png->best_size = output_size;
    }
}

static size_t compute_output_size(const pngz_t *png) {
    size_t output_size = 0;
    output_size += (8 + 25 + 12); // Signature + IHDR + IEND

    // Chunks have 12 bytes overhead (4 length, 4 type, 4 crc)
    output_size += png->idat_size + 12;
    if(png->plte_size > 0) {
        output_size += png->plte_size + 12;
    }
    if(png->trns_size > 0) {
        output_size += png->trns_size + 12;
    }
    return output_size;
}

static void print_results(const pngz_t *png) {

    const size_t original = png->original_size;
    const size_t best = png->best_size;

    printf("pngz completed in %ld seconds\r\n", clock()/CLOCKS_PER_SEC);
    printf("original size:      %30dB\r\n", original);
    printf("optimized size:     %30dB\r\n", best);

    if(original == best) {
        printf("no improvement :(\r\n");
    }
    else {
        float improvement = 100 - (float)best/original*100;
        printf("byte decrease:      %30dB\r\n", best-original);
        printf("percet decrease:    %30.2f%%\r\n", improvement);
    }
}
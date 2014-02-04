#ifndef PNGZ_H_
#define PNGZ_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct pngz_options_s
{
    char *input_filename;
    char *output_filename;

} pngz_options;

typedef struct raw_pixel_s
{
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t alpha;

} raw_pixel;

typedef struct pngz_s
{
    size_t width;
    size_t height;

    size_t original_size;
    size_t best_size;

    pngz_options *options;
    raw_pixel *raw_pixels;

    uint8_t bit_depth;
    uint8_t color_type;

    uint8_t *idat;
    size_t idat_size;

    uint8_t *trns;
    size_t trns_size;

    uint8_t *plte;
    size_t plte_size;

} pngz_t;

#endif
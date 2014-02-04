#include "pngz.h"
#include "colortypes.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct color_set_s {
    size_t size;
    size_t capacity;
    raw_pixel *colors;
} color_set;

typedef struct png_analysis_s{
    bool has_color_pixels;
    unsigned int total_transparent_px;
    unsigned int total_semitransparent_px;
    unsigned int minimum_bit_depth;
    color_set *cset;
} png_analysis;


void ct2_8(pngz_t *png, png_analysis *analysis,
           void(*callback)(pngz_t*, void*));
/*
void ct0(pngz_t *png, uint8_t bit_depth, analysis*);

void ct3(pngz_t *png, uint8_t bit_depth, analysis*);
void ct4(pngz_t *png, uint8_t bit_depth, analysis*);
void ct6(pngz_t *png, uint8_t bit_depth, analysis*);
*/

png_analysis* analyze_png(pngz_t*);
void free_png_analysis(png_analysis*);

void colortype_dispatch(pngz_t *png, void(*callback)(pngz_t*, void*)) {

    png_analysis *analysis = analyze_png(png);
    printf("minimum_bit_depth = %d\n", analysis->minimum_bit_depth);
    printf("total colors = %d\n", analysis->cset->size);
    /*
    if(analysis->minimum_bit_depth <= 8 &&
       analysis->total_semitransparent_px == 0 &&
       analysis->total_transparent_px ==0) {
        ct2_8(png, analysis, callback);
    }
    */
    ct2_8(png, analysis, callback);
    free_png_analysis(analysis);
}

// COLOR SET ////////////////////////////////////////////////

int color_set_contains(color_set* cset, raw_pixel color) {
    unsigned int i;
    for(i=0;i<cset->size;i++) {
        if(raw_pixel_cmp(cset->colors[i], color) == 0) {
            return 1;
        }
    }
    return 0;
}

int color_set_contains_rgba(color_set *cset, uint16_t red, uint16_t green,
                            uint16_t blue, uint16_t alpha, uint8_t bit_depth) {
    raw_pixel color;
    color.red = convert_bit_depth(red, bit_depth, 16);
    color.green = convert_bit_depth(green, bit_depth, 16);
    color.blue = convert_bit_depth(blue, bit_depth, 16);
    color.alpha = convert_bit_depth(alpha, bit_depth, 16);
    return color_set_contains(cset, color);
}

color_set* extract_color_set(raw_pixel *pixels, size_t total_pixels) {

    color_set *cset = malloc(sizeof(color_set));
    cset->colors = malloc(sizeof(raw_pixel)*256);
    cset->capacity = 256;
    cset->size = 0;
    raw_pixel current;

    unsigned int i;
    for(i=0;i<total_pixels;i++) {
        current = pixels[i];
        if(current.alpha == 0) {
            current.red = 0;
            current.green = 0;
            current.blue = 0;
        }
        if(!color_set_contains(cset, current)) {
            if(cset->size == cset->capacity){
                cset->capacity <<= 1;
                cset->colors = realloc(cset->colors, cset->capacity);
            }
            cset->colors[cset->size] = current;
            cset->size++;
        }
    }
    return cset;
}

void free_color_set(color_set *cset) {
    free(cset->colors);
    free(cset);
}

// ANALYSIS ////////////////////////////////////////////////

static int is_transparent(raw_pixel pixel) {
    return pixel.alpha == 0;
}

static int is_semitransparent(raw_pixel pixel) {
    return 0 < pixel.alpha && pixel.alpha < 65535;
}

static int is_color(raw_pixel pixel) {
    if(is_transparent(pixel)) return 0; // A transparent pixel is not color
    return (pixel.red != pixel.green) || (pixel.green != pixel.blue);
}

static int bit_depth_floor(uint16_t val) {
    const uint16_t mask = ~(uint16_t)0;
    if(val == mask || val == 0) {
        return 1;
    }
    uint8_t depth = 16;
    uint8_t offset = 0;
    while(1) {
        offset += (depth >> 1);
        uint16_t a = val >> offset;
        uint16_t b = val & (mask >> offset);
        if(a^b) {
            return depth;
        }
        depth >>= 1;
    }
}

static int pixel_bit_depth_floor(raw_pixel pixel) {
    if(is_transparent(pixel)) return 1;
    uint8_t red_min = bit_depth_floor(pixel.red);
    uint8_t green_min = bit_depth_floor(pixel.green);
    uint8_t blue_min = bit_depth_floor(pixel.blue);
    
    if(red_min > green_min && red_min > blue_min) {
        return red_min;
    }
    else if(green_min > red_min && green_min > blue_min) {
        return green_min;
    }
    else {
        return blue_min;
    }
}

png_analysis* analyze_png(pngz_t *png) {

    raw_pixel *pixels = png->raw_pixels;
    unsigned int total_px = png->width * png->height;
    unsigned int i;

    bool greyscale = true;
    uint8_t minimum_bit_depth = 1;
    unsigned int num_transparent_px = 0;
    unsigned int num_semitransparent_px = 0;

    for(i=0;i<total_px;i++) {

        raw_pixel current = pixels[i];

        if(greyscale && is_color(current)) {
            greyscale = false;
        }

        if(is_transparent(current)) {
            num_transparent_px++;
        }
        else if(is_semitransparent(current)) {
            num_semitransparent_px++;
        }

        uint8_t bit_depth = pixel_bit_depth_floor(current);
        if(minimum_bit_depth < bit_depth) {
            minimum_bit_depth = bit_depth;
        }
    }

    png_analysis *analysis = malloc(sizeof(png_analysis));

    analysis->has_color_pixels = !greyscale;
    analysis->total_transparent_px = num_transparent_px;
    analysis->total_semitransparent_px = num_semitransparent_px;
    analysis->minimum_bit_depth = minimum_bit_depth;
    analysis->cset = extract_color_set(pixels, total_px);

    return analysis;
}

void free_png_analysis(png_analysis *analysis) {
    free_color_set(analysis->cset);
    free(analysis);
}

// CT Helpers

void* ct_alloc(const size_t width, const size_t height,
               const size_t bits_per_pixel) {
    if(bits_per_pixel*width % 8 == 0) {
        return malloc(height*width*bits_per_pixel);
    }
    else {
        const size_t bytes_per_row = (bits_per_pixel*width + 7)/8;
        return calloc(height, bytes_per_row);
    }
}

// COLORTYPE 0 ////////////////////////////////////////////////
/*
void ct0() {

    uint16_t* data = ct_calloc(bit_depth);

    pixel_writer writer;
    writer.data = data;
    writer.bit_depth = bit_depth;
    writer.values_per_pixel = 1;

    for(i=0;i<total_px;i++){
        write_pixel(&writer, i, png->raw_pixels[i]);
    }

    if(!analysis->has_transparent_pixels) {
        callback(pngz_t, data);
    }
    else {
        // make counter to whatever
        // while counter
        // generate next color
        // if color_set_contains(analysis->cset, color):
        //      continue
        // replace all trns indexes with this color:
        // write_pixel(&writer, index, color);
        // callback
    }
    free(data);
}
*/

// COLORTYPE 2 ////////////////////////////////////////////////


void ct2_8(pngz_t *png, png_analysis *analysis,
           void(*callback)(pngz_t*, void*)) {
    png->color_type = 2;
    png->bit_depth = 8;
    size_t total_px = png->width * png->height;
    uint8_t *data = calloc(1, sizeof(uint8_t)*3*total_px);
    unsigned int i;
    
    for(i=0; i < total_px; i++) {
        data[i*3+0] = png->raw_pixels[i].red >> 8;
        data[i*3+1] = png->raw_pixels[i].green >> 8;
        data[i*3+2] = png->raw_pixels[i].blue >> 8;
    }

    callback(png, data);
    free(data);
}

// COLORTYPE 3 ////////////////////////////////////////////////
// COLORTYPE 4 ////////////////////////////////////////////////
// COLORTYPE 6 ////////////////////////////////////////////////
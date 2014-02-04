#include "load.h"
#include "png.h" // libpng
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Loads png from png->options->input_filename,
// converts all pixels to 16 bit rgba and fills in
// the passed pngz_t's raw_pixels field with them.
// Also fills in width, height, original size, and
// sets best_size to be original_size.

void load_png(pngz_t *png) {

    char *file_name = png->options->input_filename;

    size_t original_size, width, height, bytes_per_pixel, bytes_per_row;

    int png_transforms = PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_EXPAND_16;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;    

    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        printf("File '%s' could not be opened\r\n", file_name);
        exit(1);
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        printf("PNG read struct could not be created\r\n");
        exit(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        printf("PNG info struct could not be created\r\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        printf("Problem reading the file\r\n");
        exit(1);
    }

    // Get original file size
    fseek(fp, 0L, SEEK_END);
    original_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, png_transforms, NULL);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    bytes_per_row = png_get_rowbytes(png_ptr, info_ptr);
    bytes_per_pixel = bytes_per_row/width;

    raw_pixel* raw_pixels = malloc(sizeof(raw_pixel)*width*height);

    unsigned int y,x;
    row_pointers = png_get_rows(png_ptr, info_ptr);
    
    for(y=0; y<height; y++) {

        png_byte *row = row_pointers[y];

        for(x=0; x<width; x++) {

            uint16_t *src_ptr = (uint16_t*)(row+x*bytes_per_pixel);
            raw_pixel *dest_ptr = &raw_pixels[width*y+x];

            dest_ptr->red = src_ptr[0];
            dest_ptr->green = src_ptr[1];
            dest_ptr->blue = src_ptr[2];
            if(bytes_per_pixel == 6) { // No alpha chanel
                dest_ptr->alpha = 65535;
            }
            else {
                dest_ptr->alpha = src_ptr[3];
            }
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    png->width = width;
    png->height = height;
    png->original_size = original_size;
    png->best_size = original_size;
    png->raw_pixels = raw_pixels;
}
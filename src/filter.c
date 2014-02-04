#include "pngz.h"
#include "filter.h"

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Return size in bits of the pixels in this pngz_t based on
 * its color type and bit depth.
 */
int get_bits_per_pixel(pngz_t *png){
    switch(png->color_type){
        case 0:
            return png->bit_depth;
        case 2:
            return png->bit_depth*3;
        case 3:
            return png->bit_depth;
        case 4:
            return png->bit_depth*2;
        case 6:
            return png->bit_depth*4;
        default:
            exit(1);
    }
}

/* Return size in bytes of each row of pixels in this png
 * unfiltered based on the number of bits in each pixel.
 */
int get_unfiltered_bytes_per_row(pngz_t *png) {
    int bits_per_pixel = get_bits_per_pixel(png);
    int bits_per_row = bits_per_pixel * png->width;
    int bytes_per_row = (bits_per_row + 7) / 8;  // Round up div
    return bytes_per_row;
}

int get_filtered_bytes_per_row(pngz_t *png) {
    return get_unfiltered_bytes_per_row(png) + 1;
}

int get_unfiltered_size(pngz_t *png) {
    return get_unfiltered_bytes_per_row(png) * png->height;
}

int get_filtered_size(pngz_t *png) {
    return get_filtered_bytes_per_row(png) * png->height;
}

uint8_t** pre_filter_data(pngz_t *png, const uint8_t *unfiltered) {
    int i,j;
    const int num_rows = png->height;
    const int unfiltered_row_size = get_unfiltered_bytes_per_row(png);
    const uint8_t bytes_per_pixel = (get_bits_per_pixel(png) + 7)/8;

    // Filtered data has one byte at the front of each row
    // to specify the filter type.
    const size_t filtered_size = num_rows*(unfiltered_row_size+1);

    // Malloc memory for pre-filtered data
    uint8_t **prefiltered = malloc(5 * sizeof(uint8_t *));
    for(i=0;i<5;i++)
    {
        prefiltered[i] = malloc(filtered_size * sizeof(uint8_t));
    }

    // Pre-filter every row
    for(i=0;i<num_rows;i++) // row loop
    {
        int unfiltered_row_offset = i*unfiltered_row_size;
        int prefiltered_row_offset = i*(unfiltered_row_size+1);

        // Write filter-type byte for this row.
        prefiltered[0][prefiltered_row_offset] = 0; // 0: None
        prefiltered[1][prefiltered_row_offset] = 1; // 1: Sub
        prefiltered[2][prefiltered_row_offset] = 2; // 2: Up
        prefiltered[3][prefiltered_row_offset] = 3; // 3: Avg
        prefiltered[4][prefiltered_row_offset] = 4; // 4: Paeth

        // Copy entire row for filter type 0.
        memcpy(prefiltered[0]+prefiltered_row_offset+1,
               unfiltered+unfiltered_row_offset,
               unfiltered_row_size);
        
        // Filter each individual byte.
        for(j=0;j<unfiltered_row_size;j++)
        {
            int unfiltered_byte_index = unfiltered_row_offset+j;
            int prefiltered_byte_index = prefiltered_row_offset+j+1;

            uint8_t raw = unfiltered[unfiltered_byte_index]; // Original value

            // Get values of up, left, and up_left: each is needed for the
            // various filter calculations.
            uint8_t left, up, up_left;

            // Set `up` to be the value of the byte directly above the current,
            // imagining unfiltered as two dimensional (row_size x height).
            // If we're in the first row, set it to zero.
            if(i > 0) {
                up = unfiltered[unfiltered_byte_index - unfiltered_row_size];
            }
            else {
                up = 0;
            }

            // Set `left` to be the value of the byte `bytes_per_pixel` to the
            // left. If that's beyond the first column, set it to zero.
            if(j >= bytes_per_pixel) {
                left = unfiltered[unfiltered_byte_index - bytes_per_pixel];
            }
            else {
                left = 0;
            }

            // Combination of up and left: the byte one row up and
            // `bytes_per_pixel` to the left.
            if(i > 0 && j >= bytes_per_pixel) {
                up_left = unfiltered[unfiltered_byte_index -
                                    (unfiltered_row_size + bytes_per_pixel)];
            }
            else {
                up_left = 0;
            }

            // Calculate individual filtered values.

            // 1: Sub
            prefiltered[1][prefiltered_byte_index] = (raw - left) % 256;
            // 2: Up
            prefiltered[2][prefiltered_byte_index] = (raw - up) % 256;
            // 3: Avg
            const int avg = (raw-(int)floor((left+up)/2.0))%256;
            prefiltered[3][prefiltered_byte_index] = avg;
            // 4: Paeth
            int p = left + up - up_left;
            int pa, pb, pc, paeth;
            pa = abs(p - left);
            pb = abs(p - up);
            pc = abs(p - up_left);
            if(pa <= pb && pa <= pc)
                paeth = left;
            else if(pb <= pc)
                paeth = up;
            else
                paeth = up_left;
            prefiltered[4][prefiltered_byte_index] = (raw - paeth) % 256;
        }
    }
    return prefiltered;
}

void smart_filter(pngz_t *png,
                  uint8_t **prefiltered,
                  void(*callback)(pngz_t*, void*, size_t)) {

    unsigned int i,j,k;
    const size_t filtered_row_size = get_filtered_bytes_per_row(png);
    const size_t filtered_size = filtered_row_size * png->height;

    callback(png, (void *)prefiltered[0], filtered_size);
    callback(png, (void *)prefiltered[1], filtered_size);
    callback(png, (void *)prefiltered[2], filtered_size);
    callback(png, (void *)prefiltered[3], filtered_size);
    callback(png, (void *)prefiltered[4], filtered_size);

    uint8_t *filtered = malloc(filtered_size);
    
    for(i=0;i<png->height;i++) {
        int offset = i * filtered_row_size;
        int row_sums[5];
        for(j=0;j<5;j++) {
            int row_sum = 0;
            for(k=0;k<filtered_row_size;k++) {
                row_sum += abs((int8_t)prefiltered[j][offset + k]);
            }
            row_sums[j] = row_sum;
        }
        int smallest_idx = 0;
        int smallest = row_sums[0];
        for(j=1;j<5;j++) {
            if(row_sums[j] < smallest) {
                smallest = row_sums[j];
                smallest_idx = j;
            }
        }
        memcpy(filtered+offset, prefiltered[smallest_idx]+offset,
               filtered_row_size);
    }
    
    callback(png, (void *)filtered, filtered_size);
    free(filtered);
}

void brute_force_filter(pngz_t *png,
                        uint8_t **prefiltered,
                        void(*callback)(pngz_t*, void*, size_t)) {

    const int num_rows = png->height;
    const size_t filtered_row_size = get_filtered_bytes_per_row(png);
    const size_t filtered_size = filtered_row_size * num_rows;

    int i;

    // Malloc & initialize counter to zero
    uint8_t* counter = calloc(num_rows, sizeof(uint8_t));

    // Malloc & initialize 'filtered' w/ prefiltered[0]
    unsigned char* filtered = malloc(filtered_size);
    memcpy(filtered, prefiltered[0], filtered_size);

    // Go through all possible filter combinations
    for(;;)
    {
        // Incr counter and modify 'filtered' appropriately
        for(i=0;i<num_rows;i++)
        {
            if(counter[i] != 4) // Skip over 4s, as they will be zeroed
            {
                int offset = i*filtered_row_size;
                if(i != 0) // zero counter[x] where x < i
                {
                    memset(counter, 0, i*sizeof(uint8_t));
                    memcpy(filtered, prefiltered[0], offset);
                }
                counter[i]++;
                memcpy(filtered+offset, prefiltered[counter[i]]+offset,
                       filtered_row_size);
                goto callback;
            }
        }
        goto cleanup;
        callback:
        callback(png, (void *)filtered, filtered_size);
    }
    cleanup:
    free(counter);
    free(filtered);
}

void filter(pngz_t *png,
//          pngz_options opts, 
            const void *unfiltered,
            void(*callback)(pngz_t*, void*, size_t)){

    uint8_t **prefiltered = pre_filter_data(png, unfiltered);

    smart_filter(png, prefiltered, callback);
    //brute_force_filter(png, prefiltered, callback);

    // Cleanup
    int i;
    for(i=0;i<5;i++) free(prefiltered[i]);
    free(prefiltered);
}
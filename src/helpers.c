#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct counter_s
{
    const unsigned int places;
    const unsigned int base;

    uint16_t *count;

    unsigned int last_modified_idx;
    unsigned int last_modified_val;

};

#include "helpers.h"

counter *counter_create(unsigned int base, unsigned int places) {
    counter *c = malloc(sizeof(counter));
    *(unsigned int *)&c->places = places; // cast away const...
    *(unsigned int *)&c->base = base;
    c->count = calloc(places, sizeof(uint16_t));
    c->last_modified_val = 0;
    c->last_modified_idx = 0;
    return c;
}

int counter_incr(counter* c) {
    unsigned int i;
    for(i=0; i < c->places; i++)
    {
        if(c->count[i] != c->base - 1)
        {
            memset(c->count, 0, sizeof(uint16_t)*i);
            c->last_modified_val = ++c->count[i];
            c->last_modified_idx = i;
            return 1;
        }
    }
    return 0;
}

void counter_delete(counter* c) {
    free(c->count);
    free(c);
}

int counter_get_last_idx(counter* c){
    return c->last_modified_idx;
}

int counter_get_last_val(counter* c){
    return c->last_modified_val;
}

uint64_t convert_raw_pixel_to_uint64(const raw_pixel pixel) {
    uint64_t pixel_64 = 0;
    pixel_64 |= (uint64_t)pixel.red << 48;
    pixel_64 |= (uint64_t)pixel.green << 32;
    pixel_64 |= (uint64_t)pixel.blue << 16;
    pixel_64 |= (uint64_t)pixel.alpha;
    return pixel_64;
}

int raw_pixel_cmp(const raw_pixel px1, const raw_pixel px2) {
    const uint64_t px1_64 = convert_raw_pixel_to_uint64(px1);
    const uint64_t px2_64 = convert_raw_pixel_to_uint64(px2);
    return (px1_64 < px2_64) ? -1 : (px1_64 > px2_64);
}

// Converts the bit depth of `input` from `from` to `to`.
uint16_t convert_bit_depth(uint16_t input, uint8_t from, uint8_t to) {
    if (from < to) {
        for(;;) {
            input |= (input << from);
            from <<= 1;
            if(from == to) break;
        }
    }
    else if(from > to) {
        input >>= (from - to);
    }
    return input;
}
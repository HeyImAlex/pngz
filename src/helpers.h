#ifndef PNGZ_HELPERS_H
#define PNGZ_HELPERS_H

#include "pngz.h"
#include <stdint.h>

uint64_t convert_raw_pixel_to_uint64(const raw_pixel pixel);
int raw_pixel_cmp(const raw_pixel px1, const raw_pixel px2);
uint16_t convert_bit_depth(uint16_t input, uint8_t from, uint8_t to);

typedef struct counter_s counter;

counter *counter_create(unsigned int base, unsigned int places);
void counter_delete(counter*);
int counter_incr(counter*);
int counter_get_last_idx(counter*);
int counter_get_last_val(counter*);

#endif
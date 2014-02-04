#ifndef PNGZ_COMPRESS_H_
#define PNGZ_COMPRESS_H_

#include "pngz.h"

void compress(pngz_t *png, void *in, size_t insize, void(*callback)(pngz_t*));

#endif
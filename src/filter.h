#ifndef PNGZ_FILTER_H_
#define PNGZ_FILTER_H_

#include "pngz.h"

void filter(pngz_t *png,
            const void *unfiltered,
            void(*callback)(pngz_t*, void*, size_t));

#endif
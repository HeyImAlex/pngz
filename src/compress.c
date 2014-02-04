#include "compress.h"
#include "zlib_container.h" // zopfli

#include <stdlib.h>

static const ZopfliOptions zopfli_options = {
    .verbose = 0,
    .verbose_more = 0,
    .numiterations = 15,
    .blocksplitting = 1,
    .blocksplittinglast = 0,
    .blocksplittingmax = 15
};

void compress(pngz_t *png, void *in, size_t insize, void(*callback)(pngz_t*)) {

    png->idat = NULL;
    png->idat_size = 0;

    ZopfliZlibCompress(
        &zopfli_options,
        (unsigned char *)in,
        insize,
        (unsigned char **)&(png->idat),
        &(png->idat_size)
    );
    callback(png);

    free(png->idat);
    png->idat_size = 0;
}
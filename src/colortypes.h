
#ifndef PNGZ_COLORTYPES_H_
#define PNGZ_COLORTYPES_H_

#include "pngz.h"

void colortype_dispatch(pngz_t *png, void(*callback)(pngz_t*, void*));

#endif
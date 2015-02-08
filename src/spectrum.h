#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "common_sp.h"

struct spectrum;

struct spectrum* spectrum_alloc(int N);

int spectrum_add_u8(struct spectrum* s, const cmplx_u8* src, double* power_spectrum, int len);

void spectrum_free(struct spectrum* s);

#endif

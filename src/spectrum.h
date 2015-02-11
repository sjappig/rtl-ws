#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "common_sp.h"

struct spectrum;

struct spectrum* spectrum_alloc(int N);

int spectrum_add_cmplx_u8(struct spectrum* s, const cmplx_u8* src, double* power_spectrum, int len);

int spectrum_add_cmplx_s32(struct spectrum* s, const cmplx_s32* src, double* power_spectrum, int len);

int spectrum_add_real_f64(struct spectrum* s, const double* src, double* power_spectrum, int len);

void spectrum_free(struct spectrum* s);

#endif

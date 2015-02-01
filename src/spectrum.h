#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "common_sp.h"

typedef void* spectrum_handle;

spectrum_handle init_spectrum(int N);

int add_spectrum(spectrum_handle handle, const cmplx* src, double* dst, int len, double linear_energy_gain);

void free_spectrum(spectrum_handle handle);

#endif

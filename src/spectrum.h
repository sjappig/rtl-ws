#ifndef SPECTRUM_H
#define SPECTRUM_H
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <stdint.h>
#include "common_sp.h"

typedef void* spectrum_handle;

EXTERNC spectrum_handle init_spectrum(int N);

EXTERNC int calculate_spectrum(spectrum_handle handle, cmplx* src, int src_len, double* dst, int dst_len);

EXTERNC void free_spectrum(spectrum_handle handle);

#endif

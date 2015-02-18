#ifndef RESAMPLE_H
#define RESAMPLE_H

#include "common_sp.h"

#define HALF_BAND_N 11

struct cic_delay_line
{
    cmplx_s32 integrator_prev_out;
    cmplx_s32 comb_prev_in;
};

int cic_decimate(int R, const cmplx_u8* src, int src_len, cmplx_s32* dst, int dst_len, struct cic_delay_line* delay);

/* delay line has to have HALF_BAND_N - 1 elements */
void halfband_decimate(const float* input, float* output, int output_len, float* delay);

#endif

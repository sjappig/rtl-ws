#ifndef _RESAMPLE_H
#define _RESAMPLE_H
#include "common_sp.h"

struct cic_delay_line
{
    cmplx_s32 integrator_prev_out;
    cmplx_s32 comb_prev_in;
};

int cic_decimate(int R, const cmplx_u8* src, int src_len, cmplx_s32* dst, int dst_len, struct cic_delay_line* delay);

#endif

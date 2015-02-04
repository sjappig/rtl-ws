#ifndef _RESAMPLE_H
#define _RESAMPLE_H
#include "common_sp.h"

int cic_decimate(int R, const cmplx_u8* src, int src_len, cmplx_s32* dst, int dst_len);

#endif

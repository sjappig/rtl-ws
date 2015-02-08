#include <string.h>
#include "common_sp.h"

void set_cmplx_u8(cmplx_u8* dst, uint8_t re, uint8_t im)
{
    dst->re = re;
    dst->im = im;
}

void set_cmplx_s32(cmplx_s32* dst, const cmplx_s32* src)
{
    memcpy(dst, src, sizeof(cmplx_s32));
}

void set_cmplx_s32_cmplx_u8(cmplx_s32* dst, const cmplx_u8* src, int32_t transform)
{
    dst->re = (int32_t)src->re + transform;
    dst->im = (int32_t)src->im + transform;
}

void add(const cmplx_s32* a, const cmplx_s32* b, cmplx_s32* result)
{    
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

void sub(const cmplx_s32* a, const cmplx_s32* b, cmplx_s32* result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}
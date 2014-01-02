#include "common_sp.h"

void set_cmplx(cmplx* dst, uint8_t re, uint8_t im) 
{
   dst->re = re;
   dst->im = im;
}

void set_cmplx_s32(cmplx_s32* dst, cmplx_s32* src)
{
   dst->re = src->re;
   dst->im = src->im;
}

void set_cmplx_s32_cmplx(cmplx_s32* dst, cmplx* src, int32_t transform) 
{
   dst->re = (int32_t)src->re - transform;
   dst->im = (int32_t)src->im - transform;
}

void add(cmplx_s32* a, cmplx_s32* b, cmplx_s32* result)
{
   result->re = a->re + b->re;
   result->im = a->im + b->im;
}

void sub(cmplx_s32* a, cmplx_s32* b, cmplx_s32* result)
{
   result->re = a->re - b->re;
   result->im = a->im - b->im;
}






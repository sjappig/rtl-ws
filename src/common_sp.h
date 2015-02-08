#ifndef _COMMON_SP_H
#define _COMMON_SP_H

#include <stdint.h>

typedef struct 
{
    uint8_t re;
    uint8_t im;
} cmplx_u8;

typedef struct 
{
    int32_t re;
    int32_t im;
} cmplx_s32;

#define set_cmplx_u8(dst, re, im) dst.re = re; dst.im = im;

#define set_cmplx_s32(dst, src) *((int64_t*) &dst) = *((int64_t*) &src);

#define set_cmplx_s32_cmplx_u8(dst, src, transform) dst.re = transform + src.re; dst.im = transform + src.im;

#define add_cmplx_s32(a, b, result) result.re = a.re + b.re; result.im = a.im + b.im;

#define sub_cmplx_s32(a, b, result) result.re = a.re - b.re; result.im = a.im - b.im;

#endif

#ifndef _COMMON_SP_H
#define _COMMON_SP_H

#include <stdint.h>

typedef struct 
{
    uint8_t re;
    uint8_t im;
} cmplx;

typedef struct 
{
    int32_t re;
    int32_t im;
} cmplx_s32;

void set_cmplx(cmplx* dst, uint8_t re, uint8_t im);

void set_cmplx_s32(cmplx_s32* dst, cmplx_s32* src);

void set_cmplx_s32_cmplx(cmplx_s32* dst, cmplx* src, int32_t transform);

void add(cmplx_s32* a, cmplx_s32* b, cmplx_s32* result);

void sub(cmplx_s32* a, cmplx_s32* b, cmplx_s32* result);

#endif

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

void set_cmplx_u8(cmplx_u8* dst, uint8_t re, uint8_t im);

void set_cmplx_s32(cmplx_s32* dst, const cmplx_s32* src);

void set_cmplx_s32_cmplx_u8(cmplx_s32* dst, const cmplx_u8* src, int32_t transform);

void add(const cmplx_s32* a, const cmplx_s32* b, cmplx_s32* result);

void sub(const cmplx_s32* a, const cmplx_s32* b, cmplx_s32* result);

#endif

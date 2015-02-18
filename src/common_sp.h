#ifndef COMMON_SP_H
#define COMMON_SP_H

#include <stdint.h>
#include <math.h>

typedef struct 
{
    uint8_t re;
    uint8_t im;
} cmplx_u8;

typedef union 
{
    int64_t bulk;
    struct p {
        int32_t re;
        int32_t im;
    } p;
} cmplx_s32;

#define set_cmplx_u8(dst, re, im) dst.re = re; dst.im = im;

#define set_cmplx_s32(dst, src) dst.bulk = src.bulk; 

#define set_cmplx_s32_cmplx_u8(dst, src, transform) dst.p.re = transform + src.re; dst.p.im = transform + src.im;

#define add_cmplx_s32(a, b, result) result.p.re = a.p.re + b.p.re; result.p.im = a.p.im + b.p.im;

#define sub_cmplx_s32(a, b, result) result.p.re = a.p.re - b.p.re; result.p.im = a.p.im - b.p.im;

#define real_cmplx_s32(c) (c.p.re)

#define imag_cmplx_s32(c) (c.p.im)

#define real_cmplx_u8(c) (c.re)

#define imag_cmplx_u8(c) (c.im)

static inline float atan2_approx(float y, float x)
{
    static const float pi = (float) M_PI;
    static const float pi_by_2 = (float) (M_PI / 2);
    register float atan = 0;
    register float z = 0;

    if (x == 0)
    {
        if (y > 0.0f)
            return pi_by_2;
        
        if (y == 0)
            return 0;

        return -pi_by_2;
    }
    z = y/x;
    if (fabs(z) < 1.0f)
    {
        atan = z/(1.0f + 0.28f*z*z);
        if (x < 0)
        {
            if (y < 0.0f)
                return atan - M_PI;
            
            return atan + M_PI;
        }
    }
    else
    {
        atan = pi_by_2 - z/(z*z + 0.28f);
        if (y < 0.0f)
            return atan - M_PI;
    }
    return atan;
}

#endif

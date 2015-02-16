#ifndef _COMMON_SP_H
#define _COMMON_SP_H

#include <stdint.h>
#include <math.h>

#define PI_DOUBLE     3.1415926535897932
#define PIBY2_DOUBLE  1.5707963267948966

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

static inline double atan2_approx(float y, float x)
{
    register double atan = 0;
    register float z = 0;

    if ( x == 0.0f )
    {
        if ( y > 0.0f )
            return PIBY2_DOUBLE;
        
        if ( y == 0.0f )
            return 0.0f;

        return -PIBY2_DOUBLE;
    }
    z = y/x;
    if (fabs(z) < 1.0f)
    {
        atan = z/(1.0f + 0.28f*z*z);
        if ( x < 0.0f )
        {
            if ( y < 0.0f )
                return atan - PI_DOUBLE;
            
            return atan + PI_DOUBLE;
        }
    }
    else
    {
        atan = PIBY2_DOUBLE - z/(z*z + 0.28f);
        if ( y < 0.0f )
            return atan - PI_DOUBLE;
    }
    return atan;
}

#endif

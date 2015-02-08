#include <stdint.h>
#include <arm_neon.h>
#include "resample.h"

int cic_decimate(int R, const cmplx_u8* src, int src_len, cmplx_s32* dst, int dst_len)
{
    int src_idx = 0;
    int dst_idx = 0;
    cmplx_s32 integrator_curr_in = { 0, 0};
    cmplx_s32 integrator_curr_out = { 0, 0};
    cmplx_s32 integrator_prev_out = { 0, 0};
    cmplx_s32 comb_prev_in = { 0, 0};
    //uint8x8_t test;

    if (dst_len * R != src_len)
        return -1;
    for (src_idx = 0; src_idx < src_len; src_idx++)
    {
        // integrator y(n) = y(n-1) + x(n)
        set_cmplx_s32_cmplx_u8(&integrator_curr_in, &(src[src_idx]), -127);
        add(&integrator_curr_in, &integrator_prev_out, &integrator_curr_out);

        // resample
        if ((src_idx % R) == 0)
        {
            // comb y(n) = x(n) - x(n-1)
            if (dst_idx >= dst_len)
            {
                return -2;
            }
            sub(&integrator_curr_out, &comb_prev_in, &(dst[dst_idx]));
            set_cmplx_s32(&comb_prev_in, &integrator_curr_out);
            dst_idx++;
        }
        set_cmplx_s32(&integrator_prev_out, &integrator_curr_out);
    }

    return 0;
}

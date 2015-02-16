#include <string.h>
#include "resample.h"

int cic_decimate(int R, const cmplx_u8* src, int src_len, cmplx_s32* dst, int dst_len, struct cic_delay_line* delay)
{
    register int src_idx = 0;
    register int dst_idx = 0;
    register cmplx_s32 integrator_curr_in = { 0 };
    register cmplx_s32 integrator_curr_out = { 0 };
    register cmplx_s32 integrator_prev_out = { 0 };
    register cmplx_s32 comb_prev_in = { 0 };

    set_cmplx_s32(integrator_prev_out, delay->integrator_prev_out);
    set_cmplx_s32(comb_prev_in, delay->comb_prev_in);
    
    if (dst_len * R != src_len)
        return -1;

    for (src_idx = 0; src_idx < src_len; src_idx++)
    {
        // integrator y(n) = y(n-1) + x(n)
        set_cmplx_s32_cmplx_u8(integrator_curr_in, src[src_idx], -128);
        add_cmplx_s32(integrator_curr_in, integrator_prev_out, integrator_curr_out);

        // resample
        if (((src_idx+1) % R) == 0)
        {
            // comb y(n) = x(n) - x(n-1)
            if (dst_idx >= dst_len)
            {
                return -2;
            }
            sub_cmplx_s32(integrator_curr_out, comb_prev_in, dst[dst_idx]);
            set_cmplx_s32(comb_prev_in, integrator_curr_out);
            dst_idx++;
        }
        set_cmplx_s32(integrator_prev_out, integrator_curr_out);
    }

    set_cmplx_s32(delay->integrator_prev_out, integrator_prev_out);
    set_cmplx_s32(delay->comb_prev_in, comb_prev_in);
    return 0;
}

static double half_band_kernel[] = { 0.01824, 0, -0.11614, 0, 0.34790, 0.5, 0.34790, 0, -0.11614, 0, 0.01824 };

void decimate_by_two(double* input, double* output, int output_len, double* delay_line)
{
    register int n = 0;
    register int k = 0;
    register int idx = 0;

    for (n = 0; n < output_len; n++) 
    {
        output[n] = 0;
        for (k = 0; k < HALF_BAND_N; k++)
        {
            idx = 2*n - k;
            if (idx >= 0)
            {
                output[n] += half_band_kernel[k] * input[idx];
            } 
            else
            {
                output[n] += half_band_kernel[k] * delay_line[HALF_BAND_N + idx];
            }

        }
    }
    memcpy(delay_line, input, (HALF_BAND_N - 1) * sizeof(double));
}
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include "spectrum.h"

struct spectrum
{
    int N;
    fftw_complex* in;
    fftw_complex* out;
    fftw_plan plan;
};

struct spectrum* spectrum_alloc(int N)
{
    struct spectrum* s = (struct spectrum*) calloc(1, sizeof(struct spectrum));
    s->in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    s->out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    s->plan = fftw_plan_dft_1d(N, s->in, s->out, FFTW_FORWARD, FFTW_ESTIMATE);
    s->N = N;
    return s;
}

int spectrum_add_u8(struct spectrum* s, const cmplx_u8* src, double* power_spectrum, int len)
{
    int i = 0;
    int offset = s->N / 2;
    
    if (len != s->N)
        return -1;

    memcpy(s->in, src, len * sizeof(cmplx_u8));

    for (i = 0; i < s->N; i++)
    {
        s->in[i][0] = (((double)src[i].re) - 128) / 128;
        s->in[i][1] = (((double)src[i].im) - 128) / 128;
    }  
    fftw_execute(s->plan);

    for (i = 0; i < len; i++)
    {
        int idx = (offset + i) % len;
        if (idx > 0)
        {
            power_spectrum[i] += (s->out[idx][0]*s->out[idx][0] + s->out[idx][1]*s->out[idx][1]);
        }
        else
        {
            power_spectrum[i] += power_spectrum[i-1];
        }
    }

    return 0;
}

void spectrum_free(struct spectrum* s)
{
    fftw_destroy_plan(s->plan);
    fftw_free(s->in);
    fftw_free(s->out);
    free(s);
}

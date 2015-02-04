#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include "spectrum.h"

typedef struct 
{
    int N;
    fftw_complex* in;
    fftw_complex* out;
    fftw_plan plan;

} _spectrum_handle;

spectrum_handle init_spectrum(int N)
{
    _spectrum_handle* h = (_spectrum_handle*) malloc(sizeof(_spectrum_handle));
    h->in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    h->out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    h->plan = fftw_plan_dft_1d(N, h->in, h->out, FFTW_FORWARD, FFTW_ESTIMATE);
    h->N = N;
    return (spectrum_handle) h;
}

int add_spectrum(spectrum_handle handle, const cmplx_u8* src, double* dst, int len, double linear_energy_gain)
{
    _spectrum_handle* h = (_spectrum_handle*) handle;
    if (len != h->N)
        return 0;

    int i = 0;
    int offset = h->N / 2;

    for (i = 0; i < h->N; i++)
    {
        h->in[i][0] = (((double)src[i].re) - 128) / 128;
        h->in[i][1] = (((double)src[i].im) - 128) / 128;
    }   
    fftw_execute(h->plan);

    for (i = 0; i < h->N; i++)
    {
        int idx = (offset + i) % (h->N);
        if (idx > 0) 
        {
            dst[i] += linear_energy_gain*(h->out[idx][0]*h->out[idx][0] + h->out[idx][1]*h->out[idx][1]);
        }
        else
        {
            dst[i] += dst[i-1];
        }

    }

    return h->N;
}

void free_spectrum(spectrum_handle handle)
{
    _spectrum_handle* h = (_spectrum_handle*) handle;
    fftw_destroy_plan(h->plan);
    fftw_free(h->in);
    fftw_free(h->out);
    memset(h, 0, sizeof(_spectrum_handle));
    free(h);
}

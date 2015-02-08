#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "decimator.h"
#include "list.h"
#include "resample.h"
#include "log.h"

#define INTERNAL_BUF_LEN_MS 100
#define EPSILON 0.0001

struct decimator
{
    pthread_mutex_t mutex;
    struct list* callback_list;
    
    double sample_rate;
    int down_factor;

    cmplx_u8* input_signal;
    int input_signal_len;
    int surplus;

    cmplx_s32* resampled_signal;
    int resampled_signal_len;
};

static void callback_notifier(void* callback, void* decim)
{
    DECIMATOR_CALLBACK f = (DECIMATOR_CALLBACK) callback;
    struct decimator* d = (struct decimator*) decim;
    f(d->resampled_signal, d->resampled_signal_len);
}

struct decimator* decimator_alloc()
{
    struct decimator* d = (struct decimator*) calloc(1, sizeof(struct decimator));
    pthread_mutex_init(&(d->mutex), NULL);
    d->callback_list = list_alloc();
    return d;
}

void decimator_add_callback(struct decimator* d, DECIMATOR_CALLBACK callback)
{
    pthread_mutex_lock(&(d->mutex));
    list_add(d->callback_list, callback);
    pthread_mutex_unlock(&(d->mutex));
}

int decimator_set_parameters(struct decimator* d, double sample_rate, int down_factor)
{
    int r = -1;

    pthread_mutex_lock(&(d->mutex));
    if (sample_rate > 0 && down_factor > 0)
    {
        if (abs(d->sample_rate - sample_rate) < EPSILON || d->down_factor != down_factor)
        {
            DEBUG("Setting decimator params: sample_rate == %f, down_factor == %d\n", sample_rate, down_factor);
            d->sample_rate = sample_rate;
            d->down_factor = down_factor;
            d->resampled_signal_len = (int) ((d->sample_rate / d->down_factor) * INTERNAL_BUF_LEN_MS / 1000);
            d->input_signal_len = d->resampled_signal_len * d->down_factor;

            d->resampled_signal = (cmplx_s32*) calloc(d->resampled_signal_len, sizeof(cmplx_s32));
            d->input_signal = (cmplx_u8*) calloc(d->input_signal_len, sizeof(cmplx_u8*));

            d->surplus = 0;
        }
        r = 0;
    }
    pthread_mutex_unlock(&(d->mutex));

    return r;
}

int decimator_decimate_cmplx_u8(struct decimator* d, const cmplx_u8* complex_signal, int len)
{
    int current_idx = 0;
    int remaining = len;
    int block_size = 0;

    pthread_mutex_lock(&(d->mutex));
    
    block_size = d->input_signal_len - d->surplus;

    if (d->resampled_signal == NULL || d->input_signal == NULL)
        return -1;

    while (remaining >= block_size)
    {
        memcpy(&(d->input_signal[d->surplus]), &(complex_signal[current_idx]), block_size);
        remaining -= block_size;
        current_idx += block_size;

        if (cic_decimate(d->down_factor, d->input_signal, d->input_signal_len, d->resampled_signal, d->resampled_signal_len)) 
        {
            ERROR("Error while decimating signal");
            return -2;
        }

        list_apply(d->callback_list, callback_notifier, d);

        d->surplus = 0;
        block_size = d->input_signal_len;
    }
    
    if (remaining > 0)
    {
        memcpy(&(d->input_signal[d->surplus]), &(complex_signal[len - remaining]), remaining);
        d->surplus += remaining;
    }
    pthread_mutex_unlock(&(d->mutex));

    return 0;
}

void decimator_remove_callbacks(struct decimator* d)
{
    pthread_mutex_lock(&(d->mutex));
    list_clear(d->callback_list);
    pthread_mutex_unlock(&(d->mutex));
}

void decimator_free(struct decimator* d)
{
    decimator_remove_callbacks(d);
    pthread_mutex_destroy(&(d->mutex));
    list_free(d->callback_list);
    free(d->input_signal);
    free(d->resampled_signal);
    free(d);
}
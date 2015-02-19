#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "common.h"
#include "rtl_sensor.h"
#include "spectrum.h"
#include "signal_source.h"
#include "log.h"
#include "rf_decimator.h"
#include "rate_logger.h"
#include "cbb_main.h"

#define DEV_INDEX           0
#define SPECTRUM_EST_MS     250
#define FFT_POINTS          1024
#define FFT_AVERAGE         6

static struct rtl_dev* dev;
static pthread_mutex_t spectrum_mutex;
static struct spectrum* spect;
static struct rf_decimator* decim = NULL;
static struct rate_logger* cbb_rate_log = NULL;
static double power_spectrum_transfer[FFT_POINTS] = {0};
static int spectrum_averaging_count = 0;
static uint64_t last_spectrum_estimation = 0;
static volatile int new_spectrum_available = 0;

static void log_data_rate(const cmplx_u8* signal, int len)
{
    rate_logger_log(cbb_rate_log, len);
}

static void decimate(const cmplx_u8* signal, int len)
{
    rf_decimator_decimate_cmplx_u8(decim, signal, len);
}

static void estimate_spectrum(const cmplx_u8* signal, int len)
{
    static double power_spectrum[FFT_POINTS];
    int i = 0;
    int blocks = len / FFT_POINTS;

    if (timestamp() < (last_spectrum_estimation + SPECTRUM_EST_MS))
        return;

    blocks = blocks <= FFT_AVERAGE ? blocks : FFT_AVERAGE;
    memset(power_spectrum, 0, FFT_POINTS*sizeof(double));

    for (i = 0; i < blocks; i++)
    {
        if (spectrum_add_cmplx_u8(spect, &(signal[i*FFT_POINTS]), power_spectrum, FFT_POINTS))
        {
            ERROR("Error while estimating spectrum.\n");
            return;
        }
    }

    last_spectrum_estimation = timestamp();
    new_spectrum_available = 1;

    pthread_mutex_lock(&spectrum_mutex);

    memcpy(power_spectrum_transfer, power_spectrum, FFT_POINTS*sizeof(double));
    spectrum_averaging_count = blocks;

    pthread_mutex_unlock(&spectrum_mutex);
}

void cbb_init(int decimated_bw_target_hz)
{
    cbb_rate_log = rate_logger_alloc();
    rate_logger_set_parameters(cbb_rate_log, "CBB", 60000);
    
    rtl_init(&dev, DEV_INDEX);

    decim = rf_decimator_alloc();
    rf_decimator_set_parameters(decim, rtl_sample_rate(dev), rtl_sample_rate(dev)/decimated_bw_target_hz);
    
    pthread_mutex_init(&spectrum_mutex, NULL);
    spect = spectrum_alloc(FFT_POINTS);

    signal_source_start(dev);
    signal_source_add_callback(log_data_rate);
    signal_source_add_callback(decimate);
    signal_source_add_callback(estimate_spectrum);
}

struct rtl_dev* cbb_get_rtl_dev()
{
    return dev;
}

struct rf_decimator* cbb_rf_decimator()
{
    return decim;
}

int cbb_new_spectrum_available()
{
     return new_spectrum_available;
}

int cbb_get_spectrum_payload(char* buf, int buf_len, int spectrum_gain_db)
{
    static double power_spectrum_local[FFT_POINTS];
    int spectrum_averaging_count_local = 0;
    int idx = 0;
    int len = 0;
    double linear_energy_gain = pow(10, spectrum_gain_db/10);

    pthread_mutex_lock(&spectrum_mutex);

    memcpy(power_spectrum_local, power_spectrum_transfer, FFT_POINTS*sizeof(double));
    spectrum_averaging_count_local = spectrum_averaging_count;

    pthread_mutex_unlock(&spectrum_mutex);

    if (spectrum_averaging_count > 0)
    {
        for (idx = 0; idx < FFT_POINTS; idx++)
        {
            int m = 10*log10(abs(linear_energy_gain * power_spectrum_local[idx] / spectrum_averaging_count_local));
            m = m >= 0 ? m : 0;
            m = m <= 255 ? m : 255;
            buf[len++] = (char) m;
        }
    }

    new_spectrum_available = 0;

    return len;
}

void cbb_close()
{
    signal_source_remove_callbacks();

    signal_source_stop();
 
    rf_decimator_free(decim);

    spectrum_free(spect);

    pthread_mutex_destroy(&spectrum_mutex);
    
    rtl_close(dev);

    rate_logger_free(cbb_rate_log);
}


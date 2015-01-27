#include <stdio.h>
#include <rtl-sdr.h>
#include "rtl_sensor.h"

#define _RTL_DEFAULT_SAMPLE_RATE	2048000
#define _RTL_DEFAULT_FREQUENCY 		100000000

static rtlsdr_dev_t *dev = NULL;
static uint32_t frequency = 0;
static uint32_t samp_rate = 0;
static int gain = 254;

void rtl_tune(uint32_t f) 
{
	if (frequency == f)
		return;

	int r = rtlsdr_set_center_freq(dev, f);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set center freq.\n");
	else {
		printf("Tuned to %u Hz.\n", f);
		frequency = f;
	}
}

void rtl_set_sample_rate(uint32_t fs) 
{
	if (samp_rate == fs)
		return;

	int r = rtlsdr_set_sample_rate(dev, fs);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");
	else 
	{
		printf("Sample rate set to %u Hz.\n", fs);
		samp_rate = fs;
	}
}

uint32_t rtl_freq() 
{
	return frequency;
}

uint32_t rtl_sample_rate() 
{
	return samp_rate;
}

int rtl_init(int dev_index) 
{
	int r = rtlsdr_open(&dev, dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
		return -1;
	}

	rtl_set_sample_rate(_RTL_DEFAULT_SAMPLE_RATE);   
	rtl_tune(_RTL_DEFAULT_FREQUENCY);

	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to enable manual gain.\n");

        /* Set the tuner gain */
	r = rtlsdr_set_tuner_gain(dev, gain);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
	else
		fprintf(stderr, "Tuner gain set to %f dB.\n", gain/10.0);

	/* Reset endpoint before we start reading from it (mandatory) */
	r = rtlsdr_reset_buffer(dev);
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");

	return 0;
}

int rtl_read(char* buffer, int buf_len, int* n_read) {
	return rtlsdr_read_sync(dev, buffer, buf_len, n_read);
}

void rtl_cancel() {
	if (dev != NULL)
		rtlsdr_cancel_async(dev);
}

void rtl_close() {
	rtlsdr_close(dev);
}

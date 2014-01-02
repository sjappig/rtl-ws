#ifndef RTL_SENSOR_H
#define RTL_SENSOR_H
#include <stdint.h>

void rtl_tune(uint32_t f);

void rtl_set_sample_rate(uint32_t fs);

uint32_t rtl_freq();

uint32_t rtl_sample_rate();

int rtl_init(int dev_index);

int rtl_read(char* buffer, int buf_len, int* n_read);

void rtl_cancel();

void rtl_close();

#endif
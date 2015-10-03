#ifndef RTL_SENSOR_H
#define RTL_SENSOR_H

#include <stdint.h>

struct rtl_dev;


int rtl_init(struct rtl_dev** dev, int dev_index);

int rtl_set_frequency(struct rtl_dev* dev, uint32_t f);

int rtl_set_sample_rate(struct rtl_dev* dev, uint32_t fs);

int rtl_set_gain(struct rtl_dev* dev, double gain);

uint32_t rtl_freq(const struct rtl_dev* dev);

uint32_t rtl_sample_rate(const struct rtl_dev* dev);

double rtl_gain(const struct rtl_dev* dev);

int rtl_read_async(struct rtl_dev* dev, void (*callback)(unsigned char*, uint32_t, void*), void* user);

void rtl_cancel(struct rtl_dev* dev);

void rtl_close(struct rtl_dev* dev);

#endif

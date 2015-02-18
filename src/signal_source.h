#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H

#include "common_sp.h"
#include "rtl_sensor.h"

typedef void (*signal_source_callback)(const cmplx_u8*,int);


void signal_source_start(struct rtl_dev* dev);

void signal_source_add_callback(signal_source_callback callback);

void signal_source_remove_callbacks();

void signal_source_stop();

#endif

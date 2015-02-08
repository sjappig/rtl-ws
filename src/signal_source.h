#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H


#include "common_sp.h"
#include "rtl_sensor.h"


typedef void (*SIGNAL_CALLBACK)(const cmplx_u8*,int);


void start_signal_source(struct rtl_dev* dev);

void add_signal_callback(SIGNAL_CALLBACK callback);

void remove_signal_callbacks();

void stop_signal_source(struct rtl_dev* dev);

#endif

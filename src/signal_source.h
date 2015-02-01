#ifndef SIGNAL_SOURCE_H
#define SIGNAL_SOURCE_H

#include "common_sp.h"

typedef void (*SIGNAL_CALLBACK)(const cmplx*,int);

int add_signal_callback(SIGNAL_CALLBACK callback);

SIGNAL_CALLBACK remove_signal_callback(int id);
	
void start_signal_source(void* user);

void stop_signal_source();

#endif

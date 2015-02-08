#ifndef DECIMATOR_H
#define DECIMATOR_H

#include "common_sp.h"

typedef void (*DECIMATOR_CALLBACK)(const cmplx_s32*,int);

struct decimator;


struct decimator* decimator_alloc();

void decimator_add_callback(struct decimator* d, DECIMATOR_CALLBACK callback);

int decimator_set_parameters(struct decimator* d, double sample_rate, int down_factor);

int decimator_decimate_cmplx_u8(struct decimator* d, const cmplx_u8* complex_signal, int len);

void decimator_remove_callbacks(struct decimator* d);

void decimator_free(struct decimator* d);

#endif

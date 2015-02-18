#ifndef RF_DECIMATOR_H
#define RF_DECIMATOR_H

#include "common_sp.h"

typedef void (*rf_decimator_callback)(const cmplx_s32*,int);

struct rf_decimator;


struct rf_decimator* rf_decimator_alloc();

void rf_decimator_add_callback(struct rf_decimator* d, rf_decimator_callback callback);

int rf_decimator_set_parameters(struct rf_decimator* d, double sample_rate, int down_factor);

int rf_decimator_decimate_cmplx_u8(struct rf_decimator* d, const cmplx_u8* complex_signal, int len);

void rf_decimator_remove_callbacks(struct rf_decimator* d);

void rf_decimator_free(struct rf_decimator* d);


#endif

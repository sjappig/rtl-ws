#ifndef CBB_MAIN_H
#define CBB_MAIN_H

#include "rf_decimator.h"
#include "rtl_sensor.h"

void cbb_init();

struct rf_decimator* cbb_rf_decimator();

struct rtl_dev* cbb_get_rtl_dev();

int cbb_new_spectrum_available();

int cbb_get_spectrum_payload(char* buf, int buf_len, int spectrum_gain_db);

void cbb_close();

#endif
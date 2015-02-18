#ifndef RATE_LOGGER_H
#define RATE_LOGGER_H

struct rate_logger;

struct rate_logger* rate_logger_alloc();

void rate_logger_set_parameters(struct rate_logger* r, const char* logger_name, int log_rate_ms);

void rate_logger_log(struct rate_logger* r, int sample_count);

void rate_logger_free(struct rate_logger* r);

#endif
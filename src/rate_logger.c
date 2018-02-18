#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "log.h"
#include "rate_logger.h"

struct rate_logger 
{
    char* logger_name;
    int log_rate_ms;
    uint64_t received_samples;
    uint64_t last_timestamp;
};

struct rate_logger* rate_logger_alloc()
{
    struct rate_logger* r = (struct rate_logger*) calloc(1, sizeof(struct rate_logger));
    r->log_rate_ms = 10000;
    return r;
}

void rate_logger_set_parameters(struct rate_logger* r, const char* logger_name, int log_rate_ms)
{
    r->log_rate_ms = log_rate_ms;
    
    if (r->logger_name != NULL)
        free(r->logger_name);

    r->logger_name = (char*) calloc(strlen(logger_name), 1);
    strcpy(r->logger_name, logger_name);
}

void rate_logger_log(struct rate_logger* r, int sample_count)
{
    uint64_t diff_ms = 0;

    if (r->received_samples == 0)
    {
        r->last_timestamp = timestamp();
    }

    r->received_samples += sample_count;
    diff_ms = timestamp() - r->last_timestamp;
    if (diff_ms >= r->log_rate_ms)
    {
        INFO("[%s] Sample rate: %lu kHz\n", ((r->logger_name != NULL) ? r->logger_name : "null"), r->received_samples/diff_ms);
        r->received_samples = 0;
    }
}

void rate_logger_free(struct rate_logger* r)
{
    if (r->logger_name != NULL)
        free(r->logger_name);

    free(r);
}
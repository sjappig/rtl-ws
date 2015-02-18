#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "signal_source.h"
#include "rtl_sensor.h"
#include "list.h"
#include "log.h"

struct _signal_holder
{
    const cmplx_u8* signal;
    int len;
};

static pthread_t worker_thread;
static pthread_mutex_t callback_mutex;
static struct list* callback_list = NULL;
static struct rtl_dev* sensor = NULL;
static volatile int running = 0;

static void signal_source_callback_notifier(void* callback, void* signal)
{
    signal_source_callback f = (signal_source_callback) callback;
    struct _signal_holder* s_h = (struct _signal_holder*) signal;
    f(s_h->signal, s_h->len);
}

static void rtl_async_callback(unsigned char* buf, uint32_t len, void* ctx)
{
    struct _signal_holder s_h = { (cmplx_u8*) buf, len / 2 };
    pthread_mutex_lock(&callback_mutex);
    list_apply2(callback_list, signal_source_callback_notifier, &s_h);
    pthread_mutex_unlock(&callback_mutex);
}

static void* worker(void* user)
{
    int status = 0;
    struct rtl_dev* dev = (struct rtl_dev*) user;
    char ctx = 0;
    DEBUG("Reading signal from sensor\n");
    if (running)
    {
        status = rtl_read_async(dev, rtl_async_callback, &ctx);
        if (status < 0)
        {
            ERROR("Read failed with status %d\n", status);
        }
    }

    DEBUG("Done reading signal from sensor\n");

    return NULL;
}

void signal_source_start(struct rtl_dev* dev)
{
    DEBUG("Starting signal source...\n");
    if (running)
        return;

    running = 1;
    sensor = dev;
    callback_list = list_alloc();
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_create(&worker_thread, NULL, worker, sensor);
    DEBUG("Signal source started.\n");
}

void signal_source_add_callback(signal_source_callback callback)
{
    pthread_mutex_lock(&callback_mutex);
    list_add(callback_list, callback);
    pthread_mutex_unlock(&callback_mutex);
}

void signal_source_remove_callbacks()
{
    pthread_mutex_lock(&callback_mutex);
    list_clear(callback_list);
    pthread_mutex_unlock(&callback_mutex);
}

void signal_source_stop()
{
    DEBUG("Stopping signal source...\n");
    if (!running)
        return;

    running = 0;
    rtl_cancel(sensor);
    pthread_join(worker_thread, NULL);
    pthread_mutex_destroy(&callback_mutex);
    list_free(callback_list);
    DEBUG("Signal source stopped.\n");
}
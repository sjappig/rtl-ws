#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "signal_source.h"
#include "rtl_sensor.h"
#include "list.h"
#include "log.h"

#define BUFFER_SIZE 163840

struct signal_holder
{
    const cmplx_u8* signal;
    int len;
};

static pthread_t worker_thread;
static pthread_mutex_t callback_mutex;
static struct list* callback_list = NULL;
static volatile int running = 0;

static void rtl_async_callback(unsigned char* buf, uint32_t len, void* ctx);

static void notify_callbacks(const cmplx_u8* complex_signal, int len);

static void* worker(void* user);

static void callback_notifier(void* callback, void* signal);

void start_signal_source(struct rtl_dev* dev)
{
    DEBUG("Starting signal source...\n");
    if (running)
        return;

    running = 1;
    callback_list = list_alloc();
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_create(&worker_thread, NULL, worker, dev);
    DEBUG("Signal source started.\n");
}

void add_signal_callback(SIGNAL_CALLBACK callback)
{
    pthread_mutex_lock(&callback_mutex);
    list_add(callback_list, callback);
    pthread_mutex_unlock(&callback_mutex);
}

void remove_signal_callbacks()
{
    pthread_mutex_lock(&callback_mutex);
    list_clear(callback_list);
    pthread_mutex_unlock(&callback_mutex);
}

void stop_signal_source(struct rtl_dev* dev)
{
    DEBUG("Stopping signal source...\n");
    if (!running)
        return;

    running = 0;
    rtl_cancel(dev);
    pthread_join(worker_thread, NULL);
    pthread_mutex_destroy(&callback_mutex);
    list_free(callback_list);
    callback_list = NULL;
    DEBUG("Signal source stopped.\n");
}

static void notify_callbacks(const cmplx_u8* complex_signal, int len)
{
    pthread_mutex_lock(&callback_mutex);
    struct signal_holder s_h = { complex_signal, len };
    list_apply(callback_list, callback_notifier, &s_h);
    pthread_mutex_unlock(&callback_mutex);
}

static void* worker(void* user)
{
    int status = 0;
    struct rtl_dev* dev = (struct rtl_dev*) user;

    DEBUG("Entering signal source work loop\n");
    while (running)
    {
        status = rtl_read_async(dev, rtl_async_callback, NULL);
        if (status < 0)
        {
            ERROR("Read failed with status %d\n", status);
            break;
        }
    }
    DEBUG("Exited signal source work loop\n");
    return NULL;
}

static void rtl_async_callback(unsigned char* buf, uint32_t len, void* ctx)
{
    notify_callbacks((cmplx_u8*) buf, len / 2);
}

static void callback_notifier(void* callback, void* signal)
{
    SIGNAL_CALLBACK f = (SIGNAL_CALLBACK) callback;
    struct signal_holder* s_h = (struct signal_holder*) signal;
    f(s_h->signal, s_h->len);
}
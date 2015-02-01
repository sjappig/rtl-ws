#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "signal_source.h"
#include "rtl_sensor.h"

#include "log.h"

struct _callback_node
{
    int id;
    SIGNAL_CALLBACK func;
    struct _callback_node* next;
};

static pthread_t worker_thread;
static pthread_mutex_t callback_mutex;
static struct _callback_node* root = NULL;
static volatile int running = 0;

static void notify_callbacks(cmplx* complex_signal, int len)
{
    struct _callback_node* node = NULL;
    pthread_mutex_lock(&callback_mutex);
    node = root;
    while (node != NULL)
    {
        node->func(complex_signal, len);
        node = node->next;
    }   
    pthread_mutex_unlock(&callback_mutex);
}

static void* worker(void* user)
{
    int status = 0;
    int n_read = 0;
    uint32_t out_block_size = 262144;
    uint8_t* buffer = (uint8_t*) calloc(out_block_size, sizeof(uint8_t));
    struct rtl_dev* dev = (struct rtl_dev*) user;

    DEBUG("Entering signal source work loop\n");
    while (running)
    {      
        n_read = 0;
        status = rtl_read(dev, buffer, out_block_size, &n_read);
        n_read = n_read > out_block_size ? out_block_size : n_read;
        
        if (status < 0) 
        {	 
            ERROR("Read failed. Exiting work loop.\n");
            break;
        }

        notify_callbacks((cmplx*) buffer, n_read / 2);      
    }
    DEBUG("Exited signal source work loop\n");
    free(buffer);
    return NULL; 
}


int add_signal_callback(SIGNAL_CALLBACK callback)
{
    struct _callback_node* node = NULL;
    pthread_mutex_lock(&callback_mutex);
    node = root;
    if (node != NULL)
    {
        while (node->next != NULL)
        {
            node = node->next;
        }
        node->next = (struct _callback_node*) calloc(1, sizeof(struct _callback_node));
        node->next->id = node->id + 1;
        node->next->func = callback;
        node = node->next;
    }
    else
    {
        root = (struct _callback_node*) calloc(1, sizeof(struct _callback_node));
        root->id = 0;
        root->func = callback;
        node = root;
    }   
    pthread_mutex_unlock(&callback_mutex);
    return node->id;
}


SIGNAL_CALLBACK remove_signal_callback(int id)
{
    struct _callback_node* node = NULL;
    struct _callback_node* prev = NULL;
    SIGNAL_CALLBACK result = NULL;
    pthread_mutex_lock(&callback_mutex);
    node = root;
    while (node != NULL)
    {
        if (node->id == id)
        {
            if (prev == NULL)
            {
                root = node->next;
            }
            else
            {
                prev->next = node->next;
            }
            node->next = NULL;
            result = node->func;
            free(node);
            break;
        }
        prev = node;
        node = node->next;
    }   
    pthread_mutex_unlock(&callback_mutex);
    return result;
}


void start_signal_source(void* user)
{
    DEBUG("Starting signal source...\n");
    if (running)
        return;

    running = 1;
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_create(&worker_thread, NULL, worker, user);   
    DEBUG("Signal source started.\n");
}


void stop_signal_source()
{
    DEBUG("Stopping signal source...\n");
    if (!running)
        return;

    running = 0;
    pthread_join(worker_thread, NULL);
    pthread_mutex_destroy(&callback_mutex);
    DEBUG("Signal source stopped.\n");
}

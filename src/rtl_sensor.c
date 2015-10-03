#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef REAL_SENSOR
#include <rtl-sdr.h>
#endif //REAL_SENSOR

#include "rtl_sensor.h"
#include "log.h"

#define RTL_DEFAULT_SAMPLE_RATE     2048000
#define RTL_DEFAULT_FREQUENCY       100000000
#define RTL_DEFAULT_GAIN            25.4

struct rtl_dev {
#ifdef REAL_SENSOR
    rtlsdr_dev_t *dev;
#endif //REAL_SENSOR
    uint32_t f;
    uint32_t fs;
    double gain;
};

int rtl_init(struct rtl_dev** dev, int dev_index) 
{
    int r = 0;
    DEBUG("rtl_init called with dev == %x, dev_index == %d\n", (unsigned long) dev, dev_index);
    *dev = (struct rtl_dev*) calloc(1, sizeof(struct rtl_dev));
#ifdef REAL_SENSOR
    r = rtlsdr_open(&((*dev)->dev), dev_index)
#endif //REAL_SENSOR
    if (r >= 0)
    {
        if ((r = rtl_set_sample_rate(*dev, RTL_DEFAULT_SAMPLE_RATE)) >= 0 &&
            (r = rtl_set_frequency(*dev, RTL_DEFAULT_FREQUENCY)) >= 0)
        {
#ifdef REAL_SENSOR
            DEBUG("Setting rtl gain mode to manual...\n");
            r = rtlsdr_set_tuner_gain_mode((*dev)->dev, 1);
            if (r < 0)
            {
                ERROR("Failed to enable manual gain.\n");
            }
#endif //REAL_SENSOR
            
            DEBUG("Setting rtl gain to %f...\n", RTL_DEFAULT_GAIN);
            if ((r = rtl_set_gain(*dev, RTL_DEFAULT_GAIN)) >= 0) 
            {
#ifdef REAL_SENSOR
                r = rtlsdr_reset_buffer((*dev)->dev);
                if (r < 0)
                {
                    ERROR("Failed to reset buffers.\n");
                }
#endif //REAL_SENSOR
            }
        }
    }
    else 
    {
        ERROR("Failed to open rtlsdr device #%d.\n", dev_index);            
    }
    DEBUG("rtl_init returns %d\n", r);
    return r;
}

int rtl_set_frequency(struct rtl_dev* dev, uint32_t f) 
{
    int r = 0;
    if (dev->f != f)
    {
#ifdef REAL_SENSOR
        r = rtlsdr_set_center_freq(dev->dev, f);
#endif //REAL_SENSOR
        if (r < 0)
        {
            ERROR("Failed to set center freq.\n");
        }
        else
        {
            INFO("Tuned to %u Hz.\n", f);
            dev->f = f;
        }
    }
    return r;
}

int rtl_set_sample_rate(struct rtl_dev* dev, uint32_t fs) 
{
    int r = 0;
    if (dev->fs != fs)
    {
#ifdef REAL_SENSOR
        r = rtlsdr_set_sample_rate(dev->dev, fs);
#endif //REAL_SENSOR
        if (r < 0)
        {
            ERROR("Failed to set sample rate.\n");
        }
        else 
        {
            INFO("Sample rate set to %u Hz.\n", fs);
            dev->fs = fs;
        }
    }
    return r;
}

int rtl_set_gain(struct rtl_dev* dev, double gain)
{
    int r = 0;
    if (dev->gain != gain)
    {
#ifdef REAL_SENSOR
        r = rtlsdr_set_tuner_gain(dev->dev, (int) (gain * 10));
#endif //REAL_SENSOR
        if (r < 0)
        {
            ERROR("Failed to set tuner gain.\n");
        }
        else
        {
            INFO("Tuner gain set to %f dB.\n", gain);
            dev->gain = gain;        
        }
    }
    return r;
}

uint32_t rtl_freq(const struct rtl_dev* dev) 
{
    return dev->f;
}

uint32_t rtl_sample_rate(const struct rtl_dev* dev) 
{
    return dev->fs;
}

double rtl_gain(const struct rtl_dev* dev)
{
    return dev->gain;
}

int rtl_read_async(struct rtl_dev* dev, void (*callback)(unsigned char*, uint32_t, void*), void* user)
{
#ifdef REAL_SENSOR
    return rtlsdr_read_async(dev->dev, (rtlsdr_read_async_cb_t) callback, user, 0, 0);
#else
    return 0;
#endif //REAL_SENSOR
}

void rtl_cancel(struct rtl_dev* dev)
{
#ifdef REAL_SENSOR
    if (dev->dev != NULL)
    {
        if (rtlsdr_cancel_async(dev->dev) != 0) 
        {
            ERROR("Canceling rtl async failed\n");
        }
    }
#endif //REAL_SENSOR
}

void rtl_close(struct rtl_dev* dev) 
{
    DEBUG("rtl_close called with dev == %x\n", (unsigned long) dev);
#ifdef REAL_SENSOR
    if (dev->dev != NULL)
    {
        rtlsdr_close(dev->dev);
    }
#endif //REAL_SENSOR
    free(dev);
    DEBUG("rtl_close returns\n");
}

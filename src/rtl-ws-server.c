#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <libwebsockets.h>

#include "common.h"
#include "rtl_sensor.h"
#include "spectrum.h"
#include "resample.h"
#include "signal_source.h"
#include "log.h"
#include "http_handler.h"
#include "decimator.h"

#define DEV_INDEX           0

#define DEFAULT_BUF_LENGTH  (16 * 16384)
#define SEND_BUFFER_SIZE    16384
#define FREQ_CMD            "freq"
#define SAMPLE_RATE_CMD     "bw"
#define SW_GAIN_CMD         "swgain"
#define START_CMD           "start"
#define STOP_CMD            "stop"
#define FFT_POINTS          1024
#define FFT_AVERAGE         4
#define PORT                80
#define LOCAL_RESOURCE_PATH "../resources"
#define LOG_RATE_MS         30000
#define DOWNFACTOR          10


struct per_session_data__rtl_ws {
    int id;
};

static int callback_rtl_ws(struct libwebsocket_context *context,
    struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason,
    void *user, void *in, size_t len);

static struct libwebsocket_protocols protocols[] = {
    {
        "rtl-ws-protocol",
        callback_rtl_ws,
        sizeof(struct per_session_data__rtl_ws),
        1024
    },
    { NULL, NULL, 0, 0 } /* terminator */
};

static int current_user_id = 0;
static int latest_user_id = 1;

static volatile int send_data = 0;
static volatile int force_exit = 0;
static volatile int sw_gain = 0;
static volatile uint64_t spectrum_timestamp = 0;

static pthread_mutex_t data_mutex;
static char* send_buffer = NULL;
static struct spectrum* spect;
static struct decimator* decim = NULL;

static char *resource_path = LOCAL_RESOURCE_PATH;
static double power_spectrum_transfer[FFT_POINTS] = {0};
static int spectrum_averaging_count = 0;

void log_data_rate(const cmplx_u8* complex_signal, int len)
{
    static uint64_t samples = 0;
    static struct timespec tv_start = { 0, 0 };
    static struct timespec tv_end = { 0, 0 };
    static uint64_t diff_ms = 0;

    if (samples == 0)
    {
        clock_gettime(CLOCK_MONOTONIC, &tv_start);
    }

    clock_gettime(CLOCK_MONOTONIC, &tv_end);

    samples += len;
    diff_ms = (tv_end.tv_sec-tv_start.tv_sec)*1000 + ((tv_end.tv_nsec-tv_start.tv_nsec)/1000)/1000;

    if (diff_ms > LOG_RATE_MS)
    {
        DEBUG("Current sample rate: %llu kHz\n", samples/diff_ms);
        samples = 0;
    }
}

void decimate(const cmplx_u8* complex_signal, int len)
{
    decimator_decimate_cmplx_u8(decim, complex_signal, len);
}

void estimate_spectrum(const cmplx_u8* complex_signal, int len)
{
    static double power_spectrum[FFT_POINTS];
    int i = 0;
    int blocks = len / FFT_POINTS;
    blocks = blocks <= FFT_AVERAGE ? blocks : FFT_AVERAGE;
    memset(power_spectrum, 0, FFT_POINTS*sizeof(double));

    for (i = 0; i < blocks; i++)
    {
        if (spectrum_add_u8(spect, &(complex_signal[i*FFT_POINTS]), power_spectrum, FFT_POINTS))
        {
            ERROR("Error while estimating spectrum.\n");
            return;
        }
    }

    pthread_mutex_lock(&data_mutex);

    memcpy(power_spectrum_transfer, power_spectrum, FFT_POINTS*sizeof(double));
    spectrum_averaging_count = blocks;

    pthread_mutex_unlock(&data_mutex);
}

int write_spectrum_message(char* buf, int buf_len)
{
    static double power_spectrum_local[FFT_POINTS];
    int spectrum_averaging_count_local = 0;
    int idx = 0;
    int len = 0;
    double linear_energy_gain = pow(10, sw_gain/10);

    pthread_mutex_lock(&data_mutex);

    memcpy(power_spectrum_local, power_spectrum_transfer, FFT_POINTS*sizeof(double));
    spectrum_averaging_count_local = spectrum_averaging_count;

    pthread_mutex_unlock(&data_mutex);

    if (spectrum_averaging_count > 0)
    {
        sprintf(&buf[len], "d");
        len++;
        for (idx = 0; idx < FFT_POINTS; idx++)
        {
            int m = 10*log10(abs(linear_energy_gain * power_spectrum_local[idx] / spectrum_averaging_count_local));
            m = m >= 0 ? m : 0;
            m = m <= 255 ? m : 255;
            buf[len++] = (char) m;
        }
        spectrum_timestamp = timestamp();
    }

    return len;
}

int should_send_spectrum()
{
    if ((spectrum_timestamp + 250) < timestamp())
    {
        return 1;
    }
    return 0;
}

void channel_handler(const cmplx_s32* signal, int len)
{
    static double energy = 0;
    static struct timespec tv_start = { 0, 0 };
    static struct timespec tv_end = { 0, 0 };
    static uint64_t diff_ms = 0;
    int i = 0;

    if (tv_start.tv_sec == 0)
    {
        clock_gettime(CLOCK_MONOTONIC, &tv_start);
    }

    clock_gettime(CLOCK_MONOTONIC, &tv_end);

    for (i = 0; i < len; i++)
    {
        energy += (signal[i].re*signal[i].re + signal[i].im*signal[i].im);
    }

    diff_ms = (tv_end.tv_sec-tv_start.tv_sec)*1000 + ((tv_end.tv_nsec-tv_start.tv_nsec)/1000)/1000;

    if (diff_ms > LOG_RATE_MS)
    {
        DEBUG("Channel power: %f dB\n", 10*log10((energy/diff_ms) * 1000));
        energy = 0;
        memset(&tv_start, 0, sizeof(struct timespec));
    }
}

static int callback_rtl_ws(struct libwebsocket_context *context,
    struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason,
    void *user, void *in, size_t len)
{
    int f;
    int bw;
    int n, nn = 0;
    int do_write = 0;
    struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)user;
    char* buffer = NULL;
    char tmpbuffer[30];
    struct rtl_dev* dev = (struct rtl_dev*) libwebsocket_context_user(context);
    memset(tmpbuffer, 0, 30);

    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            pss->id = latest_user_id++;
            INFO("New connection, assigned ID == %d\n", pss->id);
            if (current_user_id == 0)
                current_user_id = pss->id;
            INFO("Current user ID == %d\n", current_user_id);
            if (current_user_id != pss->id)
            {
                return -1;
            }
            break;

        case LWS_CALLBACK_PROTOCOL_DESTROY:
            lwsl_notice("protocol cleaning up\n");
            send_data = 0;
            if (current_user_id == pss->id)
                current_user_id = 0;
            break;

            case LWS_CALLBACK_SERVER_WRITEABLE:
            if (should_send_spectrum())
            {
                memset(send_buffer, 0, LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING);
                n = sprintf(tmpbuffer, "f %u;b %u;s %d;", rtl_freq(dev), rtl_sample_rate(dev), sw_gain);
                memcpy(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING], tmpbuffer, n);
                nn = write_spectrum_message(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING+n], SEND_BUFFER_SIZE/2);
                n = libwebsocket_write(wsi, (unsigned char *)
                    &send_buffer[LWS_SEND_BUFFER_PRE_PADDING], n+nn, LWS_WRITE_BINARY);
            }

            usleep(5000);
            if (send_data)
                libwebsocket_callback_on_writable(context, wsi);

            break;

        case LWS_CALLBACK_RECEIVE:
            buffer = calloc(len+1, 1);
            memcpy(buffer, in, len);
            DEBUG("Got command: %s\n", buffer);
            if ((len >= strlen(FREQ_CMD)) && strncmp(FREQ_CMD, buffer, strlen(FREQ_CMD)) == 0)
            {
                f = atoi(&buffer[strlen(FREQ_CMD)])*1000;
                INFO("Trying to tune to %d Hz...\n", f);
                rtl_set_frequency(dev, f);
            }
            else if ((len >= strlen(SAMPLE_RATE_CMD)) && strncmp(SAMPLE_RATE_CMD, buffer, strlen(SAMPLE_RATE_CMD)) == 0)
            {
                bw = atoi(&buffer[strlen(SAMPLE_RATE_CMD)])*1000;
                INFO("Trying to set sample rate to %d Hz...\n", bw);
                rtl_set_sample_rate(dev, bw);
                decimator_set_parameters(decim, rtl_sample_rate(dev), DOWNFACTOR);
            }
            else if ((len >= strlen(SW_GAIN_CMD)) && strncmp(SW_GAIN_CMD, buffer, strlen(SW_GAIN_CMD)) == 0)
            {
                sw_gain = atoi(&buffer[strlen(SW_GAIN_CMD)]);
                INFO("Software gain set to %d dB\n", sw_gain);
            }
            else if ((len >= strlen(START_CMD)) && strncmp(START_CMD, buffer, strlen(START_CMD)) == 0)
            {
                send_data = 1;
                libwebsocket_callback_on_writable_all_protocol(libwebsockets_get_protocol(wsi));
            }
            else if ((len >= strlen(STOP_CMD)) && strncmp(STOP_CMD, buffer, strlen(STOP_CMD)) == 0)
            {
                send_data = 0;
            }
            free(buffer);
            break;

        default:
            break;
    }

    return 0;
}

void sighandler(int sig)
{
    force_exit = 1;
}

static struct option options[] = {
    { "help",	no_argument,		NULL, 'h' },
    { "debug",	required_argument,	NULL, 'd' },
    { "port",	required_argument,	NULL, 'p' },
    { "ssl",	no_argument,		NULL, 's' },
    { "interface",  required_argument,	NULL, 'i' },
    { "resource_path", required_argument,		NULL, 'r' },
    { NULL, 0, 0, 0 }
};


int main(int argc, char **argv)
{
    struct libwebsocket_protocols* protos = (struct libwebsocket_protocols*)
        calloc(3, sizeof(struct libwebsocket_protocols));
    struct rtl_dev* dev;
    char cert_path[1024];
    char key_path[1024];
    int n = 0;
    int use_ssl = 0;
    struct libwebsocket_context *context;
    int opts = 0;
    char interface_name[128] = "";
    const char *iface = NULL;
    int syslog_options = LOG_PID | LOG_PERROR;
    unsigned int oldus = 0;
    struct lws_context_creation_info info;
    int debug_level = 7;

    memset(&info, 0, sizeof info);
    info.port = PORT;

    INFO("Initializing RTL-device...\n");
    rtl_init(&dev, DEV_INDEX);

    INFO("Initializing decimator...\n");
    decim = decimator_alloc();
    decimator_set_parameters(decim, rtl_sample_rate(dev), DOWNFACTOR);
    decimator_add_callback(decim, channel_handler);

    info.user = dev;
    
    INFO("Initializing spectrum...\n");
    spect = spectrum_alloc(FFT_POINTS);
    
    send_buffer = calloc(LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING, 1);
    while (n >= 0)
    {
        n = getopt_long(argc, argv, "i:hsp:d:Dr:", options, NULL);
        if (n < 0)
            continue;
        switch (n)
        {
            case 'd':
            debug_level = atoi(optarg);
            break;
            case 's':
            use_ssl = 1;
            break;
            case 'p':
            info.port = atoi(optarg);
            break;
            case 'i':
            strncpy(interface_name, optarg, sizeof interface_name);
            interface_name[(sizeof interface_name) - 1] = '\0';
            iface = interface_name;
            break;
            case 'r':
            resource_path = optarg;
            printf("Setting resource path to \"%s\"\n", resource_path);
            break;
            case 'h':
            fprintf(stderr, "Usage: rtl-ws-server "
                "[--port=<p>] [--ssl] "
                "[-d <log bitfield>] "
                "[--resource_path <path>]\n");
            exit(1);
        }
    }

    signal(SIGINT, sighandler);

    /* we will only try to log things according to our debug_level */
    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("lwsts", syslog_options, LOG_DAEMON);

    /* tell the library what debug level to emit and to send it to syslog */
    lws_set_log_level(debug_level, lwsl_emit_syslog);

    info.iface = iface;
    memcpy(protos, get_http_protocol(), sizeof(struct libwebsocket_protocols));
    memcpy(&(protos[1]), protocols, 2 * sizeof(struct libwebsocket_protocols));
    info.protocols = protos;
    info.extensions = libwebsocket_get_internal_extensions();

    if (!use_ssl)
    {
        info.ssl_cert_filepath = NULL;
        info.ssl_private_key_filepath = NULL;
    }
    else
    {
        if (strlen(resource_path) > sizeof(cert_path) - 32)
        {
            lwsl_err("resource path too long\n");
            return -1;
        }
        sprintf(cert_path, "%s/libwebsockets-test-server.pem",
            resource_path);
        if (strlen(resource_path) > sizeof(key_path) - 32)
        {
            lwsl_err("resource path too long\n");
            return -1;
        }
        sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
            resource_path);

        info.ssl_cert_filepath = cert_path;
        info.ssl_private_key_filepath = key_path;
    }
    info.gid = -1;
    info.uid = -1;
    info.options = opts;

    context = libwebsocket_create_context(&info);
    if (context == NULL)
    {
        lwsl_err("libwebsocket init failed\n");
        return -1;
    }

    n = 0;
    pthread_mutex_init(&data_mutex, NULL);
    start_signal_source(dev);
    add_signal_callback(log_data_rate);
    add_signal_callback(decimate);
    add_signal_callback(estimate_spectrum);

    while (n >= 0 && !force_exit)
    {
        n = libwebsocket_service(context, 50);
    }

    force_exit = 1;
    remove_signal_callbacks();
    stop_signal_source(dev);

    libwebsocket_context_destroy(context);

    lwsl_notice("rtl-ws-server exited\n");

    closelog();

    rtl_close(dev);

    pthread_mutex_destroy(&data_mutex);

    free(send_buffer);
    spectrum_free(spect);
    free(protos);
    decimator_free(decim);
    return 0;
}

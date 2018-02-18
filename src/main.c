#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libwebsockets.h>

#include "log.h"
#include "rtl_sensor.h"
#include "http_handler.h"
#include "rf_decimator.h"

#include "audio_main.h"
#include "cbb_main.h"

#define SEND_BUFFER_SIZE        16384
#define FREQ_CMD                "freq"
#define SAMPLE_RATE_CMD         "bw"
#define SPECTRUM_GAIN_CMD       "spectrumgain"
#define START_CMD               "start"
#define STOP_CMD                "stop"
#define PORT                    8080
#define DECIMATED_TARGET_BW_HZ  192000

struct per_session_data__rtl_ws {
    int id;
    int send_data;
    int sent_audio_fragments;
};

static int current_user_id = 0;
static int latest_user_id = 1;

static volatile int force_exit = 0;
static volatile int spectrum_gain = 0;

static char* send_buffer = NULL;

static int callback_rtl_ws(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    int f = 0;
    int bw = 0;
    int n = 0, nn = 0, nnn = 0;
    struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)user;
    char* in_buffer = NULL;
    char tmpbuffer[30] = { 0 };
    struct rtl_dev* dev = cbb_get_rtl_dev();
    
    int audio_write_mode = LWS_WRITE_CONTINUATION;
    int should_sleep = 1;

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
            pss->send_data = 0;
            if (current_user_id == pss->id)
                current_user_id = 0;
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (pss->send_data)
            {
                if (cbb_new_spectrum_available() && pss->sent_audio_fragments == 0)
                {
                    memset(send_buffer, 0, LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING);
                    n = sprintf(tmpbuffer, "t s;f %u;b %u;s %d;d", rtl_freq(dev), rtl_sample_rate(dev), spectrum_gain);
                    memcpy(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING], tmpbuffer, n);
                    nn = cbb_get_spectrum_payload(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING+n], SEND_BUFFER_SIZE/2, spectrum_gain);
                    nnn = lws_write(wsi, (unsigned char *)
                        &send_buffer[LWS_SEND_BUFFER_PRE_PADDING], n+nn, LWS_WRITE_BINARY);
                }
                else if (audio_new_audio_available())
                {
                    memset(send_buffer, 0, LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING);
                    if (pss->sent_audio_fragments == 0)
                    {
                        n = sprintf(tmpbuffer, "FF;t a;d");
                        memcpy(&(send_buffer[LWS_SEND_BUFFER_PRE_PADDING]), tmpbuffer, n);
                        audio_write_mode = LWS_WRITE_BINARY;
                    }

                    nn = audio_get_audio_payload(&(send_buffer[LWS_SEND_BUFFER_PRE_PADDING+n]), 2048);
                    if (nn > 0)
                    {
                        if (pss->sent_audio_fragments < 7)
                        {
                            audio_write_mode |= LWS_WRITE_NO_FIN;
                            pss->sent_audio_fragments++;
                            should_sleep = 0;
                        }
                        else
                        {
                            pss->sent_audio_fragments = 0;
                        }
                        nnn = lws_write(wsi, (unsigned char *)
                            &(send_buffer[LWS_SEND_BUFFER_PRE_PADDING]), n + nn, audio_write_mode);
                    }
                    else 
                    {
                        DEBUG("No data available for write\n");
                        should_sleep = 0;
                    }
                }
            }

            if (nnn < 0) 
            {
                ERROR("Writing failed, error code == %d\n", nnn);
            }
            else if (nnn != n+nn)
            {
                ERROR("Partial write: Sent %d bytes, should have sent %d\n", nnn, n+nn);
            }

            if (should_sleep)
            {
                usleep(5000);
            }

            if (pss->send_data)
                lws_callback_on_writable(wsi);

            break;

        case LWS_CALLBACK_RECEIVE:
            in_buffer = calloc(len+1, 1);
            memcpy(in_buffer, in, len);
            DEBUG("Got command: %s\n", in_buffer);
            if ((len >= strlen(FREQ_CMD)) && strncmp(FREQ_CMD, in_buffer, strlen(FREQ_CMD)) == 0)
            {
                f = atoi(&in_buffer[strlen(FREQ_CMD)])*1000;
                INFO("Trying to tune to %d Hz...\n", f);
                rtl_set_frequency(dev, f);
            }
            else if ((len >= strlen(SAMPLE_RATE_CMD)) && strncmp(SAMPLE_RATE_CMD, in_buffer, strlen(SAMPLE_RATE_CMD)) == 0)
            {
                bw = atoi(&in_buffer[strlen(SAMPLE_RATE_CMD)])*1000;
                INFO("Trying to set sample rate to %d Hz...\n", bw);
                rtl_set_sample_rate(dev, bw);
                rf_decimator_set_parameters(cbb_rf_decimator(), rtl_sample_rate(dev), rtl_sample_rate(dev)/DECIMATED_TARGET_BW_HZ);
            }
            else if ((len >= strlen(SPECTRUM_GAIN_CMD)) && strncmp(SPECTRUM_GAIN_CMD, in_buffer, strlen(SPECTRUM_GAIN_CMD)) == 0)
            {
                spectrum_gain = atoi(&in_buffer[strlen(SPECTRUM_GAIN_CMD)]);
                INFO("Spectrum gain set to %d dB\n", spectrum_gain);
            }
            else if ((len >= strlen(START_CMD)) && strncmp(START_CMD, in_buffer, strlen(START_CMD)) == 0)
            {
                pss->send_data = 1;
                lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));
            }
            else if ((len >= strlen(STOP_CMD)) && strncmp(STOP_CMD, in_buffer, strlen(STOP_CMD)) == 0)
            {
                pss->send_data = 0;
            }
            free(in_buffer);
            break;

        default:
            break;
    }

    return 0;
}

static void sighandler(int sig)
{
    force_exit = 1;
}

int main(int argc, char **argv)
{
    struct lws_protocols ws_protocol = {
        "rtl-ws-protocol",
        callback_rtl_ws,
        sizeof(struct per_session_data__rtl_ws)
    };
    struct lws_protocols protos[3] = { 0 };
    int status = 0;
    struct lws_context *context = NULL;
    struct lws_context_creation_info info = { 0 };

    send_buffer = calloc(LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING, 1);
    
    INFO("Initializing audio processing...\n");
    audio_init();
    
    INFO("Initializing complex baseband processing...\n");
    cbb_init(DECIMATED_TARGET_BW_HZ);

    rf_decimator_add_callback(cbb_rf_decimator(), audio_fm_demodulator);

    signal(SIGINT, sighandler);

    memcpy(protos, get_http_protocol(), sizeof(struct lws_protocols));
    memcpy(&(protos[1]), &ws_protocol, sizeof(struct lws_protocols));
    
    info.port = PORT;
    info.protocols = protos;
    info.extensions = NULL;

    info.gid = -1;
    info.uid = -1;

    INFO("Initializing websockets...\n");
    context = lws_create_context(&info);
    if (context == NULL)
    {
        ERROR("Websocket init failed\n");
        return -1;
    }

    while (status >= 0 && !force_exit)
    {
        status = lws_service(context, 10);
    }

    free(send_buffer);
    
    INFO("Closing complex baseband processing...\n");
    cbb_close();
    
    INFO("Closing audio processing...\n");
    audio_close();
    
    INFO("Destroying libwebsocket context\n");
    lws_context_destroy(context);

    return 0;
}

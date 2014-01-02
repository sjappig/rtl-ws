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

#define HTML_FILE 				"/rtl-ui.html"
#define DEV_INDEX 				0

#define DEFAULT_BUF_LENGTH		(16 * 16384)
#define SEND_BUFFER_SIZE		(2 * 16384)
#define FREQ_CMD 				"freq"
#define SAMPLE_RATE_CMD         "bw"
#define START_CMD				"start"
#define STOP_CMD				"stop"
#define FFT_POINTS				1024
#define PORT 					80
#define FM_BW                   128000
#define LOCAL_RESOURCE_PATH 	INSTALL_DATADIR

struct transfer__ {
	uint8_t* buf;
	int n_read;
	uint64_t block_id;
};

struct per_session_data__http {
	int fd;
};

struct per_session_data__rtl_ws {
	int id;
    uint64_t block_id;
};


static int callback_http(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
		void *in, size_t len);

static int callback_rtl_ws(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user, 
		void *in, size_t len);

static struct libwebsocket_protocols protocols[] = {
	{
		"http-only",
		callback_http,
		sizeof (struct per_session_data__http),
		1024
	},
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

static volatile struct transfer__ transfer;
static pthread_mutex_t data_mutex;
static char* send_buffer = NULL;
static spectrum_handle spectrum;

static uint64_t spectrum_timestamp = 0;
static char* spectrum_temp_buffer = NULL;

static char *resource_path = LOCAL_RESOURCE_PATH;

void *rtl_worker(void* user) 
{
 	int r = 0;
 	int n_read = 0;
 	uint64_t bytes = 0;
 	uint32_t out_block_size = DEFAULT_BUF_LENGTH;
 	uint8_t* rtl_buffer = malloc(out_block_size * sizeof(uint8_t));
 	struct timespec tv_start = {0,0};
 	struct timespec tv_end = {0,0};
 	uint64_t diff_us;

 	transfer.buf = malloc(out_block_size * sizeof(uint8_t));
 	printf("rtl worker started\n");
 	fflush(stdout);
 	while (!force_exit && r >= 0) 
 	{
 		if (bytes == 0) 
 		{
 			clock_gettime(CLOCK_MONOTONIC, &tv_start);
 		}

 		r = rtl_read(rtl_buffer, out_block_size, &n_read);
 		n_read = n_read > out_block_size ? out_block_size : n_read;
 		clock_gettime(CLOCK_MONOTONIC, &tv_end);
 		bytes += n_read;
 		if (r < 0) {
 			fprintf(stderr, "WARNING: read failed. Exiting worker thread.\n");
 			continue;
 		}
 		pthread_mutex_lock(&data_mutex);
 		memcpy(transfer.buf, rtl_buffer, n_read);
 		transfer.n_read = n_read;
 		transfer.block_id++;
 		pthread_mutex_unlock(&data_mutex);
 		diff_us = (tv_end.tv_sec-tv_start.tv_sec)*1000*1000 + (tv_end.tv_nsec-tv_start.tv_nsec)/1000;
 		if (diff_us > 10000000) 
 		{
 			printf("%llu bytes / %d us : %llu bytes/sec\n", bytes, diff_us, (bytes*1000*1000)/diff_us);
 			fflush(stdout);
 			bytes = 0;
 		}
 	}
 	free(rtl_buffer);
}

void *fm_worker(void* user) {
	FILE* fid = fopen("fm_dump.bin", "w");
	int r = 0;
	int orig_bw = 0;
	int fm_band_block_size = 0;
	uint32_t out_block_size = DEFAULT_BUF_LENGTH;
	uint8_t* orig_buffer = malloc(out_block_size * sizeof(uint8_t));
 	cmplx_s32* fm_buffer = NULL;
 	int resample_ratio = 0;
 	uint64_t last_block_id = 0;

	memset(orig_buffer, 0, out_block_size * sizeof(uint8_t));
	printf("fm worker started\n");
 	fflush(stdout);
 	while (!force_exit && r >= 0) {
 		if (orig_bw != rtl_sample_rate()) 
 		{
 			if (fm_buffer)
 				free(fm_buffer);
 			orig_bw = rtl_sample_rate();
 			resample_ratio = (orig_bw / FM_BW);
 			fm_band_block_size = out_block_size / (2*resample_ratio);
 			fm_buffer = malloc(fm_band_block_size*sizeof(cmplx_s32));
 			printf("out_block_size == %d\n", out_block_size);
 			printf("fm_band_block_size == %d\n", fm_band_block_size);
 			printf("resample_ratio == %d\n", resample_ratio);
 		}
 		if (last_block_id < transfer.block_id) {
 			pthread_mutex_lock(&data_mutex);
 			memcpy(orig_buffer, transfer.buf, transfer.n_read);
 			last_block_id = transfer.block_id;
 			pthread_mutex_unlock(&data_mutex);
 			r = cic_decimate(resample_ratio, 1, (cmplx*) orig_buffer, out_block_size/2, fm_buffer, fm_band_block_size);
 			//fwrite(fm_buffer, fm_band_block_size, sizeof(cmplx_s32), fid);
 		}
 	}
 	free(fm_buffer);
 	free(orig_buffer);
 	printf("fm worker exited\n");
 	fclose(fid);
 	fflush(stdout);
 }

const char * get_mimetype(const char *file)
{
	int n = strlen(file);

	if (n < 5)
		return NULL;

	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";

	if (!strcmp(&file[n - 3], ".js"))
		return "text/javascript";

	return NULL;
}

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
		void *in, size_t len)
{
	char buf[256];
	char leaf_path[1024];
	char b64[64];
	struct timeval tv;
	int n, m;
	unsigned char *p;
	static unsigned char buffer[4096];
	struct stat stat_buf;
	struct per_session_data__http *pss =
			(struct per_session_data__http *)user;
	const char *mimetype;

	switch (reason) {
	case LWS_CALLBACK_HTTP:

		if (len < 1) {
			libwebsockets_return_http_status(context, wsi,
						HTTP_STATUS_BAD_REQUEST, NULL);
			return -1;
		}

		/* this server has no concept of directories */
		if (strchr((const char *)in + 1, '/')) {
			libwebsockets_return_http_status(context, wsi,
						HTTP_STATUS_FORBIDDEN, NULL);
			return -1;
		}

		/* if a legal POST URL, let it continue and accept data */
		if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
			return 0;

		/* if not, send a file the easy way */
		strcpy(buf, resource_path);
		if (strcmp(in, "/")) {
			if (*((const char *)in) != '/')
				strcat(buf, "/");
			strncat(buf, in, sizeof(buf) - strlen(resource_path));
		} else /* default file to serve */
			strcat(buf, HTML_FILE);
		buf[sizeof(buf) - 1] = '\0';

		/* refuse to serve files we don't understand */
		mimetype = get_mimetype(buf);
		if (!mimetype) {
			lwsl_err("Unknown mimetype for %s\n", buf);
			libwebsockets_return_http_status(context, wsi,
				      HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
			return -1;
		}

		if (libwebsockets_serve_http_file(context, wsi, buf,
						mimetype, NULL))
			return -1; /* through completion or error, close the socket */

		break;

	case LWS_CALLBACK_HTTP_BODY:
		strncpy(buf, in, 20);
		buf[20] = '\0';
		if (len < 20)
			buf[len] = '\0';

		lwsl_notice("LWS_CALLBACK_HTTP_BODY: %s... len %d\n",
				(const char *)buf, (int)len);

		break;

	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		lwsl_notice("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
		/* the whole of the sent body arried, close the connection */
		libwebsockets_return_http_status(context, wsi,
						HTTP_STATUS_OK, NULL);

		return -1;

	case LWS_CALLBACK_HTTP_FILE_COMPLETION:
		/* kill the connection after we sent one file */
		return -1;

	case LWS_CALLBACK_HTTP_WRITEABLE:
		do {
			n = read(pss->fd, buffer, 4096);
			if (n <= 0) {
				close(pss->fd);
				return -1;
			}
			m = libwebsocket_write(wsi, buffer, n, LWS_WRITE_HTTP);
			if (m < 0) {
				close(pss->fd);
				return -1;
			}
			if (m != n)
				/* partial write, adjust */
				lseek(pss->fd, m - n, SEEK_CUR);

		} while (!lws_send_pipe_choked(wsi));
		libwebsocket_callback_on_writable(context, wsi);
		break;

	default:
		break;
	}

	return 0;
}

int should_write()
{
   if ((spectrum_timestamp + 200) < timestamp()) 
   {
      return 1;
   }
   
   return 0;
}

int write_spectrum_message(char* buf, int buf_len) {
        char* p = buf;
	char convert_buf[10];
	int idx = 0;
	double magnitude[1024];
    int N = FFT_POINTS*2*4;
	pthread_mutex_lock(&data_mutex);
    memcpy(spectrum_temp_buffer, transfer.buf, N);
    pthread_mutex_unlock(&data_mutex);
    sprintf(p++, "d");
	calculate_spectrum(spectrum, (cmplx*)spectrum_temp_buffer, N, magnitude, 1024);
	for (idx = 0; idx < 1024; idx++) {
		sprintf(convert_buf, "%d", (int)magnitude[idx]);
		sprintf(p++, " %s", convert_buf);
		p += strlen(convert_buf);
		if ((int)(p - buf) > (buf_len - 10))
			break;
	}
	sprintf(p++, ";");
    spectrum_timestamp = timestamp();
	return (int)(p - buf);
}

static int callback_rtl_ws(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user, 
		void *in, size_t len)
{
	int f;
        int bw;
	int n, nn = 0;
        int do_write = 0;
	struct per_session_data__rtl_ws *pss = (struct per_session_data__rtl_ws *)user;
	char* buffer = NULL;
	char tmpbuffer[30];
	memset(tmpbuffer, 0, 30);

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("protocol starting\n");
		pss->id = latest_user_id++;
	        pss->block_id = 0;
		printf("id: %d\n", pss->id);
		if (current_user_id == 0) 
			current_user_id = pss->id;
		printf("cid: %d\n", current_user_id);
		if (current_user_id != pss->id) {
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
		pthread_mutex_lock(&data_mutex);
	        do_write = transfer.block_id > pss->block_id && should_write();
	        pss->block_id = transfer.block_id;
		pthread_mutex_unlock(&data_mutex);
	        if (do_write) {
			memset(send_buffer, 0, LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING);
			n = sprintf(tmpbuffer, "f %u;b %u;", rtl_freq(), rtl_sample_rate());
			memcpy(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING], tmpbuffer, n);
			nn = write_spectrum_message(&send_buffer[LWS_SEND_BUFFER_PRE_PADDING+n], SEND_BUFFER_SIZE/2);

		        n = libwebsocket_write(wsi, (unsigned char *)
				&send_buffer[LWS_SEND_BUFFER_PRE_PADDING], n+nn, LWS_WRITE_TEXT);
		}	
		
	        usleep(1);
		if (send_data)
			libwebsocket_callback_on_writable(context, wsi);
		break;

	case LWS_CALLBACK_RECEIVE:
		buffer = malloc(len+1);
		memset(buffer, 0, len+1);
		memcpy(buffer, in, len);
		printf("%s\n", buffer);
		if ((len >= strlen(FREQ_CMD)) && strncmp(FREQ_CMD, buffer, strlen(FREQ_CMD)) == 0) {
			f = atoi(&buffer[strlen(FREQ_CMD)])*1000;
			printf("Trying to tune to %d Hz...\n", f);
			rtl_tune(f);
		} else if ((len >= strlen(SAMPLE_RATE_CMD)) && strncmp(SAMPLE_RATE_CMD, buffer, strlen(SAMPLE_RATE_CMD)) == 0) {
			bw = atoi(&buffer[strlen(SAMPLE_RATE_CMD)])*1000;
			printf("Trying to set sample rate to %d Hz...\n", bw);
			rtl_set_sample_rate(bw);
		} else if ((len >= strlen(START_CMD)) && strncmp(START_CMD, buffer, strlen(START_CMD)) == 0) {
			send_data = 1;
			libwebsocket_callback_on_writable_all_protocol(
				libwebsockets_get_protocol(wsi));
		} else if ((len >= strlen(STOP_CMD)) && strncmp(STOP_CMD, buffer, strlen(STOP_CMD)) == 0) {
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
	rtl_cancel();
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
	pthread_t rtl_thread; 
	pthread_t fm_thread;
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

	rtl_init(DEV_INDEX);
	transfer.buf = NULL;
 	transfer.block_id = 0;

	spectrum = init_spectrum(FFT_POINTS);
        spectrum_temp_buffer = malloc(FFT_POINTS*2*4);
	send_buffer = malloc(LWS_SEND_BUFFER_PRE_PADDING + SEND_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING);
	while (n >= 0) {
		n = getopt_long(argc, argv, "i:hsp:d:Dr:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
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
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();

	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
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
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}

	n = 0;
	pthread_mutex_init(&data_mutex, NULL);
	pthread_create(&rtl_thread, NULL, rtl_worker, NULL);
	pthread_create(&fm_thread, NULL, fm_worker, NULL);

	while (n >= 0 && !force_exit) {
		n = libwebsocket_service(context, 50);
	}
	force_exit = 1;
	usleep(100);

	libwebsocket_context_destroy(context);

	lwsl_notice("rtl-ws-server exited\n");

	closelog();

	rtl_close();

	pthread_mutex_destroy(&data_mutex);
	pthread_exit(NULL);

	if (transfer.buf != NULL) {
		free(transfer.buf);
		transfer.buf = NULL;
		transfer.block_id = 0;
	}

	free(send_buffer);
	free_spectrum(spectrum);
    free(spectrum_temp_buffer);
	return 0;
}
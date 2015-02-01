#ifndef HTTP_HANDLER
#define HTTP_HANDLER

#include <libwebsockets.h>

struct libwebsocket_protocols* get_http_protocol();

int callback_http(struct libwebsocket_context *context, struct libwebsocket *wsi,
    enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len);

#endif

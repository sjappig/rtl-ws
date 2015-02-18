#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <libwebsockets.h>

struct libwebsocket_protocols* get_http_protocol();

int callback_http(struct libwebsocket_context *context, struct libwebsocket *wsi,
    enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len);

#endif

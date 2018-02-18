#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <libwebsockets.h>

struct lws_protocols* get_http_protocol();

int callback_http(struct lws *wsi,
    enum lws_callback_reasons reason, void *user, void *in, size_t len);

#endif

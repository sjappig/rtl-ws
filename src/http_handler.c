#include <string.h>
#include <stdio.h>

#include "http_handler.h"

#define MAIN_HTML   "/rtl-ui.html"
#define HTML_PATH   "../resources"

struct per_session_data__http 
{
    int fd;
};

static struct lws_protocols http_protocol =
    { "http-only", callback_http, sizeof (struct per_session_data__http) };

static const char * get_mimetype(const char *file)
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

struct lws_protocols* get_http_protocol()
{
    return &http_protocol;
}

int callback_http(struct lws *wsi,
    enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    static unsigned char buffer[4096];
    char buf[256];
    int n = 0;
    int m = 0;
    struct per_session_data__http *pss = (struct per_session_data__http *) user;
    const char *mimetype = NULL;

    switch (reason) 
    {
        case LWS_CALLBACK_HTTP:

            if (len < 1) 
            {
                lws_return_http_status(wsi,
                    HTTP_STATUS_BAD_REQUEST, NULL);
                return -1;
            }

            /* this server has no concept of directories */
            if (strchr((const char *) in + 1, '/')) 
            {
                lws_return_http_status(wsi,
                    HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }

            /* if a legal POST URL, let it continue and accept data */
            if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
                return 0;

            /* if not, send a file the easy way */
            strcpy(buf, HTML_PATH);
            if (strcmp(in, "/"))
            {
                if (*((const char *)in) != '/')
                    strcat(buf, "/");

                strncat(buf, in, sizeof(buf) - strlen(HTML_PATH));
            } 
            else 
            {
                /* default file to serve */
                strcat(buf, MAIN_HTML);
            }

            buf[sizeof(buf) - 1] = '\0';

            /* refuse to serve files we don't understand */
            mimetype = get_mimetype(buf);
            if (!mimetype) 
            {
                lwsl_err("Unknown mimetype for %s\n", buf);
                lws_return_http_status(wsi,
                    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
                return -1;
            }

            if (lws_serve_http_file(wsi, buf, mimetype, NULL, 0))
            {
                return -1; /* through completion or error, close the socket */
            }

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
            lws_return_http_status(wsi, HTTP_STATUS_OK, NULL);

            return -1;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            /* kill the connection after we sent one file */
            return -1;

        case LWS_CALLBACK_HTTP_WRITEABLE:
            do 
            {
                n = read(pss->fd, buffer, 4096);
                if (n <= 0) 
                {
                    close(pss->fd);
                    return -1;
                }
                m = lws_write(wsi, buffer, n, LWS_WRITE_HTTP);
                if (m < 0) 
                {
                    close(pss->fd);
                    return -1;
                }
                if (m != n)
                {
                    /* partial write, adjust */
                    lseek(pss->fd, m - n, SEEK_CUR);
                }

            } while (!lws_send_pipe_choked(wsi));

            lws_callback_on_writable(wsi);

            break;

        default:
            break;
    }

    return 0;
}

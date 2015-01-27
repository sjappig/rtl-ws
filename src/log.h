#ifndef LOG_H
#define LOG_H

#ifdef RTL_WS_DEBUG
#define DEBUG(...) printf(__VA_ARGS__); fflush(stdout);
#else
#define DEBUG(x)
#endif

#endif

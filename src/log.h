#ifndef LOG_H
#define LOG_H
#include <stdio.h>

#ifdef RTL_WS_DEBUG
#define DEBUG(...) printf("DEBUG "); printf(__VA_ARGS__); fflush(stdout);
#else
#define DEBUG(...)
#endif

#define INFO(...) printf("INFO "); printf(__VA_ARGS__); fflush(stdout); 

#define ERROR(...) printf("ERROR "); fprintf(stderr, __VA_ARGS__); fflush(stdout); 

#endif

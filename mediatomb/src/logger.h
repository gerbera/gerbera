#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif
#include <stdio.h>

extern FILE *LOG_FILE;

void log_open(char *filename);
void log_close();

//#define LOG_ENABLED
#define LOG_FLUSH 1

#ifdef LOG_ENABLED

#define log_info(format, ...) _log_info(format, ## __VA_ARGS__) 
#define log_warning(format, ...) _log_warning(format, ## __VA_ARGS__) 
#define log_error(format, ...) _log_error(format, ## __VA_ARGS__)

#ifdef LOG_TOMBDEBUG

//    #define log_debug(args) _log_debug args
    #define log_debug(format, ...) _log_debug(format, __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
    #define log_debug(format, ...)
#endif

#else

#define log_info(format, ...)
#define log_warning(format, ...)
#define log_error(format, ...)
#define log_debug(format, ...)

#endif


void _log_info(const char *format, ...);
void _log_warning(const char *format, ...);
void _log_error(const char *format, ...);
void _log_debug(const char *format, const char* file, int line, const char *function, ...);
        

#endif // __LOGGER_H__


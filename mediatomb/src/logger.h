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

#define log_info(args) _log_info args
#define log_warning(args) _log_warning args
#define log_error(args) _log_error args

#ifdef LOG_DEBUG
//    #define log_debug(args) _log_debug args
    #define log_debug(format, ...) _log_debug(format, __FILE__, __LINE__, ## __VA_ARGS__)
#else
    #define log_debug(format, ...)
#endif

#else

#define log_info(args)
#define log_warning(args)
#define log_error(args)
#define log_debug(format, ...)

#endif


void _log_info(const char *format, ...);
void _log_warning(const char *format, ...);
void _log_error(const char *format, ...);
void _log_debug(const char *format, const char* file, int line, ...);
        

#endif // __LOGGER_H__


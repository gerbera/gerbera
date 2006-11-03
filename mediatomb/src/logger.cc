/*MT*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "logger.h"

FILE *LOG_FILE = stderr;

#ifdef LOG_FLUSH
#define FLUSHIT fflush(LOG_FILE);
#else
#define FLUSHIT
#endif

void log_open(char *filename)
{
    LOG_FILE = fopen(filename, "a");
    if (! LOG_FILE)
    {
        fprintf(stderr, "Could not open log file %s : %s\n",
                filename, strerror(errno));
        exit(1);
    }
}
void log_close()
{
    if (LOG_FILE)
        fclose(LOG_FILE);
}

static void log_stamp(const char *type)
{
    time_t unx;
    struct tm t;
    time(&unx);
    localtime_r(&unx, &t);
    fprintf(LOG_FILE, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %*s: ",
           t.tm_year + 1900,
           t.tm_mon,
           t.tm_mday,
           t.tm_hour,
           t.tm_min,
           t.tm_sec,
           7, // max length we have is "WARNING"
           type);
}
        
void _log_info(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_stamp("INFO");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_warning(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_stamp("WARNING");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_error(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_stamp("ERROR");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_debug(const char *format, const char *file, int line, const char *function, ...)
{
    va_list ap;
    va_start(ap, function);
    log_stamp("DEBUG");
    fprintf(LOG_FILE, "[%s:%d] %s(): ", file, line, function);
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}



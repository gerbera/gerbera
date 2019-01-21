/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    logger.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file logger.cc

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include "config_manager.h"
#include "logger.h"

FILE* LOG_FILE = stderr;

#define FLUSHIT fflush(LOG_FILE);

#define LOGCHECK   \
    if (!LOG_FILE) \
        return;

void log_open(const char* filename)
{
    LOG_FILE = fopen(filename, "a");
    if (!LOG_FILE) {
        fprintf(stderr, "Could not open log file %s : %s\n",
            filename, strerror(errno));
        exit(1);
    }
}
void log_close()
{
    if (LOG_FILE) {
        fclose(LOG_FILE);
        LOG_FILE = nullptr;
    }
}

static void log_stamp(const char* type)
{
    time_t unx;
    struct tm t;
    time(&unx);
    localtime_r(&unx, &t);
    fprintf(LOG_FILE, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %*s: ",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        7, // max length we have is "WARNING"
        type);
}

void _log_info(const char* format, ...)
{
    va_list ap;
    LOGCHECK
    va_start(ap, format);
    log_stamp("INFO");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_warning(const char* format, ...)
{
    va_list ap;
    LOGCHECK
    va_start(ap, format);
    log_stamp("WARNING");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_error(const char* format, ...)
{
    va_list ap;
    LOGCHECK
    va_start(ap, format);
    log_stamp("ERROR");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_js(const char* format, ...)
{
    va_list ap;
    LOGCHECK
    va_start(ap, format);
    log_stamp("JS");
    vfprintf(LOG_FILE, format, ap);
    FLUSHIT
    va_end(ap);
}
void _log_debug(const char* format, const char* file, int line, const char* function, ...)
{
    bool enabled;
    enabled = ConfigManager::isDebugLogging();
    if (enabled) {
        va_list ap;
        LOGCHECK
        va_start(ap, function);
        log_stamp("DEBUG");
        fprintf(LOG_FILE, "[%s:%d] %s(): ", file, line, function);
        vfprintf(LOG_FILE, format, ap);
        FLUSHIT
        va_end(ap);
    }
}

void _print_backtrace(FILE* file)
{
#if defined HAVE_BACKTRACE && defined HAVE_BACKTRACE_SYMBOLS

    bool enabled;
    enabled = ConfigManager::isDebugLogging();
    if (enabled) {
        void* b[100];
        int size = backtrace(b, 100);
        char** s = backtrace_symbols(b, size);
        for (int i = 0; i < size; i++)
            fprintf(file, "_STRACE_ %i %s\n", i, s[i]);
        free(s);
    }

#endif
}

/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    logger.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file logger.h

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
    #define print_backtrace() _print_backtrace()
#else
    #define log_debug(format, ...)
    #define print_backtrace()
#endif

#else

#define log_info(format, ...)
#define log_warning(format, ...)
#define log_error(format, ...)
#define log_debug(format, ...)
#define print_backtrace()

#endif


void _log_info(const char *format, ...);
void _log_warning(const char *format, ...);
void _log_error(const char *format, ...);
void _log_debug(const char *format, const char* file, int line, const char *function, ...);
void _print_backtrace();

#endif // __LOGGER_H__


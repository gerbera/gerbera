/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    logger.h - this file is part of MediaTomb.
    
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

/// \file logger.h

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>

extern FILE* LOG_FILE;

void log_open(const char* filename);
void log_close();

#define log_info(format, ...) _log_info(format, ##__VA_ARGS__)
#define log_warning(format, ...) _log_warning(format, ##__VA_ARGS__)
#define log_error(format, ...) _log_error(format, ##__VA_ARGS__)
#define log_js(format, ...) _log_js(format, ##__VA_ARGS__)

#ifdef TOMBDEBUG
#define log_debug(format, ...) _log_debug(format, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)
#define print_backtrace() _print_backtrace()
#else
#define log_debug(format, ...)
#define print_backtrace()
#endif

void _log_info(const char* format, ...);
void _log_warning(const char* format, ...);
void _log_error(const char* format, ...);
void _log_js(const char* format, ...);
void _log_debug(const char* format, const char* file, int line, const char* function, ...);
void _print_backtrace(FILE* file = LOG_FILE);

#endif // __LOGGER_H__

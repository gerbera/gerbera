/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    memory.h - this file is part of MediaTomb.
    
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

/// \file memory.h

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdlib.h>

#ifndef MEMPROF

#ifdef TOMBDEBUG
void* MALLOC(size_t size);
void* REALLOC(void* ptr, size_t size);
#else
#define MALLOC malloc
#define REALLOC realloc
#endif
#define CALLOC calloc
#define FREE free

#else

void* MALLOC(size_t size);
void* CALLOC(size_t nmemb, size_t size);
void* REALLOC(void* ptr, size_t size);
void FREE(void* ptr);

#endif

#ifndef HAVE_MALLOC
void* rpl_malloc(size_t size);
#endif

#ifndef HAVE_REALLOC
void* rpl_realloc(void* p, size_t size);
#endif

#ifndef HAVE_CALLOC
void* rpl_calloc(size_t n, size_t s);
#endif
#endif // __MEMORY_H__

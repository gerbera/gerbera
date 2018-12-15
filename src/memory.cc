/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    memory.cc - this file is part of MediaTomb.
    
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

/// \file memory.cc

#if defined(TOMBDEBUG) && !defined(MEMPROF)
#include "logger.h"
#include <cstdlib>
#endif

#ifdef MEMPROF

static int mallocCount = 0;
static int freeCount = 0;
static int totalAlloc = 0;
static int curAlloc = 0;
static int maxAlloc = 0;

static void print_stats()
{
    printf("MAL:%d FRE:%d CUR:%d TOT:%d MAX:%d\n",
        mallocCount, freeCount, curAlloc, totalAlloc, maxAlloc);
}

void* MALLOC(size_t size)
{
    mallocCount++;
    totalAlloc += size;
    curAlloc += size;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    void* ptr = malloc(size + sizeof(int));
    *((int*)ptr) = size;
    print_stats();
    return ((void*)((int*)ptr + 1));
}
void* CALLOC(size_t nmemb, size_t size)
{
    mallocCount++;
    totalAlloc += size * nmemb;
    curAlloc += size * nmemb;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    void* ptr = calloc(nmemb, size + sizeof(int));
    *((int*)ptr) = size;
    print_stats();
    return ((void*)((int*)ptr + 1));
}
void* REALLOC(void* ptr, size_t size)
{
    freeCount++;
    mallocCount++;
    int previous = *((int*)ptr - 1);
    totalAlloc += size - previous;
    curAlloc += size - previous;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    ptr = realloc((void*)((int*)ptr - 1), size + sizeof(int));
    *((int*)ptr) = size;
    print_stats();
    return ((void*)((int*)ptr + 1));
}
void FREE(void* ptr)
{
    freeCount++;
    int size = *((int*)ptr - 1);
    curAlloc -= size;
    print_stats();
    return free((void*)((int*)ptr - 1));
}

#else

#ifdef TOMBDEBUG
void* MALLOC(size_t size)
{
#ifdef DEBUG_MALLOC_0
    if (size <= 0) {
        printf("malloc called with 0! aborting...\n");
        _print_backtrace(stderr);
        abort();
    }
#endif
    return malloc(size);
}
void* REALLOC(void* ptr, size_t size)
{
#ifdef DEBUG_MALLOC_0
    if (size <= 0) {
        printf("realloc called with 0! aborting...\n");
        _print_backtrace(stderr);
        abort();
    }
#endif
    return realloc(ptr, size);
}

#endif // TOMBDEBUG

#endif // MEMPROF

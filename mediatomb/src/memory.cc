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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <stdio.h>

#include "memory.h"

#ifndef HAVE_MALLOC
void *rpl_malloc(size_t size)
{
    if (size == 0)
        ++size;

    return malloc(size);
}
#endif

#ifndef HAVE_REALLOC
void *rpl_realloc(void *p, size_t size)
{
    if (size == 0)
        ++size;
    
    return realloc(p, size);
}
#endif

/* calloc() function that is glibc compatible.
   This wrapper function is required at least on Tru64 UNIX 5.1.
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering */


/* Allocate and zero-fill an NxS-byte block of memory from the heap.
   If N or S is zero, allocate and zero-fill a 1-byte block.  */

#if defined(TOMBDEBUG) && ! defined (MEMPROF)
#include "logger.h"
#endif

#ifndef HAVE_CALLOC
void *rpl_calloc(size_t n, size_t s)
{
    size_t bytes;

    if (n == 0 || s == 0)
        return calloc (1, 1);

    /* Defend against buggy calloc implementations that mishandle
       size_t overflow.  */
    bytes = n * s;
    if (bytes / s != n)
        return NULL;

    return calloc (n, s);
}
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

void *MALLOC(size_t size)
{
    mallocCount++;
    totalAlloc += size;
    curAlloc += size;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    void *ptr = malloc(size + sizeof(int));
    *((int *)ptr) = size;
    print_stats();
    return ( (void *)((int *)ptr + 1) );
}
void *CALLOC(size_t nmemb, size_t size)
{
    mallocCount++;
    totalAlloc += size * nmemb;
    curAlloc += size * nmemb;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    void *ptr = calloc(nmemb, size + sizeof(int));
    *((int *)ptr) = size;
    print_stats();
    return ( (void *)((int *)ptr + 1) );
}
void *REALLOC(void *ptr, size_t size)
{
    freeCount++;
    mallocCount++;
    int previous = *((int *)ptr - 1);
    totalAlloc += size - previous;
    curAlloc += size - previous;
    if (curAlloc > maxAlloc)
        maxAlloc = curAlloc;
    ptr = realloc((void *)((int *)ptr - 1), size + sizeof(int));
    *((int *)ptr) = size;
    print_stats();
    return ( (void *)((int *)ptr + 1) );
}
void FREE(void *ptr)
{
    freeCount++;
    int size = *((int *)ptr - 1);
    curAlloc -= size;
    print_stats();
    return free( (void *)((int *)ptr - 1) );
}

#else

#ifdef TOMBDEBUG
void *MALLOC(size_t size)
{
#ifdef DEBUG_MALLOC_0    
    if (size <= 0)
    {
        printf("malloc called with 0! aborting...\n");
        _print_backtrace(stderr);
        abort();
    }
#endif
    return malloc(size);
}
void *REALLOC(void *ptr, size_t size)
{
#ifdef DEBUG_MALLOC_0
    if (size <= 0)
    {
        printf("realloc called with 0! aborting...\n");
        _print_backtrace(stderr);
        abort();
    }
#endif
    return realloc(ptr, size);
}

#endif // TOMBDEBUG

#endif // MEMPROF

/*  object.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
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
*/

#include <stdio.h>

#include "memory.h"

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

#endif


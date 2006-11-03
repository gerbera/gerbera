/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    memory.h - this file is part of MediaTomb.
    
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

/// \file memory.h

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "autoconfig.h"
#include <stdlib.h>

#if (!defined(HAVE_MALLOC)) || (!defined(HAVE_REALLOC)) 
    #include <sys/types.h>
#else
    #define HAVE_CALLOC
#endif

#ifndef HAVE_MALLOC
    #undef malloc
    #define malloc rpl_malloc
#endif

#ifndef HAVE_REALLOC
    #undef realloc
    #define realloc rpl_realloc
#endif

#ifndef HAVE_CALLOC
    #undef calloc
    #define calloc rpl_calloc
#endif


#ifndef MEMPROF

#define MALLOC malloc
#define CALLOC calloc
#define REALLOC realloc
#define FREE free

#else

void *MALLOC(size_t size);
void *CALLOC(size_t nmemb, size_t size);
void *REALLOC(void *ptr, size_t size);
void FREE(void *ptr);

#endif

#ifndef HAVE_MALLOC
void *rpl_malloc(size_t size);
#endif

#ifndef HAVE_REALLOC
void *rpl_realloc(void *p, size_t size);
#endif

#ifndef HAVE_CALLOC
void *rpl_calloc(size_t n, size_t s);
#endif
#endif // __MEMORY_H__


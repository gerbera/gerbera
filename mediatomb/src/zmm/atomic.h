/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    atomic.h - this file is part of MediaTomb.
    
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

/// \file atomic.h

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

typedef struct { volatile int x; } mt_atomic_t;
//#define ATOMIC_INIT(y) {(y)}

static inline void atomic_set(mt_atomic_t *at, int val)
{
    at->x = val;
}

static inline int atomic_get(mt_atomic_t *at)
{
    return (at->x);
}

#undef ATOMIC_DEFINED

#ifdef ATOMIC_X86_SMP
    #ifdef ATOMIC_X86
        #error ATOMIC_X86_SMP and ATOMIC_X86 are defined at the same time!
    #endif
    #define ASM_LOCK "lock; "
#endif

#ifdef ATOMIC_X86
    #define ASM_LOCK
#endif

#if defined(ATOMIC_X86_SMP) || defined(ATOMIC_X86)
    #define ATOMIC_DEFINED
    static inline void atomic_inc(mt_atomic_t *at)
    {
        __asm__ __volatile__(
            ASM_LOCK "incl %0"
            :"=m" (at->x)
            :"m" (at->x)
            :"cc"
        );
    }
    
    static inline bool atomic_dec(mt_atomic_t *at)
    {
        unsigned char c;
        __asm__ __volatile__(
            ASM_LOCK "decl %0; sete %1"
            :"=m" (at->x), "=g" (c)
            :"m" (at->x)
            :"cc"
        );
        return (c!=0);
    }
#endif

#ifdef ATOMIC_TORTURE
    #ifdef ATOMIC_DEFINED
        #error ATOMIC_X86(_SMP) and ATOMIC_TORTURE are defined at the same time!
    #else
        #define ATOMIC_DEFINED
    #endif

    // this is NOT atomic in most cases! Don't use ATOMIC_TORTURE!
    static inline void atomic_inc(mt_atomic_t *at)
    {
        at->x++;
    }
    
    static inline bool atomic_dec(mt_atomic_t *at)
    {
        return ((--at->x) == 0);
    }
#endif

#ifndef ATOMIC_DEFINED
    #include <pthread.h>
    #define ATOMIC_NEED_MUTEX
    static inline void atomic_inc(mt_atomic_t *at, pthread_mutex_t *mutex)
    {
        pthread_mutex_lock(mutex);
        at->x++;
        pthread_mutex_unlock(mutex);
    }
    static inline bool atomic_dec(mt_atomic_t *at, pthread_mutex_t *mutex)
    {
        pthread_mutex_lock(mutex);
        int newval = (--at->x);
        pthread_mutex_unlock(mutex);
        return (newval == 0);
    }
#else
    #undef ATOMIC_DEFINED
#endif

#endif // __ATOMIC_H__

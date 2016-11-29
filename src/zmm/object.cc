/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    object.cc - this file is part of MediaTomb.
    
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

/// \file object.cc

#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "memory.h"

using namespace zmm;

Object::Object()
{
    atomic_set(&_ref_count, 0);
#ifdef ATOMIC_NEED_MUTEX
    pthread_mutex_init(&mutex, NULL);
#endif
}
Object::~Object()
{
#ifdef ATOMIC_NEED_MUTEX
    pthread_mutex_destroy(&mutex);
#endif
}

void Object::retain()
{
#ifdef ATOMIC_NEED_MUTEX
    atomic_inc(&_ref_count, &mutex);
#else
    atomic_inc(&_ref_count);
#endif
}
void Object::release()
{
#ifdef ATOMIC_NEED_MUTEX
    if(atomic_dec(&_ref_count, &mutex))
#else
    if(atomic_dec(&_ref_count))
#endif
    {
        delete this;
    }
}

int Object::getRefCount()
{
    return atomic_get(&_ref_count);
}

void* Object::operator new (size_t size)
{
    return MALLOC(size);
}
void Object::operator delete (void *ptr)
{
    FREE(ptr);
}

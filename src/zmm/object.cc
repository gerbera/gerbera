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

#include <cstdio>
#include <cstdlib>

#include "util/memory.h"
#include "object.h"

using namespace zmm;

Object::Object()
{
    _ref_count.store(0);
}
Object::~Object()
{
}

void Object::retain() const
{
    _ref_count++;
}
void Object::release() const
{
    if (--_ref_count == 0) {
        delete this;
    }
}

int Object::getRefCount() const
{
    return _ref_count.load();
}

void* Object::operator new(size_t size)
{
    return MALLOC(size);
}
void Object::operator delete(void* ptr)
{
    FREE(ptr);
}

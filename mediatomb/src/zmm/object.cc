/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    object.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file object.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "memory.h"

using namespace zmm;

Object::Object()
{
	_ref_count = 0;
}
Object::~Object()
{}

void Object::retain()
{
	_ref_count++;
}
void Object::release()
{
	_ref_count--;
	if(_ref_count == 0)
	{
		delete this;
	}
}

int Object::getRefCount()
{
    return _ref_count;
}

void* Object::operator new (size_t size)
{
    return MALLOC(size);
}
void Object::operator delete (void *ptr)
{
    FREE(ptr);
}


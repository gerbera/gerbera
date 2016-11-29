/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    stringbuffer.cc - this file is part of MediaTomb.
    
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

/// \file stringbuffer.cc

#include "stringbuffer.h"

#include <string.h>
#include "memory.h"
#include "strings.h"
#include "exceptions.h"

using namespace zmm;

StringBuffer::StringBuffer()
{
    capacity = DEFAULT_STRINGBUFFER_CAPACITY;
    data = (char *)MALLOC((capacity + 1) * sizeof(char));
    len = 0;
    data[len] = 0;
}

StringBuffer::StringBuffer(int capacity)
{
    this->capacity = capacity;
    data = (char *)MALLOC((capacity + 1) * sizeof(char));
    len = 0;
    data[len] = 0;
}

StringBuffer::~StringBuffer()
{
    FREE(data);
}

StringBuffer &StringBuffer::operator<<(String other)
{
    int otherLen = other.length();
    if(otherLen > 0)
    {
        addCapacity(otherLen);
        strcpy(data + len, other.base->data);
        len += otherLen;
    }
    return *this;
}

StringBuffer &StringBuffer::operator<<(Ref<StringBuffer> other)
{
    concat(other, 0);
    return *this;
}


StringBuffer &StringBuffer::operator<<(const char *str)
{
    if(! str)
        return *this;
    int otherLen = (int)strlen(str);
    if(otherLen)
    {
        addCapacity(otherLen);
        strcpy(data + len, str);
        len += otherLen;
    }
    return *this;
}

StringBuffer &StringBuffer::operator<<(signed char *str)
{
    return operator<<((char *)str);
}

StringBuffer &StringBuffer::operator<<(char chr)
{
    addCapacity(1);
    data[len] = chr;
    len++;
    data[len] = 0;
    return *this;
}

StringBuffer &StringBuffer::operator<<(signed char chr)
{
    return operator<<((char)chr);
}

StringBuffer &StringBuffer::operator<<(int x)
{
    addCapacity(MAX_INT_STRING_LENGTH);
    char *dest = data + len;
    sprintf(dest, "%d", x);
    len += (int)strlen(dest);
    return *this;
}

StringBuffer &StringBuffer::operator<<(unsigned int x)
{
    addCapacity(MAX_INT_STRING_LENGTH);
    char *dest = data + len;
    sprintf(dest, "%u", x);
    len += (int)strlen(dest);
    return *this;
}

void StringBuffer::concat(Ref<StringBuffer> other, int offset)
{
    int otherLen = other->length();
    if(otherLen > 0)
    {
        if (offset >= otherLen)
            throw _Exception(_("illegal offset"));
        otherLen -= offset;
        addCapacity(otherLen);
        strcpy(data + len, other->c_str() + offset);
        len += otherLen;
    }
}

void StringBuffer::concat(char *str, int length)
{
    if(str && length)
    {
        addCapacity(length);
        strncpy(data + len, str, length);
        len += length;
        data[len] = 0;
    }
}

int StringBuffer::length()
{
    return len;
}

void StringBuffer::setLength(int newLength)
{
    if (newLength > len)
        ensureCapacity(newLength);
    
    this->len = newLength;
    data[len] = 0;
}

char *StringBuffer::c_str(int offset)
{
    if (offset > len || offset < 0)
        throw _Exception(_("illegal offset"));
    return data + offset;
}

String StringBuffer::toString()
{
    return String(data, len);
}

String StringBuffer::toString(int offset)
{
    if (offset > len || offset < 0)
        throw _Exception(_("illegal offset"));
    return String(data + offset, len - offset);
}

void StringBuffer::setCharAt(int index, char c)
{
    if (index > len || index < 0)
        throw _Exception(_("illegal index"));
    data[index] = c;
}

void StringBuffer::clear()
{
    len = 0;
    data[len] = 0;
}

void StringBuffer::ensureCapacity(int neededCapacity)
{
    if(neededCapacity > capacity)
    {
        int newCapacity = (int)(capacity * STRINGBUFFER_CAPACITY_INCREMENT);
        if(neededCapacity > newCapacity)
            newCapacity = neededCapacity;
        capacity = newCapacity;
        data = (char *)REALLOC(data, (capacity + 1) * sizeof(char));
    }
}

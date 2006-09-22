/*  stringbuffer.cc - this file is part of MediaTomb.
                                                                                
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "stringbuffer.h"

#include <string.h>
#include "memory.h"
#include "strings.h"

using namespace zmm;

StringBuffer::StringBuffer()
{
    capacity = DEFAULT_STRINGBUFFER_CAPACITY;
    data = (char *)MALLOC((capacity + 1) * sizeof(char));
    data[0] = 0;
    len = 0;
}

StringBuffer::StringBuffer(int capacity)
{
    this->capacity = capacity;
    data = (char *)MALLOC((capacity + 1) * sizeof(char));
    data[0] = 0;
    len = 0;
}

StringBuffer::~StringBuffer()
{
    FREE(data);
}

StringBuffer &StringBuffer::operator<<(String other)
{
    int otherLen = other.length();
    if(otherLen)
    {
        addCapacity(otherLen);
        strcpy(data + len, other.base->data);
        len += otherLen;
    }
    return *this;
}

StringBuffer &StringBuffer::operator<<(char *str)
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
StringBuffer &StringBuffer::operator<<(char chr)
{
    addCapacity(1);
    *(data + len) = chr;
    *(data + len + 1) = 0;
    len++;
    return *this;
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

int StringBuffer::length()
{
    return len;
}

void StringBuffer::setLength(int newLength)
{
    addCapacity(newLength);
    this->len = newLength;
}

char *StringBuffer::c_str()
{
    return data;
}

String StringBuffer::toString()
{
    return String(data, len);
}

void StringBuffer::clear()
{
    len = 0;
    data[0] = 0;
}

void StringBuffer::addCapacity(int increment)
{
    int neededCapacity = len + increment + 1;
    if(neededCapacity > capacity)
    {
        int newCapacity = (int)(capacity * STRINGBUFFER_CAPACITY_INCREMENT);
        if(neededCapacity > newCapacity)
            newCapacity = neededCapacity;
        capacity = newCapacity;
        data = (char *)REALLOC(data, capacity);
    }
}


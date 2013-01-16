/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    stringbuffer.h - this file is part of MediaTomb.
    
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

/// \file stringbuffer.h

#ifndef __ZMM_STRINGBUFFER_H__
#define __ZMM_STRINGBUFFER_H__

#include "strings.h"

#define DEFAULT_STRINGBUFFER_CAPACITY 16
#define STRINGBUFFER_CAPACITY_INCREMENT (1.25)

namespace zmm
{

class StringBuffer : public Object
{
protected:
    char *data;
    int capacity;
    int len;
public:
    StringBuffer();
    StringBuffer(int capacity);
    virtual ~StringBuffer();
    
    StringBuffer &operator<<(String other);
    StringBuffer &operator<<(Ref<StringBuffer> other);
    StringBuffer &operator<<(const char *str);
    StringBuffer &operator<<(signed char *str);
    StringBuffer &operator<<(signed char chr);
    StringBuffer &operator<<(char chr);
    StringBuffer &operator<<(int x);
    StringBuffer &operator<<(unsigned int x);
    void concat(Ref<StringBuffer> other, int offset = 0);
    void concat(char *str, int length);
    
    int length();
    int getCapacity() { return capacity; }
    void setLength(int newLength);
    char *c_str(int offset = 0);
    String toString();
    String toString(int offset);
    void clear();
    void ensureCapacity(int neededCapacity);
    void setCharAt(int index, char c);
    
protected:
    inline void addCapacity(int increment) { ensureCapacity(len + increment); }
};

} // namespace

#endif // __ZMM_STRINGBUFFER_H__

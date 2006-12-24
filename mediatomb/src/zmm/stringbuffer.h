/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    stringbuffer.h - this file is part of MediaTomb.
    
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
    StringBuffer &operator<<(char *str);
    StringBuffer &operator<<(char chr);
    StringBuffer &operator<<(int x);
    StringBuffer &operator<<(unsigned int x);
    void concat(Ref<StringBuffer> other, int offset = 0);
    
    int length();
    void setLength(int newLength);
    char *c_str(int offset = 0);
    String toString();
    String toString(int offset);
    void clear();
    void ensureCapacity(int neededCapacity);
    void setCharAt(int index, char c);
    
protected:
    inline void addCapacity(int increment) { ensureCapacity(len + increment + 1); }
};

} // namespace

#endif // __ZMM_STRINGBUFFER_H__

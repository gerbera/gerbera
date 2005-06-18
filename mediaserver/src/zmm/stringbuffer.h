/*  stringbuffer.h - this file is part of MediaTomb.
                                                                                
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
    StringBuffer &operator<<(char *str);
    StringBuffer &operator<<(char chr);
    StringBuffer &operator<<(int x);

    int length();
    void setLength(int newLength);
    char *c_str();
    String toString();
    void clear();

protected:
    void addCapacity(int increment);
};

} // namespace

#endif // __ZMM_STRINGBUFFER_H__

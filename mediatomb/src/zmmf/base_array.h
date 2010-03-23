/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    base_array.h - this file is part of MediaTomb.
    
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

/// \file base_array.h

#ifndef __ZMMF_BASE_ARRAY_H__
#define __ZMMF_BASE_ARRAY_H__

#include "zmm/zmm.h"
#include "memory.h"

#define DEFAULT_ARRAY_CAPACITY 16

namespace zmm
{

template <class T>
class BaseArray : public Object
{
public:
    BaseArray() : Object()
    {
        _init(DEFAULT_ARRAY_CAPACITY);
    }
    
    BaseArray(int capacity) : Object()
    {
        _init(capacity);
    }
    
    void _init(int capacity)
    {
        this->capacity = capacity;
        siz = 0;
        arr = (T *)MALLOC(capacity * sizeof(T));
    }
    
    ~BaseArray()
    {
        if (arr)
            FREE(arr);
    }
    
    void append(T el)
    {
        resize(siz+1);
        arr[siz++] = el;
    }
    
    void set(T el, int index)
    {
        arr[index] = el;
    }
    
    T get(int index)
    {
        return arr[index];
    }
    
    void remove(int index, int count=1)
    {
        if (index < 0 || index >= siz) // index beyond size
            return;
        int max = index + count; // max is the last element to remove + 1
        if (max > siz) // if remove block is beyond size, cut it
            max = siz;
        if (max <= index) // if nothing to remove
            return;
        int move = siz - max;
        if (move) // if there is anything to shift
        {
            memmove(
                (void *)(arr + index),
                (void *)(arr + index + count),
                move * sizeof(T)
            );
        }
        siz -= count;
    }
    
    void removeUnordered(int index)
    {
        if (index < 0 || index >= siz) // index beyond size
            return;
        arr[index] = arr[--siz];
    }
    
    void insert(int index, T el)
    {
        resize(siz + 1);
        memmove(
            (void *)(arr + (index + 1)),
            (void *)(arr + index),
            (siz - index) * sizeof(T)
        );
        arr[index] = el;
        siz++;
    }
    
    int size()
    {
        return siz;
    }
    
    void resize(int requiredSize)
    {
        if(requiredSize > capacity)
        {
            int newCapacity = siz + (siz / 2);
            if(requiredSize > newCapacity)
                newCapacity = requiredSize;
            capacity = newCapacity;
            arr = (T *)REALLOC(arr, capacity * sizeof(T));
        }
    }
    
    
    /*
    void optimize()
    {
        
    }
    
    Object **getObjectArray()
    {
        return base.arr;
    }
    */
    
protected:
    T* arr;
    int siz;
    int capacity;
};

class IntArray : public BaseArray<int>
{
public:
    String toCSV(char sep = ',')
    {
        Ref<StringBuffer> buf(new StringBuffer());
        for (int i = 0; i < siz; i++)
            *buf << sep << get(i);
        if (buf->length() <= 0)
            return _("");
        return buf->toString(1);
    }
    
    void addCSV(String csv, char sep = ',')
    {
        char *data = csv.c_str();
        char *dataEnd = data + csv.length();
        while (data < dataEnd)
        {
            char *endptr;
            int val = (int)strtol(data, &endptr, 10);
            if (endptr == data)
                throw _Exception(_("illegal csv given to IntArray"));
            append(val);
            if (endptr >= dataEnd)
                break;
            if (*endptr == sep)
                data = endptr + 1;
            else
                throw _Exception(_("illegal csv given to IntArray"));
        }
    }
};

} // namespace

#endif // __ZMMF_BASE_ARRAY_H__

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    array.h - this file is part of MediaTomb.
    
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

/// \file array.h

#ifndef __ZMMF_ARRAY_H__
#define __ZMMF_ARRAY_H__

#include "zmm/zmm.h"

#define DEFAULT_ARRAY_CAPACITY 16

namespace zmm
{

class ArrayBase
{
public:
    ArrayBase();
    ~ArrayBase();
    void init(int capacity);
    void append(Object *el);
    void set(Object *el, int index);
    Object *get(int index);
    void remove(int index, int count);
    void removeUnordered(int index);
    void clear();
    void insert(int index, Object *el);
    inline int size() { return siz; }
    void optimize();
protected:
    void resize(int requiredSize);
public:
    Object **arr;
protected:
    int siz;
    int capacity;
};


template <class T>
class Array : public Object
{
protected:
    ArrayBase base;

public:
    inline Array() : Object()
    {
        base.init(DEFAULT_ARRAY_CAPACITY);
    }
    inline Array(int capacity) : Object()
    {
        base.init(capacity);
    }

    inline void append(Ref<T> el)
    {
        base.append(el.getPtr());
    }
    inline void set(Ref<T> el, int index)
    {
        base.set(el.getPtr(), index);
    }
    inline Ref<T> get(int index)
    {
        return Ref<T>( (T *)base.get(index) );
    }
    inline void remove(int index, int count=1)
    {
        base.remove(index, count);
    }
    inline void removeUnordered(int index)
    {
        base.removeUnordered(index);
    }
    inline void insert(int index, Ref<T> el)
    {
        base.insert(index, el.getPtr());
    }
    inline int size()
    {
        return base.size();
    }
    inline void clear()
    {
        return base.clear();
    }
    inline void optimize()
    {
        base.optimize();
    }
    
    inline Object **getObjectArray()
    {
        return base.arr;
    }
};


} // namespace

#endif // __ZMMF_ARRAY_H__

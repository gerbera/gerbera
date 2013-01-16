/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    reentrant_array.h - this file is part of MediaTomb.
    
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

/// \file reentrant_array.h
/// \brief Definiton of the ReentrantArray class.
#ifndef __REENTRANT_ARRAY_H__
#define __REENTRANT_ARRAY_H__

#include "zmmf/zmmf.h"
#include "sync.h"

/// \brief Reentrant version of the object array
template <class T>
class ReentrantArray : public zmm::Array<T>
{
public:
    ReentrantArray() : zmm::Array<T>()
    {
        mutex = zmm::Ref<Mutex>(new Mutex());
    }
    ReentrantArray(int capacity) : zmm::Array<T>(capacity)
    {
        mutex = zmm::Ref<Mutex>(new Mutex());
    }
    inline void append(zmm::Ref<T> el)
    {
        mutex->lock();
        zmm::Array<T>::append(el);
        mutex->unlock();
    }
    inline void set(zmm::Ref<T> el, int index)
    {
        mutex->lock();
        zmm::Array<T>::set(el, index);
        mutex->unlock();
    }
    inline zmm::Ref<T> get(int index)
    {
        zmm::Ref<T> ret;
        mutex->lock();
        ret = zmm::Array<T>::get(index);
        mutex->unlock();
        return ret;
    }
    inline void remove(int index, int count=1)
    {
        mutex->lock();
        zmm::Array<T>::set(index, count);
        mutex->unlock();
    }
    inline void removeUnordered(int index)
    {
        mutex->lock();
        zmm::Array<T>::removeUnordered(index);
        mutex->unlock();
    }
    inline void insert(int index, zmm::Ref<T> el)
    {
        mutex->lock();
        zmm::Array<T>::insert(index, el);
        mutex->unlock();
    }
    inline int size()
    {
        int size;
        mutex->lock();
        size = zmm::Array<T>::size();
        mutex->unlock();
        return size;
    }
    inline void clear()
    {
        mutex->lock();
        zmm::Array<T>::clear();
        mutex->unlock();
    }
    inline void optimize()
    {
        mutex->lock();
        zmm::Array<T>::optimize();
        mutex->unlock();
    }
protected:
    zmm::Ref<Mutex> mutex;
};


#endif // __REENTRANT_ARRAY_H__

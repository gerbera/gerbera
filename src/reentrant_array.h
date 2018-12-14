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

#include "zmm/zmmf.h"
#include <mutex>

/// \brief Reentrant version of the object array
template <class T>
class ReentrantArray : public zmm::Array<T> {
public:
    ReentrantArray()
        : zmm::Array<T>()
    {
    }
    ReentrantArray(int capacity)
        : zmm::Array<T>(capacity)
    {
    }
    inline void append(zmm::Ref<T> el)
    {
        AutoLock lock(mutex);
        zmm::Array<T>::append(el);
    }
    inline void set(zmm::Ref<T> el, int index)
    {
        AutoLock lock(mutex);
        zmm::Array<T>::set(el, index);
    }
    inline zmm::Ref<T> get(int index)
    {
        AutoLock lock(mutex);
        return zmm::Array<T>::get(index);
    }
    inline void remove(int index, int count = 1)
    {
        AutoLock lock(mutex);
        zmm::Array<T>::set(index, count);
    }
    inline void removeUnordered(int index)
    {
        AutoLock lock(mutex);
        zmm::Array<T>::removeUnordered(index);
    }
    inline void insert(int index, zmm::Ref<T> el)
    {
        AutoLock lock(mutex);
        zmm::Array<T>::insert(index, el);
    }
    inline int size()
    {
        AutoLock lock(mutex);
        return zmm::Array<T>::size();
    }
    inline void clear()
    {
        AutoLock lock(mutex);
        zmm::Array<T>::clear();
    }
    inline void optimize()
    {
        AutoLock lock(mutex);
        zmm::Array<T>::optimize();
    }

protected:
    std::mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
};

#endif // __REENTRANT_ARRAY_H__

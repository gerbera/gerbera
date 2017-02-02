/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    ref.h - this file is part of MediaTomb.
    
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

/// \file ref.h

#ifndef __ZMM_REF_H__
#define __ZMM_REF_H__

namespace zmm
{

template <class T>
class Ref
{
public:
    Ref(const Ref& other)
    {
        _ptr = other._ptr;
        if(_ptr)
            _ptr->retain();
    }
    explicit Ref(T* ptr = nullptr) : _ptr(ptr)
    {
        if(ptr)
            ptr->retain();
    }
    Ref(std::nullptr_t)
    {
        _ptr = nullptr;
    }
    ~Ref()
    {
        if(_ptr)
            _ptr->release();
    }

    Ref& operator=(const Ref& other)
    {
        if (this == &other)
            return *this;
        if(_ptr)
            _ptr->release();
        _ptr = other._ptr;
        if(_ptr)
            _ptr->retain();
        return *this;
    }
    inline Ref& operator=(std::nullptr_t)
    {
        if(_ptr)
            _ptr->release();
        _ptr = nullptr;
        return *this;
    }

    inline T& operator*() const
    {
        return *_ptr;
    }
    inline T* operator->() const
    {
        return _ptr;
    }
    inline T* getPtr()
    {
        return _ptr;
    }
    inline int operator==(std::nullptr_t) const
    {
        return (_ptr == nullptr);
    }
    inline int operator!=(std::nullptr_t) const
    {
        return (_ptr != nullptr);
    }
    inline int operator==(const Ref& other) const
    {
        return (_ptr == other._ptr);
    }
    inline int operator!=(const Ref& other) const
    {
        return (_ptr != other._ptr);
    }
protected:
    T* _ptr;
};

} // namespace

#define RefCast(ref, klass) zmm::Ref< klass >(( klass *)ref.getPtr())

#endif // __ZMM_REF_H__

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    object_stack.h - this file is part of MediaTomb.
    
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

/// \file object_stack.h

#ifndef __ZMMF_OBJECT_STACK_H__
#define __ZMMF_OBJECT_STACK_H__

#include "zmm/zmm.h"
#include "memory.h"
#include "base_stack.h"

namespace zmm
{
    template <class T>
    class ObjectStack : public BaseStack<Object *>
    {
    public:
        ObjectStack(int initialCapacity) : BaseStack<Object *>(initialCapacity, NULL)
        {
        }
        
        ~ObjectStack()
        {
            Object *obj;
            while(NULL != (obj = BaseStack<Object *>::pop()))
            {
                log_debug("releasing!\n");
                obj->release();
            }
        }
        
        inline void push(Ref<T> element)
        {
            Object *obj = element.getPtr();
            obj->retain();
            BaseStack<Object *>::push(obj);
        }
        
        inline Ref<T> pop()
        {
            Object *obj = BaseStack<Object *>::pop();
            if (obj == NULL)
                return nil;
            Ref<T> ret((T *)obj);
            obj->release();
            return ret;
        }
    };
}

#endif // __ZMMF_OBJECT_STACK_H__

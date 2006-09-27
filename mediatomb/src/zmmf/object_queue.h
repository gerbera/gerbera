/*  object_queue.h - this file is part of MediaTomb.
                                                                                
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

#ifndef __ZMMF_OBJECT_QUEUE_H__
#define __ZMMF_OBJECT_QUEUE_H__

#include "zmm/zmm.h"

namespace zmm
{
    /// \brief a simple stack for a base type. NOT thread safe!
    template <class T>
    class ObjectQueue : public Object
    {
    public:
        inline ObjectQueue(int initialCapacity) : Object()
        {
            capacity = initialCapacity;
            queueEnd = 0;
            queueBegin = 0;
            overlap = false;
            data = (Object **)MALLOC(capacity * sizeof(Object *));
        }
        
        inline ~ObjectQueue()
        {
            FREE(this->data);
        }
        
        inline int getCapacity()
        {
            return capacity;
        }
        
        inline int size()
        {
            if (overlap) return capacity;
            if (queueBegin < queueEnd)
                return queueEnd - queueBegin;
            return capacity - queueEnd + queueBegin;
        }
        
        inline bool isEmpty()
        {
            return (! overlap && queueEnd == queueBegin);
        }
        
        void resize(int requiredSize)
        {
            if(requiredSize > capacity)
            {
                int count = size();
                int oldCapacity = capacity;
                capacity = count + (count / 2);
                if(requiredSize > capacity)
                    capacity = requiredSize;
                data = (Object **)REALLOC(data, capacity * sizeof(Object *));
                log_debug("resizing %d -> %d\n", oldCapacity, capacity);
                if ((overlap && (queueEnd != 0)) || queueBegin > queueEnd)
                {
                    int moveAmount = oldCapacity - queueBegin;
                    int newQueueBegin = capacity - moveAmount;
                    memmove(data + newQueueBegin, data + queueBegin, moveAmount * sizeof(T));
                    queueBegin = newQueueBegin;
                }
                else if (queueEnd == 0)
                    queueEnd = oldCapacity;
                overlap = false;
            }
        }
        
        inline void enqueue(Ref<T> element)
        {
            if (overlap)
                resize(size() + 1);
            Object *obj = element.getPtr();
            obj->retain();
            data[queueEnd++] = obj;
            if (queueEnd == capacity)
                queueEnd = 0;
            if (queueEnd == queueBegin)
                overlap = true;
        }
        
        inline Ref<T> dequeue()
        {
            if (isEmpty())
                return nil;
            Object *obj = data[queueBegin++];
            Ref<T> ret((T *)obj);
            ret->release();
            overlap = false;
            if (queueBegin == capacity)
                queueBegin = 0;
            return ret;
        }
        
    protected:
        Object **data;
        int capacity;
        int queueBegin;
        int queueEnd;
        bool overlap;
    };
}

#endif // __ZMMF_OBJECT_QUEUE_H__


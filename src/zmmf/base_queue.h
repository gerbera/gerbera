/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    base_queue.h - this file is part of MediaTomb.
    
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

/// \file base_queue.h

#ifndef __ZMMF_BASE_QUEUE_H__
#define __ZMMF_BASE_QUEUE_H__

#include "zmm/zmm.h"

namespace zmm
{
    /// \brief a simple stack for a base type. NOT thread safe!
    template <typename T>
    class BaseQueue : public Object
    {
    public:
        BaseQueue(int initialCapacity, T emptyType) : Object()
        {
            capacity = initialCapacity;
            this->emptyType = emptyType;
            queueEnd = 0;
            queueBegin = 0;
            overlap = false;
            data = (T *)MALLOC(capacity * sizeof(T));
        }
        
        ~BaseQueue()
        {
            FREE(this->data);
        }
        
        inline int getCapacity()
        {
            return capacity;
        }
        
        int size()
        {
            if (overlap) return capacity;
            if (queueBegin <= queueEnd)
                return queueEnd - queueBegin;
            return capacity - queueBegin + queueEnd;
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
                data = (T *)REALLOC(data, capacity * sizeof(T));
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
        
        inline void enqueue(T element)
        {
            if (overlap)
                resize(size() + 1);
            data[queueEnd++] = element;
            if (queueEnd == capacity)
                queueEnd = 0;
            if (queueEnd == queueBegin)
                overlap = true;
        }
        
        inline T dequeue()
        {
            if (isEmpty())
                return emptyType;
            T ret = data[queueBegin++];
            overlap = false;
            if (queueBegin == capacity)
                queueBegin = 0;
            return ret;
        }
        
        T get(int index)
        {
            return data[ (queueBegin + index) % capacity ];
        }
        
    protected:
        T *data;
        T emptyType;
        int capacity;
        int queueBegin;
        int queueEnd;
        bool overlap;
    };
}

#endif // __ZMMF_BASE_QUEUE_H__

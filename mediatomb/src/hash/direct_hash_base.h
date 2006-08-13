/*  ref.h - this file is part of MediaTomb.
                                                                                
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

#ifndef __HASH_DIRECT_HASH_BASE_H__
#define __HASH_DIRECT_HASH_BASE_H__

#include "hash.h"

template <typename KT, typename ST>
class DHashBase : public zmm::Object
{
protected:
    int capacity;
    int count;
    ST *data;
public:
    DHashBase(int capacity) : zmm::Object()
    {
        this->capacity = capacity;
        count = 0;
        data = (ST *)MALLOC(capacity * sizeof(ST));
    }
    virtual ~DHashBase()
    {
        FREE(data);
    }
    void zero()
    {
        count = 0;
        memset(data, 0, capacity * sizeof(ST));
    }
    inline int size()
    {
        return count;
    }

    /* abstract methods to be implemented in deriving classes */
    virtual int hashCode(KT key) = 0;
    virtual bool match(KT key, ST *slot) = 0;
    virtual bool isEmptySlot(ST *slot) = 0;

    inline int baseTypeHashCode(unsigned int key)
    {
        return ((key * 0xd2d84a61) ^ 0x7832c9f4) % capacity;
    }
    int stringHashCode(zmm::String key)
    {
        int i;
            
        int len = key.length();
        int iLen = len >> 2;
        int iRest = len & 3;
        unsigned int *iData = (unsigned int *)key.c_str();
        unsigned int sum = 0;
        
        for(i = 0; i < iLen; i++)
            sum ^= iData[i];
        if (iRest)
        {
            unsigned char *ptr = (unsigned char *)(iData + i);
            unsigned int restSum = 0;
            while (*ptr)
                restSum = (restSum << 8) | *(ptr++);
            sum ^= restSum;
        }
        return ((sum * 0xd2d84a61) ^ 0x7832c9f4) % capacity;
    }

    int secondaryHashCode(int primary)
    {
        int prime = HASH_PRIME;
        int h2 = (prime - (primary % prime));
        if (h2 == 0)
            h2 = prime;
        return h2;
    }
   
    // if found an empty slot, retSlot is set and false is returned
    // if found a slot with the given key retSlot is set and true is returned
    bool search(KT key, ST **retSlot)
    {
        // calculating primary hash
        int h = hashCode(key);
        // secondary hash
        int h2 = 0;
        for (int i = 0; i < HASH_MAX_ITERATIONS; i++)
        {
            ST *slot = data + h;
            
            // found an empty slot
            if (isEmptySlot(slot))
            {
                *retSlot = slot;
                return false;
            }

            // found the key
            if (match(key, slot))
            {
                *retSlot = slot;
                return true;
            }

            // collision, probing next slot
            if (! h2)
                h2 = secondaryHashCode(h);
            h = (h + h2) % capacity;
        }
        log_error("AbstractHash::search() failed, maximal number of iterations exceeded: h:%d  h2:%d\n", h, h2);
        throw zmm::Exception(_("AbstractHash::search() failed, maximal number of iterations exceeded: h:") + h + " h2:"+ h2);
    }
};

#endif // __HASH_DIRECT_HASH_BASE_H__


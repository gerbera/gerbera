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

#ifndef __HASH_DB_HASH_H__
#define __HASH_DB_HASH_H__

#include "hash.h"

template <typename T>
class DBHash : public zmm::Object
{
protected:
    int capacity;
    int count;
    T *data;
    T emptyKey;
public:
    DBHash(int capacity, T emptyKey) : zmm::Object()
    {
        this->capacity = capacity;
        this->emptyKey = emptyKey;
        count = 0;
        data = (T *)calloc(capacity, sizeof(T));
    }
    virtual ~DBHash()
    {
        free(data);
    }
    void zero()
    {
        count = 0;
        memset(data, 0, capacity * sizeof(T));
    }
    inline int size()
    {
        return count;
    }
    T hashCode(T key)
    {
        return (((unsigned int)key * 0xd2d84a61) ^ 0x7832c9f4) % capacity;
    }
    
    bool put(T key)
    {
        int prime = HASH_PRIME;
        
        // calculating primary hash
        int h = hashCode(key);
        // calculating secondary hash
        int h2 = (prime - ((int)key % prime)) % capacity;
        if (h2 == 0)
            h2 = prime;
        for (int i = 0; i < HASH_MAX_ITERATIONS; i++)
        {
            T *slot = data + h;
            
            // found an empty slot
            if (*slot == emptyKey)
            {
                count++;
                // log_debug(("hash::put: h:%d  h2:%d  count:%d  i:%d\n", h, h2, count, i));
                *slot = key;
                return true;
            }
            // if found the same key do nothing
            if (*slot == key)
                return false;

            // collision, probing next slot
            h = (h + h2) % capacity;
        }
        throw zmm::Exception(zmm::String("DBHash::put() failed, maximal number of iterations exceeded: h:") + h + " h2:"+ h2);
    }

    bool exists(T key)
    {
        int prime = HASH_PRIME;
        
        // calculating primary hash
        int h = hashCode(key);
        // calculating secondary hash
        int h2 = (prime - ((int)key % prime)) % capacity;
        if (h2 == 0)
            h2 = prime;
        for (int i = 0; i < HASH_MAX_ITERATIONS; i++)
        {
            T *slot = data + h;
            
            // found an empty slot
            if (*slot == emptyKey)
                return false;
            
            // found the key
            if (*slot == key)
                return true;

            // collision, probing next slot
            h = (h + h2) % capacity;
        }
        throw zmm::Exception(zmm::String("DBHash::exists() failed, maximal number of iterations exceeded: h:") + h + " h2:"+ h2);
    }
};

#endif // __HASH_DB_HASH_H__


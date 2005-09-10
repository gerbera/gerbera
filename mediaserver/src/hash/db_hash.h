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

#include "direct_hash_base.h"

template <typename KT>
class DBHash : public DHashBase<KT, KT>
{
protected:
    KT emptyKey;
public:
    DBHash(int capacity, KT emptyKey) : DHashBase<KT, KT>(capacity)
    {
        this->emptyKey = emptyKey;
        clear();
    }

    /* virtual methods */
    virtual int hashCode(KT key)
    {
        return this->baseTypeHashCode((unsigned int)key);
    }
    virtual bool match(KT key, KT *slot)
    {
       return (key == *slot);
    }
    virtual bool isEmptySlot(KT *slot)
    {
        return (*slot == emptyKey);
    }
   
    void clear()
    {
        if (! emptyKey)
            this->zero();
        else
        {
            for (int i = 0; i < this->capacity; i++)
                this->data[i] = emptyKey;
            this->count = 0;
        }
    }

    inline void put(KT key)
    {
        KT *slot;
        if (! search(key, &slot))
        {
            *slot = key;
            this->count++;
        }
    }
    inline void put(KT key, hash_slot_t destSlot)
    {
        KT *slot = (KT *)destSlot;
        if (*slot == emptyKey)
        {
            *slot = key;
            this->count++;
        }
    }

    inline bool exists(KT key)
    {
        KT *slot;
        return search(key, &slot);
    }
    inline bool exists(KT key, hash_slot_t *destSlot)
    {
        return search(key, (KT **)destSlot);
    }
};

#endif // __HASH_DS_HASH_H__


/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    db_hash.h - this file is part of MediaTomb.
    
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

/// \file db_hash.h

#ifndef __HASH_DB_HASH_H__
#define __HASH_DB_HASH_H__

#include "direct_hash_base.h"

/// \brief Direct hash with base type keys only.
template <typename KT>
class DBHash : public DHashBase<KT, KT>
{
protected:
    KT emptyKey;
    KT deletedKey;
public:
    DBHash(int capacity, KT emptyKey, KT deletedKey) : DHashBase<KT, KT>(capacity)
    {
        // emptyKey and deletedKey must not be the same!
        assert(emptyKey != deletedKey);
        this->emptyKey = emptyKey;
        this->deletedKey = deletedKey;
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
    
    virtual bool isDeletedSlot(KT *slot)
    {
        return (*slot == deletedKey);
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
    
    inline bool remove(KT key)
    {
       KT *slot;
        if (! search(key, &slot))
            return false;
        *slot = deletedKey;
        this->count--;
        return true;
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

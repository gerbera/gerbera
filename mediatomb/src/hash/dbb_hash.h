/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dbb_hash.h - this file is part of MediaTomb.
    
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

/// \file dbb_hash.h

#ifndef __HASH_DBB_HASH_H__
#define __HASH_DBB_HASH_H__

#include "direct_hash_base.h"

template <typename KT, typename VT> struct dbb_hash_slot
{
    KT key;
    VT value;
};

/// \brief Direct hash with base type keys and base type values.
template <typename KT, typename VT>
class DBBHash : public DHashBase<KT, struct dbb_hash_slot<KT, VT> >
{
protected:
    KT emptyKey;
public:
    DBBHash(int capacity, KT emptyKey) : DHashBase<KT, struct dbb_hash_slot<KT, VT> >(capacity)
    {
        this->emptyKey = emptyKey;
        clear();
    }
    
    /* virtual methods */
    virtual int hashCode(KT key)
    {
        return this->baseTypeHashCode((unsigned int)key);
    }
    virtual bool match(KT key, struct dbb_hash_slot<KT, VT> *slot)
    {
        return (key == slot->key);
    }
    virtual bool isEmptySlot(struct dbb_hash_slot<KT, VT> *slot)
    {
        return (slot->key == emptyKey);
    }

    void clear()
    {
        if (! emptyKey)
            this->zero();
        else
        {
            for (int i = 0; i < this->capacity; i++)
                this->data[i].key = emptyKey;
            this->count = 0;
        }
    }
    
    /* need to use deleted key
    inline bool remove(KT key)
    {
        struct dbb_hash_slot<KT, VT> *slot;
        if (! search(key, &slot))
            return false;
        slot->key = emptyKey;
        this->count--;
        return true;
    }
    */
    
    inline void put(KT key, VT value)
    {
        struct dbb_hash_slot<KT, VT> *slot;
        bool found = search(key, &slot);
        if (! found)
        {
            slot->key = key;
            this->count++;
        }
        slot->value = value;
    }
    inline void put(KT key, hash_slot_t destSlot, VT value)
    {
        struct dbb_hash_slot<KT, VT> *slot = (struct dbb_hash_slot<KT, VT> *)destSlot;
        if (slot->key == emptyKey)
        {
            slot->key = key;
            this->count++;
        }
        slot->value = value;
    }

    inline bool get(KT key, VT *value)
    {
        struct dbb_hash_slot<KT, VT> *slot;
        bool found = search(key, &slot);
        if (found)
            *value = slot->value;
        return found;
    }
    bool get(KT key, hash_slot_t *destSlot, VT *value)
    {
        struct dbb_hash_slot<KT, VT> **slot = (struct dbb_hash_slot<KT, VT> **)destSlot;
        bool found = search(key, slot);
        if (found)
            *value = (*slot)->value;
        return found;
    }
    inline bool exists(KT key)
    {
        struct dbb_hash_slot<KT, VT> *slot;
        return search(key, &slot);
    }
    inline bool exists(KT key, hash_slot_t *destSlot)
    {
        return search(key, (struct dbb_hash_slot<KT, VT> **)destSlot);
    }
};

#endif // __HASH_DBB_HASH_H__

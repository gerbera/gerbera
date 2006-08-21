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

#ifndef __HASH_DBO_HASH_H__
#define __HASH_DBO_HASH_H__

#include "direct_hash_base.h"

template <typename KT, typename VT> struct dbo_hash_slot
{
    KT key;
    VT *value;
};

/// \brief direct hash with binary keys and object ("Ref") values
template <typename KT, typename VT>
class DBOHash : public DHashBase<KT, struct dbo_hash_slot<KT, VT> >
{
protected:
    KT emptyKey;
public:
    DBOHash(int capacity, KT emptyKey) : DHashBase<KT, struct dbo_hash_slot<KT, VT> >(capacity)
    {
        this->emptyKey = emptyKey;
        init();
    }
    virtual ~DBOHash()
    {
        releaseData();
    }
protected:
    void init()
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
    void releaseData()
    {
        dbo_hash_slot<KT, VT> *slot;
        for (int i = 0; i < this->capacity; i++)
        {
            slot = this->data + i;
            if (slot->key != emptyKey)
                slot->value->release();
        }
    }
public:
    void clear()
    {
        dbo_hash_slot<KT, VT> *slot;
        for (int i = 0; i < this->capacity; i++)
        {
            slot = this->data + i;
            if (slot->key != emptyKey)
            {
                slot->key = emptyKey;
                slot->value->release();
            }
        }
        this->count = 0;
    }

    /* virtual methods */
    virtual int hashCode(KT key)
    {
        return this->baseTypeHashCode((unsigned int)key);
    }
    virtual bool match(KT key, struct dbo_hash_slot<KT, VT> *slot)
    {
        return (key == slot->key);
    }
    virtual bool isEmptySlot(struct dbo_hash_slot<KT, VT> *slot)
    {
        return (slot->key == emptyKey);
    }

    inline void put(KT key, zmm::Ref<VT> value)
    {
        struct dbo_hash_slot<KT, VT> *slot;
        search(key, &slot);
        put(key, (hash_slot_t)slot, value);
    }
    void put(KT key, hash_slot_t destSlot, zmm::Ref<VT> value)
    {
        struct dbo_hash_slot<KT, VT> *slot = (struct dbo_hash_slot<KT, VT> *)destSlot;
        if (slot->key != emptyKey)
        {
            VT *valuePtr = value.getPtr();
            valuePtr->retain();
            slot->value->release();
            slot->value = valuePtr;
        }            
        else
        {
            this->count++;
            slot->key = key;
            VT *valuePtr = value.getPtr();
            valuePtr->retain();
            slot->value = valuePtr;
        }    
    }

    inline zmm::Ref<VT> get(KT key)
    {
        struct dbo_hash_slot<KT, VT> *slot;
        bool found = search(key, &slot);
        if (found)
            return zmm::Ref<VT>(slot->value);
        else
            return nil;
        
        hash_slot_t destSlot;
        return get(key, &destSlot);
    }
    inline zmm::Ref<VT> get(KT key, hash_slot_t *destSlot)
    {
        struct dbo_hash_slot<KT, VT> **slot = (struct dbo_hash_slot<KT, VT> **)destSlot;
        bool found = search(key, slot);
        if (found)
            return zmm::Ref<VT>((*slot)->value);
        else
            return nil;
    }
};

#endif // __HASH_DBO_HASH_H__


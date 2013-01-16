/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dsb_hash.h - this file is part of MediaTomb.
    
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

/// \file dsb_hash.h

#ifndef __HASH_DSB_HASH_H__
#define __HASH_DSB_HASH_H__

#include "direct_hash_base.h"
#include "tools.h"

template <typename VT> struct dsb_hash_slot
{
    zmm::StringBase *key;
    VT *value;
};


/// \brief Direct hash with string keys and base type values.
template <typename VT>
class DSBHash : public DHashBase<zmm::String, struct dsb_hash_slot<VT> >
{
public:
    DSBHash(int capacity) : DHashBase<zmm::String, struct dsb_hash_slot<VT> >(capacity)
    {
        this->zero();
    }
    virtual ~DSBHash()
    {
        releaseData();
    }
    void releaseData()
    {
        for (int i = 0; i < this->capacity; i++)
        {
            dsb_hash_slot<VT> *slot = this->data + i;
            if (slot->key)
                slot->key->release();
        }
    }
    
    /* virtual methods */
    virtual int hashCode(zmm::String key)
    {
        return stringHash(key);
    }
    virtual bool match(zmm::String key, struct dsb_hash_slot<VT> *slot)
    {
       return (! strcmp(key.c_str(), slot->key->data));
    }
    virtual bool isEmptySlot(struct dsb_hash_slot<VT> *slot)
    {
        return (slot->key == NULL);
    }
    
    void clear()
    {
        releaseData();
        this->zero();
    }
    
    /* #error need to use deleted key
    inline bool remove(zmm::String key)
    {
        struct dsb_hash_slot<VT> *slot;
        if (! search(key, &slot))
            return false;
        slot->key->release();
        slot->key = NULL;
        this->count--;
        return true;
    }
    */
    
    inline void put(zmm::String key, VT value)
    {
        struct dsb_hash_slot<VT> *slot;
        search(key, &slot);
        put(key, (hash_slot_t)slot, value);
    }
    void put(zmm::String key, hash_slot_t destSlot, VT value)
    {
        struct dsb_hash_slot<VT> *slot = (struct dsb_hash_slot<VT> *)destSlot;
        if (slot->key != NULL)
            slot->value = value;
        else
        {
            this->count++;
            zmm::StringBase *keyBase = key.getBase();
            keyBase->retain();
            slot->key = keyBase;
            slot->value = value;
        }    
    }

    inline VT get(zmm::String key)
    {
        struct dsb_hash_slot<VT> *slot;
        bool found = search(key, &slot);
        if (found)
            return slot->value;
        else
            return nil;
        
        hash_slot_t destSlot;
        return get(key, destSlot);
    }
    inline VT get(zmm::String key, hash_slot_t *destSlot)
    {
        struct dsb_hash_slot<VT> **slot = (struct dsb_hash_slot<VT> **)destSlot;
        bool found = search(key, slot);
        if (found)
            return (*slot)->value;
        else
            return nil;
    }
};

#endif // __HASH_DSB_HASH_H__

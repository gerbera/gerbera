/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dso_hash.h - this file is part of MediaTomb.
    
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

/// \file dso_hash.h

#ifndef __HASH_DSO_HASH_H__
#define __HASH_DSO_HASH_H__

#include "direct_hash_base.h"
#include "tools.h"

template <typename VT> struct dso_hash_slot
{
    zmm::StringBase *key;
    VT *value;
};

/// \brief direct hash with string keys and object ("Ref") values
template <typename VT>
class DSOHash : public DHashBase<zmm::String, struct dso_hash_slot<VT> >
{
public:
    DSOHash(int capacity) : DHashBase<zmm::String, struct dso_hash_slot<VT> >(capacity)
    {
        this->zero();
        deletedKey = new zmm::StringBase("");
    }
    virtual ~DSOHash()
    {
        releaseData();
        deletedKey->release();
    }
protected:
    
    zmm::StringBase *deletedKey;
    
    void releaseData()
    {
        for (int i = 0; i < this->capacity; i++)
        {
            dso_hash_slot<VT> *slot = this->data + i;
            if (slot->key && slot->key != deletedKey)
            {
                slot->key->release();
                slot->value->release();
            }
        }
    }
public:
    /* virtual methods */
    virtual int hashCode(zmm::String key)
    {
        return this->baseTypeHashCode(stringHash(key));
    }
    virtual bool match(zmm::String key, struct dso_hash_slot<VT> *slot)
    {
       return (! strcmp(key.c_str(), slot->key->data));
    }
    virtual bool isEmptySlot(struct dso_hash_slot<VT> *slot)
    {
        return (slot->key == NULL);
    }
    virtual bool isDeletedSlot(struct dso_hash_slot<VT> *slot)
    {
        return (slot->key == deletedKey);
    }
    
    void clear()
    {
        releaseData();
        this->zero();
    }
    
    inline bool remove(zmm::String key)
    {
        struct dso_hash_slot<VT> *slot;
        if (! this->search(key, &slot))
            return false;
        slot->key->release();
        slot->value->release();
        slot->key = deletedKey;
        this->count--;
        return true;
    }
    
    inline void put(zmm::String key, zmm::Ref<VT> value)
    {
        struct dso_hash_slot<VT> *slot;
        this->search(key, &slot);
        put(key, (hash_slot_t)slot, value);
    }
    void put(zmm::String key, hash_slot_t destSlot, zmm::Ref<VT> value)
    {
        struct dso_hash_slot<VT> *slot = (struct dso_hash_slot<VT> *)destSlot;
        if (slot->key != NULL && slot->key != deletedKey)
        {
            VT *valuePtr = value.getPtr();
            valuePtr->retain();
            slot->value->release();
            slot->value = valuePtr;
        }            
        else
        {
            this->count++;
            zmm::StringBase *keyBase = key.getBase();
            keyBase->retain();
            slot->key = keyBase;
                
            VT *valuePtr = value.getPtr();
            valuePtr->retain();
            slot->value = valuePtr;
        }    
    }

    inline zmm::Ref<VT> get(zmm::String key)
    {
        struct dso_hash_slot<VT> *slot;
        bool found = this->search(key, &slot);
        if (found)
            return zmm::Ref<VT>(slot->value);
        else
            return nil;
        
        hash_slot_t destSlot;
        return get(key, &destSlot);
    }
    inline zmm::Ref<VT> get(zmm::String key, hash_slot_t *destSlot)
    {
        struct dso_hash_slot<VT> **slot = (struct dso_hash_slot<VT> **)destSlot;
        bool found = this->search(key, slot);
        if (found)
            return zmm::Ref<VT>((*slot)->value);
        else
            return nil;
    }
};

#endif // __HASH_DSO_HASH_H__

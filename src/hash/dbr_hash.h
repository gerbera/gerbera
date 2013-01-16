/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dbr_hash.h - this file is part of MediaTomb.
    
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

/// \file dbr_hash.h

#ifndef __HASH_DBR_HASH_H__
#define __HASH_DBR_HASH_H__

#include "direct_hash_base.h"

template <typename KT> struct dbr_hash_slot
{
    KT key;
    int array_slot;
};

template <typename KT> struct hash_data_array_t
{
    int size;
    KT *data;
};

/// \brief Direct hash with base type keys only. It is NOT thread-safe!
/// It has remove() and getAll() functions
template <typename KT>
class DBRHash : public DHashBase<KT, struct dbr_hash_slot<KT> >
{
protected:
    KT emptyKey;
    KT deletedKey;
    KT *data_array;
#ifdef TOMBDEBUG
    int realCapacity;
#endif
public:
    /// \param hashCapacity the size of the hashtable
    /// \param realCapacityKT maximum number of elements "you" will store in this instance (has to be < than hashCapacity!)
    /// WARNING: the number of stored entries MUST be <= realCapacity at all times!
    DBRHash(int hashCapacity, int realCapacity, KT emptyKey, KT deletedKey) : DHashBase<KT, struct dbr_hash_slot<KT> >(hashCapacity)
    {
#ifdef TOMBDEBUG
        if (realCapacity <=0)
            throw zmm::Exception(_("illegal realCapacity (") + realCapacity + ")");
        this->realCapacity = realCapacity;
        if (realCapacity >= hashCapacity)
            throw zmm::Exception(_("realCapacity (") + realCapacity + ") MUST be < hashCapacity (" + hashCapacity + ")");
#endif
        if (emptyKey == deletedKey)
            throw zmm::Exception(_("emptyKey and deletedKey must not be the same!"));
        this->emptyKey = emptyKey;
        this->deletedKey = deletedKey;
        data_array = (KT *)MALLOC(realCapacity * sizeof(KT));
        clear();
    }
    
    virtual ~DBRHash()
    {
        FREE(data_array);
    }
    
    /* virtual methods */
    virtual int hashCode(KT key)
    {
        return this->baseTypeHashCode((unsigned int)key);
    }
    
    virtual bool match(KT key, struct dbr_hash_slot<KT> *slot)
    {
       return (key == slot->key);
    }
    
    virtual bool isEmptySlot(struct dbr_hash_slot<KT> *slot)
    {
        return (slot->key == emptyKey);
    }
    
    virtual bool isDeletedSlot(struct dbr_hash_slot<KT> *slot)
    {
        return (slot->key == deletedKey);
    }
    
    void clear()
    {
        if (! emptyKey)
            this->zero();
        else
        {
            struct dbr_hash_slot<KT> *slot;
            for (int i = 0; i < this->capacity; i++)
            {
                slot = this->data +i;
                slot->key = emptyKey;
            }
            this->count = 0;
        }
    }
    
    inline bool remove(KT key)
    {
        struct dbr_hash_slot<KT> *slot;
        if (! search(key, &slot))
            return false;
        slot->key = deletedKey;
        int array_slot = slot->array_slot;
        if (this->count == 1 || this->count-1 == array_slot)
        {
            this->count--;
            return true;
        }
        data_array[array_slot] = data_array[--this->count];
        if (! search(data_array[array_slot], &slot))
        {
            log_debug("DBR-Hash-Error: (%d; array_slot=%d; count=%d)\n", data_array[array_slot], array_slot, this->count);
            throw zmm::Exception(_("DBR-Hash-Error: key in data_array not found in hashtable"));
        }
        slot->array_slot = array_slot;
        return true;
    }
    
    inline void put(KT key)
    {
        struct dbr_hash_slot<KT> *slot;
        if (! search(key, &slot))
        {
#ifdef TOMBDEBUG
            if (this->count >= realCapacity)
                throw zmm::Exception(_("count (") + this->count + ") reached realCapacity (" + realCapacity + ")!\n");
#endif
            slot->key = key;
            slot->array_slot = this->count;
            data_array[this->count++] = key;
        }
    }
    
    /// \brief returns all keys as an array. After the deletion of the DBRHash object, the array is invalid!
    inline void getAll(hash_data_array_t<KT> *hash_data_array)
    {
        hash_data_array->size = this->count;
        hash_data_array->data = data_array;
    }
    
    zmm::String debugGetAll()
    {
        zmm::Ref<zmm::StringBuffer> buf(new zmm::StringBuffer());
        if (this->count == 0)
            return _("");
        *buf << data_array[0];
        for (int i = 1; i < this->count; i++)
        {
            *buf << ", " << data_array[i];
        }
        return buf->toString();
    }
    
    /*
     * is this really needed? seems so make no sense...
    inline void put(KT key, hash_slot_t destSlot)
    {
        KT *slot = (KT *)destSlot;
        if (slot->key == emptyKey)
        {
            slot->key = key;
            this->count++;
        }
    }
    */
    
    inline bool exists(KT key)
    {
        struct dbr_hash_slot<KT> *slot;
        return search(key, &slot);
    }
    
    /*
     * unneded, i think...
     
    inline bool exists(KT key, hash_slot_t *destSlot)
    {
        return search(key, (KT **)destSlot);
    }
    */
};

#endif // __HASH_DBR_HASH_H__

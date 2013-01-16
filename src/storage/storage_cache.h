/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    storage_cache.h - this file is part of MediaTomb.
    
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

/// \file storage_cache.h

#ifndef __STORAGE_CACHE_H__
#define __STORAGE_CACHE_H__

#include "zmmf/zmmf.h"
#include "common.h"
#include "hash.h"
#include "sync.h"
#include "cache_object.h"

#define STORAGE_CACHE_CAPACITY 29989
#define STORAGE_CACHE_MAXFILL 9973

class StorageCache : public zmm::Object
{
public:
    StorageCache();
    
    zmm::Ref<CacheObject> getObject(int id);
    zmm::Ref<CacheObject> getObjectDefinitely(int id);
    bool removeObject(int id);
    void clear();
    
    zmm::Ref<zmm::Array<CacheObject> > getObjects(zmm::String location);
    void checkLocation(zmm::Ref<CacheObject> obj);
    
    // a child was added to the specified object - update numChildren accordingly,
    // if the object has cached information
    void addChild(int id);
    
    bool flushed();
    
    zmm::Ref<Mutex> getMutex() { return mutex; }
    
private:
    
    int capacity;
    int maxfill;
    bool hasBeenFlushed;
    
    void ensureFillLevelOk();
    
    zmm::Ref<DBOHash<int,CacheObject> > idHash;
    zmm::Ref<DSOHash<zmm::Array<CacheObject> > > locationHash;
    zmm::Ref<Mutex> mutex;
};

#endif // __STORAGE_CACHE_H__

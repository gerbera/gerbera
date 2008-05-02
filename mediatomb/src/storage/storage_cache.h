/*MT*/

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

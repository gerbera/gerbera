/*MT*/

/// \file storage_cache.h

#ifndef __STORAGE_CACHE_H__
#define __STORAGE_CACHE_H__

#include "zmmf/zmmf.h"
#include "common.h"
#include "hash.h"
#include "cache_object.h"

#define STORAGE_CACHE_CAPACITY 29989
#define STORAGE_CACHE_MAXFILL 9973

class StorageCache : public zmm::Object
{
public:
    StorageCache();
    
    zmm::Ref<CacheObject> getObject(int id);
    zmm::Ref<CacheObject> getObjectDefinitly(int id);
    bool removeObject(int id);
    void clear();
    
private:
    
    int capacity;
    int maxfill;
    
    void ensureFillLevelOk();
    
    zmm::Ref<DBOHash<int,CacheObject> > hash;
};

#endif // __STORAGE_CACHE_H__

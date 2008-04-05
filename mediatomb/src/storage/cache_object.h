/*MT*/

/// \file cache_object.h

#ifndef __CACHE_OBJECT_H__
#define __CACHE_OBJECT_H__

#include "zmmf/zmmf.h"
#include "common.h"
#include "cds_objects.h"

class CacheObject : public zmm::Object
{
public:
    CacheObject();
    
    void setParentID(int parentID) { this->parentID = parentID; }
    int getParentID() { return parentID; }
    bool knowsParentID() { return parentID != INVALID_OBJECT_ID; }
    
    void setRefID(int refID) { this->refID = refID; knowRefID = true; }
    int getRefID() { return refID; }
    bool knowsRefID() { return knowRefID; }
    
    void setObject(zmm::Ref<CdsObject> obj);
    zmm::Ref<CdsObject> getObject() { return obj; }
    bool knowsObject() { return obj != nil; }
    
    void setHasChildren(bool hasChildren);
    bool getHasChildren() { return hasChildren; }
    bool knowsHasChildren() { return knowHasChildren; }
    
    void setObjectType(int objectType) { this->objectType = objectType; knowObjectType = true; }
    int getObjectType() { return objectType; }
    bool knowsObjectType() { return knowObjectType; }
    
private:
    
    int parentID;
    int refID;
    bool knowRefID;
    zmm::Ref<CdsObject> obj;
    bool knowHasChildren;
    bool hasChildren;
    int objectType;
    bool knowObjectType;
};

#endif // __CACHE_OBJECT_H_

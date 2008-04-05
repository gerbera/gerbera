/*MT*/

/// \file cache_object.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "common.h"
#include "cache_object.h"

using namespace zmm;

CacheObject::CacheObject()
{
    parentID = INVALID_OBJECT_ID;
    refID = INVALID_OBJECT_ID;
    knowRefID = false;
    obj = nil;
    hasChildren = false;
    knowHasChildren = false;
    objectType = 0;
    knowObjectType = false;
}


void CacheObject::setObject(Ref<CdsObject> obj)
{
    this->obj = obj;
    setParentID(obj->getParentID());
    setRefID(obj->getRefID());
    setObjectType(obj->getObjectType());
    knowHasChildren = false;
    
    if (IS_CDS_CONTAINER(objectType))
    {
        location = String(obj->isVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX) + obj->getLocation();
    }
    else if (IS_CDS_ITEM(objectType))
    {
        if (IS_CDS_PURE_ITEM(objectType) && ! obj->isVirtual())
            location = String(LOC_FILE_PREFIX) + obj->getLocation();
    }
}

void CacheObject::setHasChildren(bool hasChildren)
{
    knowHasChildren = true;
    this->hasChildren = hasChildren;
}

/*MT*/

/// \file cache_object.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

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
    
    /*
    if (IS_CDS_CONTAINER(obj->getObjectType()))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        setHasChildren(cont->getChildCount() > 0);
    }
    */
}

void CacheObject::setHasChildren(bool hasChildren)
{
    knowHasChildren = true;
    this->hasChildren = hasChildren;
}

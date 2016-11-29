/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cache_object.cc - this file is part of MediaTomb.
    
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

/// \file cache_object.cc

#include "../common.h"
#include "cache_object.h"

using namespace zmm;

CacheObject::CacheObject()
{
    parentID = INVALID_OBJECT_ID;
    refID = INVALID_OBJECT_ID;
    knowRefID = false;
    obj = nil;
    numChildren = 0;
    knowNumChildren = false;
    objectType = 0;
    knowObjectType = false;
    virtualObj = true;
    knowVirtualObj = false;
}


void CacheObject::setObject(Ref<CdsObject> obj)
{
    Ref<CdsObject> nObj = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(nObj);
    this->obj = nObj;
    setParentID(nObj->getParentID());
    setRefID(nObj->getRefID());
    setObjectType(nObj->getObjectType());
    
    knowVirtualObj = true;
    setVirtual(nObj->isVirtual());
    
    if (IS_CDS_CONTAINER(objectType))
    {
        location = String(nObj->isVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX) + nObj->getLocation();
    }
    else if (IS_CDS_ITEM(objectType))
    {
        if (IS_CDS_PURE_ITEM(objectType))
            location = String(LOC_FILE_PREFIX) + nObj->getLocation();
    }
}

void CacheObject::debug()
{
    log_debug("== cache object ==\n");
    log_debug("== knows parent ID: %d %d\n", knowsParentID(), parentID);
    log_debug("== knows ref ID: %d %d\n", knowsRefID(), refID);
    log_debug("== knows object: %d\n", knowsObject());
    log_debug("== knows num children: %d %d\n", knowsNumChildren(), numChildren);
    log_debug("== knows object type: %d %d\n", knowsObjectType(), objectType);
    log_debug("== knows location: %d %s\n", knowsLocation(), (knowsLocation() ? location.c_str() : ""));
    log_debug("== knows virtual: %d %d\n", knowsVirtual(), virtualObj);
    log_debug("==================\n");
}

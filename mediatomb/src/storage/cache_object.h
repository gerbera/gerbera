/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cache_object.h - this file is part of MediaTomb.
    
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
    
    void debug();
    
    void setParentID(int parentID) { this->parentID = parentID; }
    int getParentID() { return parentID; }
    bool knowsParentID() { return parentID != INVALID_OBJECT_ID; }
    
    void setRefID(int refID) { this->refID = refID; knowRefID = true; }
    int getRefID() { return refID; }
    bool knowsRefID() { return knowRefID; }
    
    void setObject(zmm::Ref<CdsObject> obj);
    zmm::Ref<CdsObject> getObject() { return obj; }
    bool knowsObject() { return obj != nil; }
    
    void setNumChildren(int numChildren) { this->numChildren = numChildren; knowNumChildren = true; }
    int getNumChildren() { return numChildren; }
    bool knowsNumChildren() { return knowNumChildren; }
    
    void setObjectType(int objectType) { this->objectType = objectType; knowObjectType = true; }
    int getObjectType() { return objectType; }
    bool knowsObjectType() { return knowObjectType; }
    
    void setLocation(zmm::String location) { this->location = location; }
    zmm::String getLocation() { return location; }
    bool knowsLocation() { return location != nil; }
    
    void setVirtual(bool virtualObj) { this->virtualObj = virtualObj; knowVirtualObj = true; }
    bool getVirtual() { return virtualObj; }
    bool knowsVirtual() { return knowVirtualObj; }
    
private:
    
    int parentID;
    int refID;
    bool knowRefID;
    zmm::Ref<CdsObject> obj;
    bool knowNumChildren;
    int numChildren;
    int objectType;
    bool knowObjectType;
    bool virtualObj;
    bool knowVirtualObj;
    
    zmm::String location;
};

#endif // __CACHE_OBJECT_H_

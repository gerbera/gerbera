/*  storage.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "zmmf/zmmf.h"
#include "cds_objects.h"
#include "dictionary.h"

typedef enum
{
    BROWSE_METADATA = 1,
    BROWSE_DIRECT_CHILDREN = 2
} browse_flag_t;

typedef enum
{
    PRIMARY_STORAGE = 1,
    FILESYSTEM_STORAGE = 2
} storage_type_t;

class BrowseParam : public zmm::Object
{
protected:
    int flag;
    zmm::String objectID;

    int startIndex;
    int requestedCount;

    // output parameters
    int totalMatches;

public:
    BrowseParam(zmm::String objectID, int flag);

    int getFlag();
    zmm::String getObjectID();

    void setRange(int startIndex, int requestedCount);
    void setStartingIndex(int startIndex);
    void setRequestedCount(int requestedCount);

    int getStartingIndex();
    int getRequestedCount();

    int getTotalMatches();
    void setTotalMatches(int x);

};

class Storage : public zmm::Object
{
public:
    Storage();

    virtual void init() = 0;
    virtual void addObject(zmm::Ref<CdsObject> object) = 0;
    virtual void updateObject(zmm::Ref<CdsObject> object) = 0;
    virtual void eraseObject(zmm::Ref<CdsObject> object) = 0;

    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param) = 0;
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes() = 0;

    virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, zmm::String parentID) = 0;

    /* utility methods */
    virtual zmm::Ref<CdsObject> loadObject(zmm::String objectID);
    virtual void removeObject(zmm::Ref<CdsObject> object);
    virtual void removeObject(zmm::String objectID);
    virtual zmm::Ref<zmm::Array<CdsContainer> > getContainerPath(zmm::String objectID);

    /* accounting methods */
    virtual int getTotalFiles(){ return 0; }
    
    /* static methods */
    static zmm::Ref<Storage> getInstance(storage_type_t type = PRIMARY_STORAGE);
protected:
    void getContainerPath(zmm::Ref<zmm::Array<CdsContainer> > arr, zmm::String objectID);
};

#endif // __STORAGE_H__


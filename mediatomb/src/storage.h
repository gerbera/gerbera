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

#include "autoconfig.h"
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
    SELECT_BASIC = 0,
    SELECT_EXTENDED,
    SELECT_FULL,

    SELECT__MAX
} select_mode_t;

/* flags for SelectParam */
#define FILTER_PARENT_ID 1
#define FILTER_REF_ID 2
#define FILTER_PARENT_ID_CONTAINERS 3
#define FILTER_PARENT_ID_ITEMS 4

class BrowseParam : public zmm::Object
{
protected:
    int flag;
    int objectID;

    int startingIndex;
    int requestedCount;

    // output parameters
    int totalMatches;

public:
    bool containersOnly;

    inline BrowseParam(int objectID, int flag)
    {
        this->objectID = objectID;
        this->flag = flag;
        startingIndex = 0;
        requestedCount = 0;
        containersOnly = false;
    }

    inline int getFlag() { return flag;}
    inline int getObjectID() { return objectID; }

    inline void setRange(int startingIndex, int requestedCount)
    {
        this->startingIndex = startingIndex;
        this->requestedCount = requestedCount;
    }
    inline void setStartingIndex(int startingIndex)
    { this->startingIndex = startingIndex; }

    inline void setRequestedCount(int requestedCount)
    { this->requestedCount = requestedCount; }

    inline int getStartingIndex() { return startingIndex; }
    inline int getRequestedCount() { return requestedCount; }

    inline int getTotalMatches() { return totalMatches; }

    inline void setTotalMatches(int totalMatches)
    { this->totalMatches = totalMatches; }

};


class Storage;
class SelectParam : public zmm::Object
{
public:
    inline SelectParam(int flags, int iArg1)
    {
        this->flags = flags;
        this->iArg1 = iArg1;
    }
public:
    int flags;
    int iArg1;
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

    virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param,
                                                           select_mode_t mode = SELECT_FULL) = 0;
    
    virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, int parentID) = 0;
    virtual void incrementUpdateIDs(int *ids, int size) = 0;

    /* utility methods */
    virtual zmm::Ref<CdsObject> loadObject(int objectID,
                                           select_mode_t mode = SELECT_FULL);
    virtual void removeObject(zmm::Ref<CdsObject> object);
    virtual void removeObject(int objectID);
    virtual zmm::Ref<zmm::Array<CdsObject> > getObjectPath(int objectID);

    /* accounting methods */
    virtual int getTotalFiles(){ return 0; }
    
    /* static methods */
    static zmm::Ref<Storage> getInstance();
    
    virtual void shutdown() = 0;
protected:
    void getObjectPath(zmm::Ref<zmm::Array<CdsObject> > arr, int objectID);
};

#endif // __STORAGE_H__


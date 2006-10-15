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
#include "sync.h"
#include "hash.h"

typedef enum
{
    BROWSE_METADATA = 1,
    BROWSE_DIRECT_CHILDREN = 2
} browse_flag_t;

/*
typedef enum
{
    SELECT_BASIC = 0,
    SELECT_EXTENDED,
    SELECT_FULL,

    SELECT__MAX
} select_mode_t;
*/

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

    /// \brief Adds a virtual container chain specified by path.
    /// \param path container path separated by '/'. Slashes in container
    /// titles must be escaped. 
    /// \param containerID will be filled in by the function
    /// \param updateID will be filled in by the function
    ///
    /// The function gets a path (i.e. "/Audio/All Music/") and will create
    /// the container path if needed. The container ID will be filled in with
    /// the object ID of the container that is last in the path. The
    /// updateID will hold the objectID of the container that was changed,
    /// in case new containers were created during the operation.
    virtual void addContainerChain(zmm::String path, int *containerID, int *updateID) = 0;
    
    /// \brief Builds the container path. Fetches the path of the
    /// parent and adds the title
    /// \param parentID the parent id of the parent container
    /// \param title the title of the container to add to the path.
    /// It will be escaped.
    virtual zmm::String buildContainerPath(int parentID, zmm::String title) = 0;
    
    virtual void updateObject(zmm::Ref<CdsObject> object) = 0;

    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param) = 0;
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes() = 0;

    virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param) = 0;
    
    //virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, int parentID) = 0;
    virtual zmm::Ref<CdsObject> findObjectByPath(zmm::String path) = 0;
    virtual zmm::Ref<CdsObject> findObjectByFilename(zmm::String path) = 0;
    virtual int findObjectIDByPath(zmm::String fullpath) = 0;
    virtual void incrementUpdateIDs(int *ids, int size) = 0;
    
    /* utility methods */
    virtual zmm::Ref<CdsObject> loadObject(int objectID) = 0;
    virtual int getChildCount(int contId, bool containersOnly = false) = 0;
    
    /// \brief Removed the object identified by the objectID from the database.
    /// all references will be automatically removed. If the object is
    /// a container, all children will be also removed automatically. If
    /// the object is a reference to another object, the "all" flag
    /// determines, if the main object will be removed too.
    /// \param objectID the object id of the object to remove
    /// \param all if true and the object to be removed is a reference
    /// to another object, the referenced object (and all it's references)
    /// will be removed too. Default: false.
    virtual void removeObject(int objectID, bool all) = 0;
    
    //virtual zmm::Ref<zmm::Array<CdsObject> > getObjectPath(int objectID);
    
    /// \brief Determines if a folder given by it's full path is in the database
    /// \param path the path of the folder
    /// \return objectID if the folder exits, otherwise a negaive value
    virtual int isFolderInDatabase(zmm::String path) = 0;
    
    /// \brief Determines if a file given by it's name is in the database.
    /// \param parentID parent container
    /// \param filename name of the file as stored on disk
    /// \return objectID if file exists in the database, otherwise a negative value
    virtual int isFileInDatabase(zmm::String path) = 0;
    
    /// \brief Get all objects under the given parentID.
    /// \param parentID parent container
    /// \return DBHash containing the objectID's 
    virtual zmm::Ref<DBRHash<int> > getObjects(int parentID) = 0;
    
    /// \brief Remove all objects found in list
    /// \param parentID ID of the parent container
    /// \param list a DBHash containing objectIDs that have to be removed
    virtual void removeObjects(zmm::Ref<DBRHash<int> > list) = 0;
    
    /* accounting methods */
    virtual int getTotalFiles() { return 0; }
    
    /* internal setting methods */
    virtual zmm::String getInternalSetting(zmm::String key) = 0;
    virtual void storeInternalSetting(zmm::String key, zmm::String value) = 0;
    
    /* static methods */
    static zmm::Ref<Storage> getInstance();
    
    virtual void shutdown() = 0;
protected:
    int uiUpdateId;
    
    /* helper for addContainerChain */
    static void stripAndUnescapeVirtualContainerFromPath(zmm::String path, zmm::String &first, zmm::String &last);
    
    //void getObjectPath(zmm::Ref<zmm::Array<CdsObject> > arr, int objectID);
    
    static Mutex mutex;
};

#endif // __STORAGE_H__


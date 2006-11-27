/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    storage.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file storage.h

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "autoconfig.h"
#include "zmmf/zmmf.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "sync.h"
#include "hash.h"
#include "autoscan.h"

#define BROWSE_DIRECT_CHILDREN      0x00000001
#define BROWSE_ITEMS                0x00000002
#define BROWSE_CONTAINERS           0x00000004
#define BROWSE_EXACT_CHILDCOUNT     0x00000008

class BrowseParam : public zmm::Object
{
protected:
    unsigned int flags;
    int objectID;
    
    int startingIndex;
    int requestedCount;
    
    // output parameters
    int totalMatches;
    
public:
    inline BrowseParam(int objectID, unsigned int flags)
    {
        this->objectID = objectID;
        this->flags = flags;
        startingIndex = 0;
        requestedCount = 0;
    }
    
    inline int getFlags() { return flags; }
    inline unsigned int getFlag(unsigned int mask) { return flags & mask; }
    inline void setFlags(unsigned int flags) { this->flags = flags; }
    inline void setFlag(unsigned int mask) { flags |= mask; }
    inline void changeFlag(unsigned int mask, bool value) { if (value) setFlag(mask); else clearFlag(mask); }
    inline void clearFlag(unsigned int mask) { flags &= !mask; }
    
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

class Storage : public zmm::Object
{
public:
    Storage();

    virtual void init() = 0;
    virtual void addObject(zmm::Ref<CdsObject> object, int *changedContainer) = 0;

    /// \brief Adds a virtual container chain specified by path.
    /// \param path container path separated by '/'. Slashes in container
    /// titles must be escaped. 
    /// \param containerID will be filled in by the function
    /// \param updateID will be filled in by the function only if it is set to INVALID_OBJECT_ID
    /// and it is necessary to update a container. Otherwise it will be left unchanged.
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
    
    virtual void updateObject(zmm::Ref<CdsObject> object, int *changedContainer) = 0;
    
    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param) = 0;
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes() = 0;
    
    //virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param) = 0;
    
    /// \brief Loads a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// if the path ends with DIR_SEPERATOR, as file otherwise
    /// multiple DIR_SEPERATOR are irgnored
    /// \return the CdsObject
    virtual zmm::Ref<CdsObject> findObjectByPath(zmm::String path) = 0;
    
    /// \brief checks for a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// if the path ends with DIR_SEPERATOR, as file otherwise
    /// multiple DIR_SEPERATOR are irgnored
    /// \return the obejectID
    virtual int findObjectIDByPath(zmm::String fullpath) = 0;
    
    /// \brief increments the updateIDs for the given objectIDs
    /// \param ids pointer to the array of ids
    /// \param size number of entries in the given array
    /// \return a String for UPnP: a CSV list; for every existing object:
    ///  "id,update_id"
    virtual zmm::String incrementUpdateIDs(int *ids, int size) = 0;
    
    /* utility methods */
    virtual zmm::Ref<CdsObject> loadObject(int objectID) = 0;
    virtual int getChildCount(int contId, bool containers = true, bool items = true) = 0;
    
    /// \brief Removes the object identified by the objectID from the database.
    /// all references will be automatically removed. If the object is
    /// a container, all children will be also removed automatically. If
    /// the object is a reference to another object, the "all" flag
    /// determines, if the main object will be removed too.
    /// \param objectID the object id of the object to remove
    /// \param all if true and the object to be removed is a reference
    /// \param objectType pointer to an int; will be filled with the objectType of
    /// the removed object, if not NULL
    /// \return parent id of the removed object
    /// to another object, the referenced object (and all it's references)
    /// will be removed too. Default: false.
    virtual int removeObject(int objectID, bool all, int *objectType = NULL) = 0;
    
    //virtual zmm::Ref<zmm::Array<CdsObject> > getObjectPath(int objectID);
    
    /// \brief Get all objects under the given parentID.
    /// \param parentID parent container
    /// \return DBHash containing the objectID's 
    virtual zmm::Ref<DBRHash<int> > getObjects(int parentID) = 0;
    
    /// \brief Remove all objects found in list
    /// \param parentID ID of the parent container
    /// \param list a DBHash containing objectIDs that have to be removed
    /// \return true if a container was in the list, false if the list was empty
    /// or there were only items in the list
    virtual bool removeObjects(zmm::Ref<DBRHash<int> > list) = 0;
    
    /* accounting methods */
    virtual int getTotalFiles() { return 0; }
    
    /* internal setting methods */
    virtual zmm::String getInternalSetting(zmm::String key) = 0;
    virtual void storeInternalSetting(zmm::String key, zmm::String value) = 0;
    
    /* autoscan methods */
    virtual zmm::Ref<AutoscanList> getAutoscanList(scan_mode_t scanmode) = 0;
    virtual void addAutoscanDirectory(zmm::Ref<AutoscanDirectory> adir) = 0;
    virtual void removeAutoscanDirectory(int objectId) = 0;
    virtual bool isAutoscanDirectory(int objectId) = 0;
    virtual void autoscanUpdateLM(zmm::Ref<AutoscanDirectory> adir) = 0;
    
    /* static methods */
    static zmm::Ref<Storage> getInstance();
    
    virtual void shutdown() = 0;

    /// \brief Ensures that a container given by it's location on disk is
    /// present in the database. If it does not exist it will be created, but
    /// it's content will not be added.
    /// 
    /// \param *changedContainer returns the ID for the UpdateManager
    /// \return objectID of the container given by path
    virtual int ensurePathExistence(zmm::String path, int *changedContainer) = 0;
        
protected:
    int uiUpdateId;
    
    /* helper for addContainerChain */
    static void stripAndUnescapeVirtualContainerFromPath(zmm::String path, zmm::String &first, zmm::String &last);
    
    //void getObjectPath(zmm::Ref<zmm::Array<CdsObject> > arr, int objectID);
    
    static zmm::Ref<Mutex> mutex;
};

#endif // __STORAGE_H__


/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    storage.h - this file is part of MediaTomb.
    
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

/// \file storage.h

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "autoscan.h"
#include "cds_objects.h"
#include "zmm/zmmf.h"

#define BROWSE_DIRECT_CHILDREN 0x00000001
#define BROWSE_ITEMS 0x00000002
#define BROWSE_CONTAINERS 0x00000004
#define BROWSE_EXACT_CHILDCOUNT 0x00000008
#define BROWSE_TRACK_SORT 0x00000010
#define BROWSE_HIDE_FS_ROOT 0x00000020

class BrowseParam {
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
    inline void changeFlag(unsigned int mask, bool value)
    {
        if (value)
            setFlag(mask);
        else
            clearFlag(mask);
    }
    inline void clearFlag(unsigned int mask) { flags &= !mask; }

    inline int getObjectID() { return objectID; }

    inline void setRange(int startingIndex, int requestedCount)
    {
        this->startingIndex = startingIndex;
        this->requestedCount = requestedCount;
    }
    inline void setStartingIndex(int startingIndex)
    {
        this->startingIndex = startingIndex;
    }

    inline void setRequestedCount(int requestedCount)
    {
        this->requestedCount = requestedCount;
    }

    inline int getStartingIndex() { return startingIndex; }
    inline int getRequestedCount() { return requestedCount; }

    inline int getTotalMatches() { return totalMatches; }

    inline void setTotalMatches(int totalMatches)
    {
        this->totalMatches = totalMatches;
    }
};

class SearchParam {
protected:
    std::string containerID;
    std::string searchCrit;
    int startingIndex;
    int requestedCount;

public:
    SearchParam(const std::string& containerID, const std::string& searchCriteria, int startingIndex,
        int requestedCount)
        : containerID(containerID)
        , searchCrit(searchCriteria)
        , startingIndex(startingIndex)
        , requestedCount(requestedCount)
    {
    }
    const std::string& searchCriteria() const { return searchCrit; };
    int getStartingIndex() { return startingIndex; };
    int getRequestedCount() { return requestedCount; };
};

// forward declaration
class ConfigManager;
class Timer;

class Storage {
public:
    Storage(std::shared_ptr<ConfigManager> config);
    virtual void init() = 0;

    /// \brief shutdown the Storage with its possible threads
    virtual void shutdown() = 0;

    virtual void addObject(std::shared_ptr<CdsObject> object, int* changedContainer) = 0;

    /// \brief Adds a virtual container chain specified by path.
    /// \param path container path separated by '/'. Slashes in container
    /// titles must be escaped.
    /// \param lastClass upnp:class of the last container in the chain, it
    /// is only set when the container is created for the first time.
    /// \param lastRefID reference id of the last container in the chain,
    /// INVALID_OBJECT_ID indicates that the id will not be set.
    /// \param containerID will be filled in by the function
    /// \param updateID will be filled in by the function only if it is set to INVALID_OBJECT_ID
    /// and it is necessary to update a container. Otherwise it will be left unchanged.
    ///
    /// The function gets a path (i.e. "/Audio/All Music/") and will create
    /// the container path if needed. The container ID will be filled in with
    /// the object ID of the container that is last in the path. The
    /// updateID will hold the objectID of the container that was changed,
    /// in case new containers were created during the operation.
    virtual void addContainerChain(std::string path, std::string lastClass, int lastRefID, int* containerID,
        int* updateID, const std::map<std::string,std::string>& lastMetadata) = 0;

    /// \brief Builds the container path. Fetches the path of the
    /// parent and adds the title
    /// \param parentID the parent id of the parent container
    /// \param title the title of the container to add to the path.
    /// It will be escaped.
    virtual std::string buildContainerPath(int parentID, std::string title) = 0;

    virtual void updateObject(std::shared_ptr<CdsObject> object, int* changedContainer) = 0;

    virtual std::vector<std::shared_ptr<CdsObject>> browse(const std::unique_ptr<BrowseParam>& param) = 0;
    virtual std::vector<std::shared_ptr<CdsObject>> search(const std::unique_ptr<SearchParam>& param, int* numMatches) = 0;

    virtual std::vector<std::string> getMimeTypes() = 0;

    //virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param) = 0;

    /// \brief Loads a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// if the path ends with DIR_SEPERATOR, as file otherwise
    /// multiple DIR_SEPERATOR are irgnored
    /// \return the CdsObject
    virtual std::shared_ptr<CdsObject> findObjectByPath(std::string path) = 0;

    /// \brief checks for a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// if the path ends with DIR_SEPERATOR, as file otherwise
    /// multiple DIR_SEPERATOR are irgnored
    /// \return the obejectID
    virtual int findObjectIDByPath(std::string fullpath) = 0;

    /// \brief increments the updateIDs for the given objectIDs
    /// \param ids pointer to the array of ids
    /// \param size number of entries in the given array
    /// \return a String for UPnP: a CSV list; for every existing object:
    ///  "id,update_id"
    virtual std::string incrementUpdateIDs(const std::unique_ptr<std::unordered_set<int>>& ids) = 0;

    /* utility methods */
    virtual std::shared_ptr<CdsObject> loadObject(int objectID) = 0;
    virtual int getChildCount(int contId, bool containers = true, bool items = true, bool hideFsRoot = false) = 0;

    virtual std::string findFolderImage(int id, std::string trackArtBase) = 0;

    class ChangedContainers {
    public:
        // Signed because IDs start at -1.
        std::vector<int32_t> upnp;
        std::vector<int32_t> ui;
    };

    /// \brief Removes the object identified by the objectID from the database.
    /// all references will be automatically removed. If the object is
    /// a container, all children will be also removed automatically. If
    /// the object is a reference to another object, the "all" flag
    /// determines, if the main object will be removed too.
    /// \param objectID the object id of the object to remove
    /// \param all if true and the object to be removed is a reference
    /// \param objectType pointer to an int; will be filled with the objectType of
    /// the removed object, if not NULL
    /// \return changed container ids
    virtual std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) = 0;

    /// \brief Get all objects under the given parentID.
    /// \param parentID parent container
    /// \param withoutContainer if false: all children are returned; if true: only items are returned
    /// \return DBHash containing the objectID's - nullptr if there are none!
    virtual std::unique_ptr<std::unordered_set<int>> getObjects(int parentID, bool withoutContainer) = 0;

    /// \brief Remove all objects found in list
    /// \param list a DBHash containing objectIDs that have to be removed
    /// \param all if true and the object to be removed is a reference
    /// \return changed container ids
    virtual std::unique_ptr<ChangedContainers> removeObjects(const std::unique_ptr<std::unordered_set<int>>& list, bool all = false) = 0;

    /// \brief Loads an object given by the online service ID.
    virtual std::shared_ptr<CdsObject> loadObjectByServiceID(std::string serviceID) = 0;

    /// \brief Return an array of object ID's for a particular service.
    ///
    /// In the database, the service is identified by a service id prefix.
    virtual std::unique_ptr<std::vector<int>> getServiceObjectIDs(char servicePrefix) = 0;

    /* accounting methods */
    virtual int getTotalFiles() = 0;

    /* internal setting methods */
    virtual std::string getInternalSetting(std::string key) = 0;
    virtual void storeInternalSetting(std::string key, std::string value) = 0;

    /* autoscan methods */
    virtual void updateAutoscanPersistentList(ScanMode scanmode, std::shared_ptr<AutoscanList> list) = 0;
    virtual zmm::Ref<AutoscanList> getAutoscanList(ScanMode scanode) = 0;
    virtual void addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) = 0;
    virtual void updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) = 0;
    virtual void removeAutoscanDirectoryByObjectID(int objectID) = 0;
    virtual void removeAutoscanDirectory(int autoscanID) = 0;
    /// \brief checks if the given object is a direct or indirect child of
    /// a recursive autoscan start point
    /// \param objectID the object id of the object to check
    /// \return the objectID of the nearest matching autoscan start point or
    /// INVALID_OBJECT_ID if it is not a child.
    virtual int isAutoscanChild(int objectID) = 0;

    /// \brief returns wheather the given id is an autoscan start point and if yes, if it is persistent
    /// \param objectId the object id to check
    /// \return 0 if the given id is no autoscan start point, 1 if it is a non-persistent one, 2 if it is a persistent one
    virtual int getAutoscanDirectoryType(int objectId) = 0;

    /// \brief returns wheather the given id is an autoscan start point and if yes, if it is recursive
    /// \param objectId the object id to check
    /// \return 0 if the given id is no autoscan start point, 1 if it is a non-recursive one, 2 if it is a recursive on
    virtual int isAutoscanDirectoryRecursive(int objectId) = 0;

    /// \brief returns the AutoscanDirectory for the given objectID or nullptr if
    /// it's not an autoscan start point - scan id will be invalid
    /// \param objectID the object id to get the AutoscanDirectory for
    /// \return nullptr if the given id is no autoscan start point,
    /// or the matching AutoscanDirectory
    virtual std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) = 0;

    /// \brief updates the last modified info for the given AutoscanDirectory
    /// in the database
    /// \param adir the AutoscanDirectory to be updated
    virtual void autoscanUpdateLM(std::shared_ptr<AutoscanDirectory> adir) = 0;

    virtual void checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir) = 0;

    virtual std::unique_ptr<std::vector<int>> getPathIDs(int objectID) = 0;

    /// \brief Ensures that a container given by it's location on disk is
    /// present in the database. If it does not exist it will be created, but
    /// it's content will not be added.
    ///
    /// \param *changedContainer returns the ID for the UpdateManager
    /// \return objectID of the container given by path
    virtual int ensurePathExistence(std::string path, int* changedContainer) = 0;

    /// \brief clears the given flag in all objects in the DB
    virtual void clearFlagInDB(int flag) = 0;

    virtual std::string getFsRootName() = 0;

    virtual void threadCleanup() = 0;
    virtual bool threadCleanupRequired() = 0;

    virtual void doMetadataMigration() = 0;

protected:
    /* helper for addContainerChain */
    static void stripAndUnescapeVirtualContainerFromPath(std::string path, std::string& first, std::string& last);

    static std::shared_ptr<Storage> createInstance(std::shared_ptr<ConfigManager> config, std::shared_ptr<Timer> timer);
    friend class Server;

    virtual std::shared_ptr<Storage> getSelf() = 0;

protected:
    std::shared_ptr<ConfigManager> config;
};

#endif // __STORAGE_H__

/*MT*

    MediaTomb - http://www.mediatomb.cc/

    database.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file database.h

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <map>
#include <unordered_set>
#include <vector>

#include "util/grb_fs.h"

// forward declaration
class AutoscanList;
class AutoscanDirectory;
enum class AutoscanScanMode;
class CdsContainer;
class CdsObject;
struct ClientCacheEntry;
class ClientStatusDetail;
class Config;
class ConfigValue;
class Mime;
class Timer;

#define BROWSE_DIRECT_CHILDREN 0x00000001
#define BROWSE_ITEMS 0x00000002
#define BROWSE_CONTAINERS 0x00000004
#define BROWSE_EXACT_CHILDCOUNT 0x00000008
#define BROWSE_TRACK_SORT 0x00000010
#define BROWSE_HIDE_FS_ROOT 0x00000020

class BrowseParam {
protected:
    unsigned int flags;
    std::shared_ptr<CdsObject> object;

    int startingIndex {};
    int requestedCount {};
    std::string sortCrit;
    std::string group;
    bool showDynamicContainers { true };

    // output parameters
    int totalMatches {};

public:
    BrowseParam(std::shared_ptr<CdsObject> object, unsigned int flags)
        : flags(flags)
        , object(std::move(object))
    {
    }

    unsigned int getFlag(unsigned int mask) const { return flags & mask; }
    void setFlag(unsigned int mask) { flags |= mask; }
    void clearFlag(unsigned int mask) { flags &= ~mask; }

    std::shared_ptr<CdsObject> getObject() const { return object; }

    void setRange(int startingIndex, int requestedCount)
    {
        this->startingIndex = startingIndex;
        this->requestedCount = requestedCount;
    }
    void setStartingIndex(int startingIndex)
    {
        this->startingIndex = startingIndex;
    }

    void setRequestedCount(int requestedCount)
    {
        this->requestedCount = requestedCount;
    }

    void setSortCriteria(const std::string& sortCrit)
    {
        this->sortCrit = sortCrit;
    }

    void setGroup(const std::string& group)
    {
        this->group = group;
    }

    void setDynamicContainers(bool showDynamicContainers)
    {
        this->showDynamicContainers = showDynamicContainers;
    }

    bool getDynamicContainers() const { return showDynamicContainers; }
    int getStartingIndex() const { return startingIndex; }
    int getRequestedCount() const { return requestedCount; }
    int getTotalMatches() const { return totalMatches; }
    const std::string& getSortCriteria() const { return sortCrit; }
    const std::string& getGroup() const { return group; }

    void setTotalMatches(int totalMatches)
    {
        this->totalMatches = totalMatches;
    }
};

class SearchParam {
protected:
    std::string containerID;
    std::string searchCrit;
    std::string sortCrit;
    int startingIndex;
    int requestedCount;
    bool searchableContainers;
    std::string group;

public:
    SearchParam(std::string containerID, std::string searchCriteria, std::string sortCriteria, int startingIndex,
        int requestedCount, bool searchableContainers, std::string group)
        : containerID(std::move(containerID))
        , searchCrit(std::move(searchCriteria))
        , sortCrit(std::move(sortCriteria))
        , startingIndex(startingIndex)
        , requestedCount(requestedCount)
        , searchableContainers(searchableContainers)
        , group(std::move(group))
    {
    }
    const std::string& getSearchCriteria() const { return searchCrit; }
    const std::string& getContainerID() const { return containerID; }
    bool getSearchableContainers() const { return searchableContainers; }
    int getStartingIndex() const { return startingIndex; }
    int getRequestedCount() const { return requestedCount; }
    const std::string& getSortCriteria() const { return sortCrit; }
    const std::string& getGroup() const { return group; }
};

class StatsParam {
public:
    enum class StatsMode {
        Count,
        Size,
    };

protected:
    std::string mimeType;
    std::string upnpClass;
    StatsMode mode { StatsMode::Count };
    bool isVirtual { false };

public:
    StatsParam(StatsMode mode, std::string mimeType, std::string upnpClass, bool isVirtual)
        : mimeType(std::move(mimeType))
        , upnpClass(std::move(upnpClass))
        , mode(mode)
        , isVirtual(isVirtual)
    {
    }
    const std::string& getMimeType() const { return mimeType; }
    const std::string& getUpnpClass() const { return upnpClass; }
    StatsMode getMode() const { return mode; }
    bool getVirtual() const { return isVirtual; }
};

enum class DbFileType {
    Auto,
    Directory,
    File,
    Virtual,
};

class Database {
public:
    explicit Database(std::shared_ptr<Config> config);
    virtual ~Database() = default;
    virtual void init() = 0;

    /// \brief shutdown the Database with its possible threads
    virtual void shutdown() = 0;

    virtual void addObject(const std::shared_ptr<CdsObject>& object, int* changedContainer) = 0;

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
    virtual bool addContainer(int parentContainerId, std::string virtualPath, const std::shared_ptr<CdsContainer>& cont, int* containerID) = 0;

    /// \brief Builds the container path. Fetches the path of the
    /// parent and adds the title
    /// \param parentID the parent id of the parent container
    /// \param title the title of the container to add to the path.
    /// It will be escaped.
    virtual fs::path buildContainerPath(int parentID, const std::string& title) = 0;

    virtual void updateObject(const std::shared_ptr<CdsObject>& object, int* changedContainer) = 0;

    virtual std::vector<std::shared_ptr<CdsObject>> browse(BrowseParam& param) = 0;
    virtual std::vector<std::shared_ptr<CdsObject>> search(const SearchParam& param, int* numMatches) = 0;
    virtual std::vector<std::shared_ptr<CdsObject>> findObjectByContentClass(const std::string& contentClass) = 0;

    virtual std::vector<std::string> getMimeTypes() = 0;

    // virtual std::vector<std::shared_ptr<CdsObject>> selectObjects(const std::unique_ptr<SelectParam>& param) = 0;

    /// \brief Loads a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// \param wasRegularFile was a regular file before file was moved, now fs::is_regular_file returns false (used for inotify events)
    /// \return the CdsObject
    virtual std::shared_ptr<CdsObject> findObjectByPath(const fs::path& path, DbFileType fileType = DbFileType::Auto) = 0;

    /// \brief checks for a given (pc directory) object, identified by the given path
    /// from the database
    /// \param path the path of the object; object is interpreted as directory
    /// \param wasRegularFile was a regular file before file was moved, now fs::is_regular_file returns false (used for inotify events)
    /// \return the obejectID
    virtual int findObjectIDByPath(const fs::path& fullpath, DbFileType fileType = DbFileType::Auto) = 0;

    /// \brief increments the updateIDs for the given objectIDs
    /// \param ids pointer to the array of ids
    /// \param size number of entries in the given array
    /// \return a String for UPnP: a CSV list; for every existing object:
    ///  "id,update_id"
    virtual std::string incrementUpdateIDs(const std::unordered_set<int>& ids) = 0;

    /* utility methods */
    virtual std::shared_ptr<CdsObject> loadObject(int objectID) = 0;
    virtual std::shared_ptr<CdsObject> loadObject(const std::string& group, int objectID) = 0;
    virtual int getChildCount(int contId, bool containers = true, bool items = true, bool hideFsRoot = false) = 0;
    virtual std::map<int, int> getChildCounts(const std::vector<int>& contId, bool containers = true, bool items = true, bool hideFsRoot = false) = 0;

    struct ChangedContainers {
        // Signed because IDs start at -1.
        std::vector<std::int32_t> upnp;
        std::vector<std::int32_t> ui;
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
    virtual std::unique_ptr<ChangedContainers> removeObject(int objectID, const fs::path& path, bool all) = 0;

    /// \brief Get all objects under the given parentID.
    /// \param parentID parent container
    /// \param withoutContainer if false: all children are returned; if true: only items are returned
    /// \return DBHash containing the objectID's - nullptr if there are none!
    virtual std::size_t getObjects(int parentID, bool withoutContainer, std::unordered_set<int>& ret, bool full) = 0;
    virtual std::vector<std::pair<int, std::chrono::seconds>> getRefObjects(int objectId) = 0;
    virtual std::unordered_set<int> getUnreferencedObjects() = 0;

    /// \brief Remove all objects found in list
    /// \param list a DBHash containing objectIDs that have to be removed
    /// \param all if true and the object to be removed is a reference
    /// \return changed container ids
    virtual std::unique_ptr<ChangedContainers> removeObjects(const std::unordered_set<int>& list, bool all = false) = 0;

    /// \brief Loads an object given by the online service ID.
    virtual std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID) = 0;

    /// \brief Return an array of object ID's for a particular service.
    ///
    /// In the database, the service is identified by a service id prefix.
    virtual std::vector<int> getServiceObjectIDs(char servicePrefix) = 0;

    /* accounting methods */
    virtual long long getFileStats(const StatsParam& stats) = 0;
    virtual std::map<std::string, long long> getGroupStats(const StatsParam& stats) = 0;

    /* internal setting methods */
    virtual std::string getInternalSetting(const std::string& key) = 0;
    virtual void storeInternalSetting(const std::string& key, const std::string& value) = 0;

    /* autoscan methods */
    virtual std::shared_ptr<AutoscanList> getAutoscanList(AutoscanScanMode scanode) = 0;
    virtual void updateAutoscanList(AutoscanScanMode scanmode, const std::shared_ptr<AutoscanList>& list) = 0;

    /* config methods */
    virtual std::vector<ConfigValue> getConfigValues() = 0;
    virtual void removeConfigValue(const std::string& item) = 0;
    virtual void updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status = "unchanged") = 0;

    /* clients methods */
    virtual std::vector<ClientCacheEntry> getClients() = 0;
    virtual void saveClients(const std::vector<ClientCacheEntry>& cache) = 0;
    virtual std::shared_ptr<ClientStatusDetail> getPlayStatus(const std::string& group, int objectId) = 0;
    virtual void savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail) = 0;
    virtual std::vector<std::shared_ptr<ClientStatusDetail>> getPlayStatusList(int objectId) = 0;
    virtual std::vector<std::map<std::string, std::string>> getClientGroupStats() = 0;

    /// \brief returns the AutoscanDirectory for the given objectID or nullptr if
    /// it's not an autoscan start point - scan id will be invalid
    /// \param objectID the object id to get the AutoscanDirectory for
    /// \return nullptr if the given id is no autoscan start point,
    /// or the matching AutoscanDirectory
    virtual std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) = 0;
    virtual void addAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) = 0;
    virtual void updateAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) = 0;
    virtual void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) = 0;
    virtual void checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir) = 0;

    virtual std::vector<int> getPathIDs(int objectID) = 0;

    /// \brief Ensures that a container given by it's location on disk is
    /// present in the database. If it does not exist it will be created, but
    /// it's content will not be added.
    ///
    /// \param *changedContainer returns the ID for the UpdateManager
    /// \return objectID of the container given by path
    virtual int ensurePathExistence(const fs::path& path, int* changedContainer) = 0;

    virtual void threadCleanup() = 0;
    virtual bool threadCleanupRequired() const = 0;

protected:
    static std::shared_ptr<Database> createInstance(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<Timer>& timer);
    friend class Server;

    virtual std::shared_ptr<Database> getSelf() = 0;

    std::shared_ptr<Config> config;
};

#endif // __STORAGE_H__

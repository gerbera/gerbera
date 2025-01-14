/*GRB*

    Gerbera - https://gerbera.io/

    import_service.h - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file import_service.h

#ifndef __IMPORT_SERVICE_H__
#define __IMPORT_SERVICE_H__

#include "util/grb_fs.h"

#include <map>
#include <mutex>
#include <tuple>
#include <unordered_set>

// forward declarations
class AutoscanDirectory;
enum class AutoscanMediaMode;
class AutoScanSetting;
class CdsContainer;
class CdsItem;
class CdsObject;
class Config;
class ContentManager;
class Context;
class ConverterManager;
class Database;
class GenericTask;
class ImportService;
class Layout;
enum class LayoutType;
class MetadataService;
class Mime;
enum class ObjectType;
class ScriptingRuntime;
class UpnpMap;

#ifdef HAVE_JS
class PlaylistParserScript;
class MetafileParserScript;
#endif // HAVE_JS

enum class ImportState {
    New,
    Loaded,
    Created,
    Existing,
    WithLayout,
    ToDelete,
    LayoutDeleted,
    Broken = 99,
};

/// @brief State container class for imported files
class ContentState {
private:
    ImportState state;
    /// \brief directory_entry to process
    fs::directory_entry dirEntry;
    /// \brief Last modification time in the file system.
    /// In seconds since UNIX epoch.
    std::chrono::seconds mtime = std::chrono::seconds::zero();
    /// \brief CdsObject associated with the directory_entry
    std::shared_ptr<CdsObject> cdsObject;
    /// \brief CdsObject associated with the container (if cdsObject is a container)
    std::shared_ptr<CdsObject> firstObject;
    /// \brief counters of child object types for container type
    std::map<ObjectType, std::size_t> itemCounter;

    /// \brief parent container (if cdsObject is a item)
    std::shared_ptr<CdsContainer> parentObject;

public:
    ContentState(fs::directory_entry dirEntry,
        ImportState state,
        std::chrono::seconds mtime = std::chrono::seconds::zero(),
        std::shared_ptr<CdsObject> cdsObject = nullptr);

    void setObject(ImportState state, std::shared_ptr<CdsObject> cdsObject)
    {
        this->state = state;
        this->cdsObject = std::move(cdsObject);
    }
    void setFirstObject(std::shared_ptr<CdsObject> firstObject)
    {
        this->firstObject = std::move(firstObject);
    }
    void setParentObject(std::shared_ptr<CdsContainer> parentObject)
    {
        this->parentObject = std::move(parentObject);
    }
    std::shared_ptr<CdsObject> getObject() { return cdsObject; }
    std::shared_ptr<CdsObject> getFirstObject() { return firstObject; }
    std::shared_ptr<CdsContainer> getParentObject() { return parentObject; }
    fs::directory_entry getDirEntry() { return dirEntry; }

    /// \brief Set modification time of file.
    void setMTime(std::chrono::seconds mtime) { this->mtime = mtime; }
    /// \brief Retrieve the file modification time (in seconds since UNIX epoch).
    std::chrono::seconds getMTime() const { return mtime; }

    ImportState getState() const { return state; }
    void setState(ImportState newState) { state = newState; }
    void increaseItemCounter(ObjectType mt) { itemCounter[mt]++; }
    AutoscanMediaMode getMediaMode() const;
};

/// @brief Mapping logic to generate upnpClass from file (meta) data
class UpnpMap {
private:
    std::vector<std::tuple<std::string, std::string, std::string>> filters;

    bool checkValue(const std::string& op, const std::string& expect, const std::string& actual) const;
    bool checkValue(const std::string& op, int expect, int actual) const;

public:
    std::string mimeType;
    std::string upnpClass;

    UpnpMap(std::string mt, std::string cls, std::vector<std::tuple<std::string, std::string, std::string>> f)
        : filters(std::move(f))
        , mimeType(std::move(mt))
        , upnpClass(std::move(cls))
    {
    }

    bool isMatch(const std::shared_ptr<CdsItem>& item, const std::string& mt) const;
    static void initMap(std::vector<UpnpMap>& target, const std::map<std::string, std::string>& source);
};

/// @brief Implementation of import functionality
class ImportService {
private:
    std::shared_ptr<Context> context;
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<MetadataService> metadataService;
    std::shared_ptr<ConverterManager> converterManager;

    std::map<std::string, std::string> mimetypeContenttypeMap;
    std::map<std::string, std::string> mimetypeUpnpclassMap;
    std::map<std::string, std::string> configLayoutMapping;
    std::vector<UpnpMap> upnpMap {};

    std::shared_ptr<AutoscanDirectory> autoscanDir;
    std::map<AutoscanMediaMode, std::string> containerTypeMap;
    fs::path rootPath;
    std::string noMediaName;
    bool hasReadableNames { false };
    bool hasCaseSensitiveNames { true };
    bool hasDefaultDate { true };
    bool pcDirTypes { true };
    int containerImageParentCount { 2 };
    int containerImageMinDepth { 2 };
    std::vector<std::vector<std::pair<std::string, std::string>>> virtualDirKeys {};

    /// \brief cache for containers while creating new layout
    mutable std::map<std::string, std::shared_ptr<CdsContainer>> containerMap;
    mutable std::map<int, std::shared_ptr<CdsContainer>> containersWithFanArt;

    mutable std::mutex layoutMutex;
    using LayoutAutoLock = std::scoped_lock<decltype(layoutMutex)>;
    mutable std::shared_ptr<Layout> layout;

    mutable std::mutex cacheMutex;
    using CacheAutoLock = std::scoped_lock<decltype(cacheMutex)>;
#ifdef HAVE_JS
    std::unique_ptr<PlaylistParserScript> playlistParserScript;
    std::unique_ptr<MetafileParserScript> metafileParserScript;
#endif

    mutable std::map<fs::path, std::shared_ptr<ContentState>> contentStateCache = std::map<fs::path, std::shared_ptr<ContentState>>();
    std::error_code ec;
    fs::path activeScan {};

    /// @brief build upnp class based on mime type
    std::string mimeTypeToUpnpClass(const std::string& mimeType) const;
    /// @brief build object titles based on location and upnpClass
    std::string makeTitle(const fs::path& objectPath, const std::string upnpClass) const;

    /// @brief read files from one folder depnending on settings
    void readDir(const fs::path& location, AutoScanSetting settings);
    /// @brief read single file (triggered by autoscan)
    void readFile(const fs::path& location);
    /// \brief create containers for all discovered folders
    void createContainers(int parentContainerId, AutoScanSetting& settings);
    /// \brief create items for all discovered files
    void createItems(AutoScanSetting& settings);
    void updateSingleItem(const fs::directory_entry& dirEntry, const std::shared_ptr<CdsItem>& item, const std::string& mimetype);
    void fillLayout(const std::shared_ptr<GenericTask>& task);
    void updateFanArt(bool isDir);
    /// @brief try to assign fanart to container
    /// @param container target object
    /// @param refObj object to point to for fanart
    /// @param mediaMode media mode for physical folders
    /// @param isDir target is physical folder
    /// @param count current depth of folder path
    /// @param isNew new virtual target
    void assignFanArt(
        const std::shared_ptr<CdsContainer>& container,
        const std::shared_ptr<CdsObject>& refObj,
        AutoscanMediaMode mediaMode,
        bool isDir,
        int count,
        bool isNew);
    void removeHidden(const AutoScanSetting& settings);

    void cacheState(
        const fs::path& entryPath,
        const fs::directory_entry& dirEntry,
        ImportState state,
        std::chrono::seconds mtime = std::chrono::seconds::zero(),
        const std::shared_ptr<CdsObject>& cdsObject = nullptr);

    /// @brief Extract mime type and corresponding upnp class from file
    /// @param objectPath location of the file on the disk
    /// @return pair containing mimetype and upnpclass
    std::tuple<bool, std::string, std::string> getMimeForFile(const fs::path& objectPath) const;

public:
    ImportService(std::shared_ptr<Context> context, std::shared_ptr<ConverterManager> converterManager);

    void run(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> autoScan = {}, fs::path path = "");
    void initLayout(LayoutType layoutType);
    void destroyLayout();

    void doImport(const fs::path& location, AutoScanSetting& settings, std::unordered_set<int>& currentContent, const std::shared_ptr<GenericTask>& task);
    void clearCache();

    std::pair<bool, std::shared_ptr<CdsObject>> createSingleItem(const fs::directory_entry& dirEntry);
    std::shared_ptr<CdsContainer> createSingleContainer(int parentContainerId, const fs::directory_entry& dirEntry, const std::string& upnpClass);

    /// \brief create layout of a single itme
    /// \param state item state
    /// \param object object to create layout
    /// \param parent parent container
    /// \param task import task associated
    void fillSingleLayout(
        const std::shared_ptr<ContentState>& state,
        std::shared_ptr<CdsObject> object,
        const std::shared_ptr<CdsContainer>& parent,
        const std::shared_ptr<GenericTask>& task);

    bool isHiddenFile(const fs::path& entryPath, bool isDirectory, const fs::directory_entry& dirEntry, const AutoScanSetting& settings);

    void updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype);

    /// \brief Adds a virtual container chain specified by path.
    /// \param parentContainerId id of the root container
    /// \param chain list of container objects to create
    /// \param refItem object to take artwork from
    /// \param createdIds of the last container in the chain.
    /// \return last id created
    std::pair<int, bool> addContainerTree(
        int parentContainerId,
        const std::vector<std::shared_ptr<CdsObject>>& chain,
        const std::shared_ptr<CdsObject>& refItem,
        std::vector<int>& createdIds);

    void finishScan(
        const fs::path& location,
        const std::shared_ptr<CdsContainer>& parent,
        std::chrono::seconds lmt,
        const std::shared_ptr<CdsObject>& firstObject = nullptr);

    std::shared_ptr<CdsContainer> getContainer(const std::string& location) const;
    std::shared_ptr<CdsObject> getObject(const fs::path& location) const;
    std::shared_ptr<Layout> getLayout() const { return layout; }
    std::shared_ptr<MetadataService> getMetadataService() const { return metadataService; }

    /// \brief parse a file containing metadata for object
    void parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const;
};

#endif

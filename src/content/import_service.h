/*GRB*

    Gerbera - https://gerbera.io/

    import_service.h - this file is part of Gerbera.

    Copyright (C) 2023 Gerbera Contributors

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
#include <unordered_set>

class AutoScanSetting;
class AutoscanDirectory;
class CdsContainer;
class CdsItem;
class CdsObject;
class Config;
class ContentManager;
class Context;
class Database;
class GenericTask;
class ImportService;
class Layout;
enum class LayoutType;
class Mime;
class ScriptingRuntime;
class UpnpMap;

// forward declarations
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
};

class ContentState {
private:
    ImportState state;
    /// \brief directory_entry to process
    fs::directory_entry dirEntry;
    /// \brief Last modification time in the file system.
    /// In seconds since UNIX epoch.
    std::chrono::seconds mtime {};
    /// \brief CdsObject associated with the directory_entry
    std::shared_ptr<CdsObject> cdsObject;

public:
    ContentState(const fs::directory_entry& dirEntry, ImportState state, std::chrono::seconds mtime = std::chrono::seconds::zero(), std::shared_ptr<CdsObject> cdsObject = nullptr)
        : state(state)
        , dirEntry(dirEntry)
        , mtime(mtime)
        , cdsObject(cdsObject)
    {
    }

    void setObject(ImportState state, std::shared_ptr<CdsObject> cdsObject)
    {
        this->state = state;
        this->cdsObject = cdsObject;
    }
    std::shared_ptr<CdsObject> getObject() { return cdsObject; }
    fs::directory_entry getDirEntry() { return dirEntry; }
    /// \brief Set modification time of file.
    void setMTime(std::chrono::seconds mtime) { this->mtime = mtime; }
    /// \brief Retrieve the file modification time (in seconds since UNIX epoch).
    std::chrono::seconds getMTime() const { return mtime; }
};

class ImportService {
private:
    std::shared_ptr<Context> context;
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<ContentManager> content;

    std::map<std::string, std::string> mimetypeContenttypeMap;
    std::map<std::string, std::string> mimetypeUpnpclassMap;
    std::map<std::string, std::string> configLayoutMapping;
    std::vector<UpnpMap> upnpMap {};

    std::shared_ptr<AutoscanDirectory> autoscanDir;
    fs::path rootPath;
    bool hasReadableNames { false };

    ///\brief cache for containers while creating new layout
    std::map<std::string, std::shared_ptr<CdsContainer>> containerMap;
    std::map<int, std::shared_ptr<CdsContainer>> containersWithFanArt;

    std::mutex layoutMutex;
    std::shared_ptr<Layout> layout;
#ifdef HAVE_JS
    std::unique_ptr<PlaylistParserScript> playlistParserScript;
#endif

    std::map<fs::path, std::shared_ptr<ContentState>> contentStateCache = std::map<fs::path, std::shared_ptr<ContentState>>();

    std::string mimeTypeToUpnpClass(const std::string& mimeType);

    void readDir(std::unique_ptr<fs::directory_iterator>& dirIterator, AutoScanSetting& settings);
    void createContainers();
    void createItems(AutoScanSetting& settings);
    void fillLayout(const std::shared_ptr<GenericTask>& task);
    void assignFanArt(const std::shared_ptr<CdsContainer>& container, const std::shared_ptr<CdsObject>& refObj, int count);

public:
    ImportService(std::shared_ptr<Context> context);

    void run(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> autoScan = {}, fs::path path = "");
    void initLayout(LayoutType layoutType);
    void destroyLayout();

    void doImport(std::unique_ptr<fs::directory_iterator>& dirIterator, AutoScanSetting& settings, std::unordered_set<int>& currentContent, const std::shared_ptr<GenericTask>& task);
    void clearCache();

    std::shared_ptr<CdsObject> createSingleItem(const fs::directory_entry& dirEntry);
    std::shared_ptr<CdsContainer> createSingleContainer(const fs::directory_entry& dirEntry, const std::string& upnpClass);
    void fillSingleLayout(const std::shared_ptr<ContentState>& state, std::shared_ptr<CdsObject> object, const std::shared_ptr<GenericTask>& task);

    void updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype);
    std::pair<int, bool> addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain, std::vector<int>& createdIds);
    void finishScan(const fs::path& location, const std::shared_ptr<CdsContainer>& parent, std::chrono::seconds lmt, const std::shared_ptr<CdsObject>& firstObject = nullptr);
    std::shared_ptr<CdsContainer> getContainer(const fs::path& location) const;

    std::shared_ptr<Layout> getLayout() const { return layout; }
};

#endif

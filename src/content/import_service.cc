/*GRB*

    Gerbera - https://gerbera.io/

    import_service.cc - this file is part of Gerbera.

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

/// \file import_service.cc
#define GRB_LOG_FAC GrbLogFacility::content

#include "import_service.h" // API

#include "autoscan_setting.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content_manager.h"
#include "context.h"
#include "database/database.h"
#include "exceptions.h"
#include "layout/builtin_layout.h"
#include "metadata/metadata_enums.h"
#include "metadata/metadata_handler.h"
#include "metadata/metadata_service.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_JS
#include "layout/js_layout.h"
#include "scripting/import_script.h"
#include "scripting/metafile_parser_script.h"
#include "scripting/playlist_parser_script.h"
#endif

#include <algorithm>
#include <fmt/chrono.h>
#include <regex>

bool UpnpMap::checkValue(const std::string& op, const std::string& expect, const std::string& actual) const
{
    if (op == "=" || op == "==")
        return actual.find(expect) != std::string::npos;
    if (op == "!=")
        return actual.find(expect) == std::string::npos;
    if (op == "<")
        return actual < expect;
    if (op == ">")
        return actual > expect;
    return false;
}

bool UpnpMap::checkValue(const std::string& op, int expect, int actual) const
{
    if (op == "=" || op == "==")
        return actual == expect;
    if (op == "!=")
        return actual != expect;
    if (op == "<")
        return actual < expect;
    if (op == ">")
        return actual > expect;
    return false;
}

bool UpnpMap::isMatch(const std::shared_ptr<CdsItem>& item, const std::string& mt) const
{
    bool match = false;
    if (startswith(mt, mimeType)) {
        for (auto&& [root, op, expect] : filters) {
            if (root == "location") {
                match = checkValue(op, expect, item->getLocation().string());
            } else if (root == "tracknumber") {
                match = checkValue(op, stoiString(expect), item->getTrackNumber());
            } else if (root == "partnumber") {
                match = checkValue(op, stoiString(expect), item->getPartNumber());
            } else if (std::find_if(MetaEnumMapper::mt_keys.begin(), MetaEnumMapper::mt_keys.end(), [val = root](auto&& kvp) { return kvp.second == val; }) != MetaEnumMapper::mt_keys.end()) {
                match = checkValue(op, expect, item->getMetaData(root));
            }
        }
    }
    return match;
}

void UpnpMap::initMap(std::vector<UpnpMap>& target, const std::map<std::string, std::string>& source)
{
    auto re = std::regex("^([A-Za-z0-9_:]+)(<|>|=|==|!=)([A-Za-z0-9_]+)$");
    for (auto&& [key, cls] : source) {
        auto filterList = splitString(key, ';');
        auto filters = std::vector<std::tuple<std::string, std::string, std::string>>();
        auto mt = filterList.front();
        std::vector<std::string> parts = splitString(mt, '/');

        if (parts.size() == 2 && parts.at(1) == "*")
            mt = fmt::format("{}/", parts.at(0));
        filterList.erase(filterList.begin());

        for (auto&& filter : filterList) {
            std::smatch match;
            std::regex_match(filter, match, re);
            if (match.size() == 4) {
                filters.emplace_back(match[1], match[2], match[3]);
            }
        }
        log_vdebug("UpnpMap: {} -> {}", mt, cls);
        target.emplace_back(mt, cls, filters);
    }
}

ContentState::ContentState(fs::directory_entry dirEntry, ImportState state, std::chrono::seconds mtime, std::shared_ptr<CdsObject> cdsObject)
    : state(state)
    , dirEntry(std::move(dirEntry))
    , mtime(mtime)
    , cdsObject(std::move(cdsObject))
{
    itemCounter[ObjectType::Audio] = 0;
    itemCounter[ObjectType::Video] = 0;
    itemCounter[ObjectType::Image] = 0;
    itemCounter[ObjectType::Playlist] = 0;
    itemCounter[ObjectType::Folder] = 0;
    itemCounter[ObjectType::Unknown] = 0;
}

AutoscanMediaMode ContentState::getMediaMode() const
{
    AutoscanMediaMode mediaMode = AutoscanMediaMode::Mixed;
    std::size_t maxValue = 3; // at least 4 items are required to set upnp_class
    if (itemCounter.at(ObjectType::Audio) > maxValue) {
        mediaMode = AutoscanMediaMode::Audio;
        maxValue = itemCounter.at(ObjectType::Audio);
    }
    if (itemCounter.at(ObjectType::Image) > maxValue) {
        mediaMode = AutoscanMediaMode::Image;
        maxValue = itemCounter.at(ObjectType::Image);
    }
    if (itemCounter.at(ObjectType::Video) > maxValue) {
        mediaMode = AutoscanMediaMode::Video;
        maxValue = itemCounter.at(ObjectType::Video);
    }
    return mediaMode;
}

ImportService::ImportService(std::shared_ptr<Context> context, std::shared_ptr<ConverterManager> converterManager)
    : context(std::move(context))
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
    , converterManager(std::move(converterManager))
    , containerTypeMap(AutoscanDirectory::ContainerTypesDefaults)
{
    hasReadableNames = config->getBoolOption(ConfigVal::IMPORT_READABLE_NAMES);
    hasCaseSensitiveNames = config->getBoolOption(ConfigVal::IMPORT_CASE_SENSITIVE_TAGS);
    hasDefaultDate = config->getBoolOption(ConfigVal::IMPORT_DEFAULT_DATE);
    mimetypeContenttypeMap = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    mimetypeUpnpclassMap = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    configLayoutMapping = config->getDictionaryOption(ConfigVal::IMPORT_LAYOUT_MAPPING);
    containerImageParentCount = config->getIntOption(ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT);
    containerImageMinDepth = config->getIntOption(ConfigVal::IMPORT_RESOURCES_CONTAINERART_MINDEPTH);
    virtualDirKeys = config->getVectorOption(ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS);
    noMediaName = config->getOption(ConfigVal::IMPORT_NOMEDIA_FILE);
    UpnpMap::initMap(upnpMap, mimetypeUpnpclassMap);
}

void ImportService::run(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> autoScan, fs::path path)
{
    this->content = std::move(content);
    metadataService = std::make_shared<MetadataService>(context, this->content);
    if (autoScan) {
        this->autoscanDir = std::move(autoScan);
        this->containerTypeMap = this->autoscanDir->getContainerTypes();
        this->rootPath = std::move(path);
        this->pcDirTypes = this->autoscanDir->hasDirTypes();
    }
}

void ImportService::initLayout(LayoutType layoutType)
{
    if (!layout) {
        log_debug("virtual layout Type {}", layoutType);
        try {
            LayoutAutoLock lock(layoutMutex);

            if (layoutType == LayoutType::Js) {
#ifdef HAVE_JS
                layout = std::make_shared<JSLayout>(content, rootPath.string());
#else
                log_error("Cannot init virtual layout:\nGerbera compiled without JavaScript support, but JavaScript was requested.");
#endif
            } else if (layoutType == LayoutType::Builtin) {
                layout = std::make_shared<BuiltinLayout>(content);
            }
        } catch (const std::runtime_error& e) {
            layout = nullptr;
            log_error("Init virtual container layout failed: {}", e.what());
            throw;
        }
    }
#ifdef HAVE_JS
    if (!playlistParserScript) {
        playlistParserScript = std::make_unique<PlaylistParserScript>(content, rootPath.string());
    }
    if (!metafileParserScript) {
        metafileParserScript = std::make_unique<MetafileParserScript>(content, rootPath.string());
    }
#endif
}

void ImportService::destroyLayout()
{
    layout = nullptr;
#ifdef HAVE_JS
    playlistParserScript = nullptr;
    metafileParserScript = nullptr;
#endif
}

std::string ImportService::mimeTypeToUpnpClass(const std::string& mimeType) const
{
    auto it = std::find_if(upnpMap.begin(), upnpMap.end(), [=](auto&& um) { return startswith(mimeType, um.mimeType); });
    if (it != upnpMap.end())
        return it->upnpClass;

    // try direct search
    auto itm = mimetypeUpnpclassMap.find(mimeType);
    if (itm != mimetypeUpnpclassMap.end())
        return itm->second;

    // try pattern search
    std::vector<std::string> parts = splitString(mimeType, '/');
    if (parts.size() != 2)
        return {};

    return getValueOrDefault(mimetypeUpnpclassMap, parts.at(0) + "/*");
}

void ImportService::clearCache()
{
    log_debug("Clearing Cache '{}'", rootPath.c_str());
    containerMap.clear();
    containersWithFanArt.clear();
}

void ImportService::doImport(
    const fs::path& location,
    AutoScanSetting& settings,
    std::unordered_set<int>& currentContent,
    const std::shared_ptr<GenericTask>& task)
{
    log_debug("start {} root '{}' update {}", location.string(), rootPath.string(), !!settings.changedObject);
    if (activeScan.empty()) {
        auto cacheLock = CacheAutoLock(cacheMutex);
        contentStateCache.clear();
        if (settings.changedObject)
            clearCache();
        activeScan = location;
    } else {
        log_debug("Additional scan {}, already active {}", location.c_str(), activeScan.c_str());
    }

    if (!rootPath.empty()) {
        auto rootDirEntry = fs::directory_entry(rootPath);
        cacheState(rootPath, rootDirEntry, ImportState::New, toSeconds(rootDirEntry.last_write_time(ec)));
    }
    auto rootEntry = fs::directory_entry(location, ec);
    if (ec) {
        log_error("Failed to start {}, {}", location.c_str(), ec.message());
        return;
    }
    auto isDir = rootEntry.is_directory(ec);
    if (ec) {
        log_error("Failed to start {}, {}", location.c_str(), ec.message());
        return;
    }

    cacheState(location, rootEntry, ImportState::New, toSeconds(rootEntry.last_write_time(ec)), settings.changedObject);
    if (isDir) {
        readDir(location, settings);
    } else {
        readFile(location);
    }
    removeHidden(settings);
    createContainers(CDS_ID_FS_ROOT, settings);
    createItems(settings);
    updateFanArt(isDir);
    fillLayout(task);

    // update currentContent
    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        if (!stateEntry)
            continue;
        if (stateEntry->getObject() && stateEntry->getState() == ImportState::Existing) {
            auto entry = currentContent.find(stateEntry->getObject()->getID());
            if (entry != currentContent.end()) {
                currentContent.erase(stateEntry->getObject()->getID());
            }
        }
    }
    log_debug("import of {} left {} item(s) to be deleted", location.c_str(), currentContent.size());

    if (!task && autoscanDir && autoscanDir->updateLMT()) {
        log_debug("Updating last_modified for autoscan directory {}", autoscanDir->getLocation().c_str());
        database->updateAutoscanDirectory(autoscanDir);
    }
    if (activeScan == location)
        activeScan = "";
}

std::shared_ptr<CdsObject> ImportService::getObject(const fs::path& location) const
{
    log_debug("start {}", location.string());
    if (contentStateCache.find(location) != contentStateCache.end()) {
        return contentStateCache.at(location)->getObject();
    }
    return {};
}

void ImportService::readDir(const fs::path& location, AutoScanSetting settings)
{
    log_debug("start {}", location.string());
    auto dirIterator = fs::directory_iterator(location, ec);
    if (ec) {
        log_error("Failed to iterate {}, {}", location.c_str(), ec.message());
        return;
    }
    settings.mergeOptions(config, location);
    for (auto&& dirEntry : dirIterator) {
        auto&& entryPath = dirEntry.path();
        if (entryPath.empty() || isHiddenFile(entryPath, true, dirEntry, settings)) {
            continue;
        }
        cacheState(entryPath, dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));
        if (dirEntry.is_directory(ec) && settings.recursive) {
            if (!ec) {
                readDir(entryPath, settings);
            } else {
                cacheState(entryPath, dirEntry, ImportState::Broken);
                log_error("ImportService::readDir {}: Failed to read {}, {}", location.c_str(), entryPath.c_str(), ec.message());
            }
        } else if (ec) {
            cacheState(entryPath, dirEntry, ImportState::Broken);
            log_error("ImportService::readDir {}: Failed to read {}, {}", location.c_str(), entryPath.c_str(), ec.message());
        }
    }
    log_debug("end {}", location.string());
}

void ImportService::cacheState(
    const fs::path& entryPath,
    const fs::directory_entry& dirEntry,
    ImportState state,
    std::chrono::seconds mtime,
    const std::shared_ptr<CdsObject>& cdsObject)
{
    auto cacheLock = CacheAutoLock(cacheMutex);
    log_debug("cache '{}' , '{}' ({})", entryPath.string(), dirEntry.path().string(), state);

    if (entryPath.empty())
        return;

    if (contentStateCache.find(entryPath) == contentStateCache.end()) {
        contentStateCache[entryPath] = std::make_shared<ContentState>(dirEntry, state, mtime, cdsObject);
    } else {
        state = contentStateCache.at(entryPath)->getState() < state ? state : contentStateCache[entryPath]->getState();
        if (!cdsObject)
            contentStateCache.at(entryPath)->setState(state);
        else
            contentStateCache.at(entryPath)->setObject(state, cdsObject);
        if (mtime > std::chrono::seconds::zero())
            contentStateCache.at(entryPath)->setMTime(mtime);
    }
}

void ImportService::readFile(const fs::path& location)
{
    auto dirEntry = fs::directory_entry(location.parent_path());
    cacheState(location.parent_path(), dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));

    auto entryPath = dirEntry.path().parent_path();
    while (entryPath != "/" && !entryPath.empty() && entryPath != rootPath) {
        dirEntry.assign(entryPath, ec);
        if (ec) {
            cacheState(entryPath, dirEntry, ImportState::Broken);
            log_error("ImportService::readFile {}: Failed to navigate up {}, {}", location.c_str(), entryPath.c_str(), ec.message());
            break;
        }
        cacheState(entryPath, dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));
        entryPath = entryPath.parent_path();
    }
}

void ImportService::removeHidden(const AutoScanSetting& settings)
{
    auto hiddenPaths = std::vector<fs::path>();
    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        if (!stateEntry)
            continue;
        auto dirEntry = stateEntry->getDirEntry();
        if (isHiddenFile(itemPath, dirEntry.is_directory(ec), dirEntry, settings)) {
            hiddenPaths.push_back(itemPath);
        }
    }

    {
        auto cacheLock = CacheAutoLock(cacheMutex);
        for (auto&& hiddenPath : hiddenPaths) {
            for (auto it = contentStateCache.begin(); it != contentStateCache.end();) {
                if (startswith(it->first.string(), hiddenPath.string())) {
                    contentStateCache.erase(it++);
                } else {
                    ++it;
                }
            }
        }
    }
}

bool ImportService::isHiddenFile(const fs::path& entryPath, bool isDirectory, const fs::directory_entry& dirEntry, const AutoScanSetting& settings)
{
    auto&& name = entryPath.filename().string();
    if (name.empty())
        return true;
    if ((name.at(0) == '.' && !settings.hidden)
        || (!settings.followSymlinks && dirEntry.is_symlink())
        || config->getConfigFilename() == entryPath) {
        cacheState(entryPath, dirEntry, ImportState::ToDelete);
        log_vdebug("hidden {}", entryPath.string());
        return true;
    }
    if (!noMediaName.empty()) {
        auto noMediaFile = (isDirectory) ? entryPath / noMediaName : entryPath.parent_path() / noMediaName;
        {
            auto cacheLock = CacheAutoLock(cacheMutex);
            if (contentStateCache.find(noMediaFile) != contentStateCache.end()) {
                return !contentStateCache.at(noMediaFile) || contentStateCache.at(noMediaFile)->getState() != ImportState::Broken; // broken means: file not found
            }
        }
        // if file exists it will be automatically created
    }
    return false;
}

///\brief create containers for all discovered folders
void ImportService::createContainers(int parentContainerId, AutoScanSetting& settings)
{
    log_debug("start {} {}", rootPath.string(), parentContainerId);
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (!stateEntry || stateEntry->getState() != ImportState::New)
            continue;
        auto dirEntry = stateEntry->getDirEntry();
        bool doUpdate = false;
        if (dirEntry.exists(ec) && dirEntry.is_directory(ec)) {
            auto cdsObj = stateEntry->getObject();
            if (cdsObj) {
                auto oldLocation = cdsObj->getLocation();
                cdsObj->setLocation(contPath);
                log_debug("Container renamed {} {}", cdsObj->getTitle(), contPath.filename().string());
                cdsObj->setTitle(contPath.filename().string());
                doUpdate = true;
                log_debug("Container moved {} {}", oldLocation.string(), contPath.string());
                for (auto&& [childPath, childEntry] : contentStateCache) {
                    // find direct folder entry with old name and rely on iteration to rename hierarchy
                    if (childEntry && childPath.parent_path() == contPath) {
                        auto childObj = database->findObjectByPath(oldLocation / childPath.filename(), UNUSED_CLIENT_GROUP, DbFileType::Any);
                        if (childObj)
                            childEntry->setObject(ImportState::New, childObj);
                    }
                }
            }
            if (!cdsObj)
                cdsObj = database->findObjectByPath(contPath, UNUSED_CLIENT_GROUP, DbFileType::Directory);

            if (cdsObj) {
                try {
                    std::shared_ptr<CdsObject> container = std::dynamic_pointer_cast<CdsContainer>(cdsObj);
                    if (!container || !cdsObj->isContainer()) {
                        throw_std_runtime_error("Object {} is not a container", cdsObj->getLocation().string());
                    }
                    auto lastModified = toSeconds(dirEntry.last_write_time(ec));
                    if (lastModified > container->getMTime()) {
                        container->setMTime(lastModified);
                        doUpdate = true;
                    }
                    if (doUpdate) {
                        database->updateObject(cdsObj, nullptr);
                        stateEntry->setObject(ImportState::Created, cdsObj);
                        log_debug("Container updated {} {}", contPath.string(), container->getID());
                    } else {
                        stateEntry->setObject(ImportState::Existing, cdsObj);
                        log_debug("Container found {} {}", contPath.string(), container->getID());
                    }
                } catch (const std::runtime_error& e) {
                    stateEntry->setObject(ImportState::Broken, cdsObj);
                    log_error("createContainers: Failed to load parent container {}, {}", contPath.c_str(), e.what());
                }
            } else {
                // Create container
                stateEntry->setObject(ImportState::Created, createSingleContainer(parentContainerId, dirEntry, UPNP_CLASS_CONTAINER_FOLDER));
            }
        }
    }
    log_debug("end {}", rootPath.string());
}

std::tuple<bool, std::string, std::string> ImportService::getMimeForFile(const fs::path& objectPath) const
{
    /* retrieve information about item and decide if it should be included */
    auto [skip, mimetype] = mime->getMimeType(objectPath, MIMETYPE_DEFAULT);
    if (mimetype.empty()) {
        if (skip)
            log_debug("Mime set empty for file {}", objectPath.c_str());
        else
            log_error("Mime not found for file {}", objectPath.c_str());
        return { skip, "", "" };
    }
    log_debug("Mime '{}' for file {}", mimetype, objectPath.c_str());

    std::string upnpClass = mimeTypeToUpnpClass(mimetype);
    if (upnpClass.empty()) {
        std::string contentType = getValueOrDefault(mimetypeContenttypeMap, mimetype);
        if (contentType == CONTENT_TYPE_OGG) {
            upnpClass = isTheora(objectPath)
                ? UPNP_CLASS_VIDEO_ITEM
                : UPNP_CLASS_MUSIC_TRACK;
        }
    }
    log_debug("UpnpClass '{}' for file {}", upnpClass, objectPath.c_str());

    return { skip, mimetype, upnpClass };
}

///\brief create items for all discovered files
void ImportService::createItems(AutoScanSetting& settings)
{
    log_debug("start {}", rootPath.string());
    std::shared_ptr<CdsContainer> parentContainer = nullptr;
    auto lastModifiedCurrentMax = std::chrono::seconds::zero();
    auto lastModifiedNewMax = lastModifiedCurrentMax;
    fs::path contPath;

    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        if (!stateEntry) {
            log_debug("broken entry {}", itemPath.string());
            continue;
        }
        auto cdsObj = stateEntry->getObject();
        // cache containers as parent item for following item
        if (cdsObj && cdsObj->isContainer()) {
            std::shared_ptr<CdsContainer> container = std::dynamic_pointer_cast<CdsContainer>(cdsObj);
            if (!contPath.empty()) {
                contentStateCache.at(contPath)->setMTime(lastModifiedNewMax);
                if (autoscanDir) {
                    autoscanDir->setCurrentLMT(contPath, lastModifiedNewMax);
                }
            }
            parentContainer = container;
            contPath = itemPath;
            if (autoscanDir) {
                lastModifiedCurrentMax = autoscanDir->getPreviousLMT(contPath, parentContainer);
                lastModifiedNewMax = lastModifiedCurrentMax;
                autoscanDir->setCurrentLMT(contPath, std::chrono::seconds::zero());
            }
        }
        if (stateEntry->getState() != ImportState::New) {
            log_debug("wrong state entry {}", itemPath.string());
            continue;
        }
        // create items from files
        auto dirEntry = stateEntry->getDirEntry();
        if (isRegularFile(dirEntry, ec)) {
            // Start with cached item
            auto contState = contentStateCache.at(itemPath.parent_path());
            if (contState)
                parentContainer = std::dynamic_pointer_cast<CdsContainer>(contState->getObject());
            else
                log_error("No Container parent for Item {}", itemPath.string());

            if (!cdsObj) {
                // Search item in database
                log_debug("Searching Item {} in database", itemPath.string());
                cdsObj = database->findObjectByPath(itemPath, UNUSED_CLIENT_GROUP, DbFileType::File);
            }
            if (cdsObj && cdsObj->isItem()) {
                auto isChanged = stateEntry->getMTime() != cdsObj->getMTime() || cdsObj->getLocation().string() != dirEntry.path().string();
                if (autoscanDir && autoscanDir->getForceRescan())
                    isChanged = isChanged || cdsObj->getClass().empty() || cdsObj->getClass() == UPNP_CLASS_ITEM;
                if (isChanged) {
                    // Update changed item in database
                    log_debug("Updating Item {} in database {}", itemPath.string(), cdsObj->getID());
                    auto item = std::dynamic_pointer_cast<CdsItem>(cdsObj);
                    if (item->getMimeType().empty() || item->getClass().empty() || item->getClass() == UPNP_CLASS_ITEM) {
                        auto [skip, mimetype, upnpClass] = getMimeForFile(itemPath);
                        if (!mimetype.empty()) {
                            item->setMimeType(mimetype);
                        }
                        if (!upnpClass.empty()) {
                            item->setClass(upnpClass);
                        }
                        log_debug("Updating Item properties {} in database: skip {} mimeType {}, upnpClass {}", skip, itemPath.string(), mimetype, upnpClass);
                    }
                    item->clearMetaData();
                    item->clearAuxData();
                    item->clearResources();
                    log_debug("Changing location {} to {}", item->getLocation().string(), itemPath.string());
                    item->setLocation(itemPath);
                    item->setTitle(makeTitle(itemPath, item->getClass()));
                    updateSingleItem(dirEntry, item, item->getMimeType());
                    if (lastModifiedNewMax < cdsObj->getMTime())
                        lastModifiedNewMax = cdsObj->getMTime();
                    database->updateObject(cdsObj, nullptr);
                    stateEntry->setObject(ImportState::Created, cdsObj);
                    log_debug("Item changed {} {}", itemPath.string(), cdsObj->getID());
                } else {
                    // Store local item with updated status
                    if (contState && contState->getMTime() < cdsObj->getMTime()) {
                        contState->setMTime(cdsObj->getMTime());
                        if (lastModifiedNewMax < cdsObj->getMTime())
                            lastModifiedNewMax = cdsObj->getMTime();
                    }
                    stateEntry->setObject(ImportState::Existing, cdsObj);
                    log_debug("Item found {} {}", itemPath.string(), cdsObj->getID());
                }
            } else {
                // Create item from scratch
                log_debug("Creating Item {}", itemPath.string());
                auto [skip, newCdsObj] = createSingleItem(dirEntry);
                if (newCdsObj) {
                    cdsObj = newCdsObj;
                    if (contState) {
                        contState->setMTime(cdsObj->getMTime());
                        if (lastModifiedNewMax < cdsObj->getMTime())
                            lastModifiedNewMax = cdsObj->getMTime();
                    }
                    stateEntry->setObject(ImportState::Created, cdsObj);
                    cdsObj->setParentID(parentContainer ? parentContainer->getID() : INVALID_OBJECT_ID);
                    database->addObject(cdsObj, nullptr);
                } else {
                    stateEntry->setObject(ImportState::Broken, cdsObj);
                    cdsObj = nullptr;
                    if (!skip)
                        log_error("Object not created for file {}", dirEntry.path().string());
                }
            }
            if (contState && cdsObj) {
                contState->increaseItemCounter(cdsObj->getMediaType());
                contState->setFirstObject(cdsObj);
            }
            if (parentContainer) {
                stateEntry->setParentObject(parentContainer);
            }
        } else {
            log_debug("Not a file {}", itemPath.string());
        }
    }
    if (autoscanDir && contPath != "") {
        autoscanDir->setCurrentLMT(contPath, lastModifiedNewMax);
    }
    log_debug("end {}", rootPath.string());
}

std::pair<bool, std::shared_ptr<CdsObject>> ImportService::createSingleItem(const fs::directory_entry& dirEntry) // ToDo: Use StateEntry here
{
    const auto& objectPath = dirEntry.path();
    auto [skip, mimetype, upnpClass] = getMimeForFile(objectPath);
    if (mimetype.empty() && upnpClass.empty()) {
        return { skip, nullptr };
    }
    auto item = std::make_shared<CdsItem>();
    item->setLocation(objectPath);

    if (!mimetype.empty()) {
        item->setMimeType(mimetype);
    }
    if (!upnpClass.empty()) {
        item->setClass(upnpClass);
    }

    item->setTitle(makeTitle(objectPath, upnpClass));
    updateSingleItem(dirEntry, item, mimetype);

    return { skip, item };
}

std::string ImportService::makeTitle(const fs::path& objectPath, const std::string upnpClass) const
{
    auto f2i = converterManager->f2i();
    std::string title = objectPath.filename().string();
    if (hasReadableNames && upnpClass != UPNP_CLASS_ITEM) {
        title = objectPath.stem().string();
        if (title.length() > 1)
            std::replace(title.begin() + 1, title.end() - 1, '_', ' ');
    }
    auto [mval, err] = f2i->convert(title);
    if (!err.empty()) {
        log_warning("{}: {}", title, err);
    }
    return mval;
}

void ImportService::updateSingleItem(const fs::directory_entry& dirEntry, const std::shared_ptr<CdsItem>& item, const std::string& mimetype)
{
    auto mTime = toSeconds(dirEntry.last_write_time(ec));
    item->setMTime(mTime);
    item->setUTime(mTime);
    item->setSizeOnDisk(getFileSize(dirEntry));

    try {
        metadataService->extractMetaData(item, dirEntry);
        updateItemData(item, mimetype);
    } catch (const std::runtime_error& ex) {
        log_error("updateSingleItem '{}' failed: {}", dirEntry.path().string(), ex.what());
    }
}

void ImportService::fillLayout(const std::shared_ptr<GenericTask>& task)
{
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (!stateEntry || stateEntry->getState() != ImportState::Created)
            continue;
        contentStateCache.at(contPath)->setObject(ImportState::Loaded, stateEntry->getObject());
        fillSingleLayout(stateEntry, nullptr, stateEntry->getParentObject(), task);
    }
}

/// \param object used to make code compatible with legacy scan
void ImportService::fillSingleLayout(const std::shared_ptr<ContentState>& state,
    std::shared_ptr<CdsObject> object,
    const std::shared_ptr<CdsContainer>& parent,
    const std::shared_ptr<GenericTask>& task)
{
    std::shared_ptr<CdsObject> cdsObject = state ? state->getObject() : std::move(object);
    log_debug("cds {}, layout {}, autoscanDir {}", !!cdsObject, !!layout, !!autoscanDir);

    if (cdsObject && cdsObject->isItem() && layout) {
        try {
            std::string mimetype = std::static_pointer_cast<CdsItem>(cdsObject)->getMimeType();
            std::string contentType = getValueOrDefault(mimetypeContenttypeMap, mimetype);
            log_vdebug("mimetype {}, contentype {}, autoscanDir {}", mimetype, contentType, (!autoscanDir || autoscanDir->hasContent(cdsObject->getClass())));

            if (contentType == CONTENT_TYPE_PLAYLIST) {
#ifdef HAVE_JS
                try {
                    if (playlistParserScript)
                        playlistParserScript->processPlaylistObject(cdsObject, task, rootPath);
                } catch (const std::runtime_error& e) {
                    log_error("{}", e.what());
                }
#else
                log_warning("Playlist {} will not be parsed: Gerbera was compiled without JS support!", cdsObject->getLocation().c_str());
#endif // HAVE_JS
            } else if (!autoscanDir || autoscanDir->hasContent(cdsObject->getClass())) {
                // only lock mutex while processing item layout
                LayoutAutoLock lock(layoutMutex);
                // get ref'd objects with last mod time
                auto refObjects = state ? database->getRefObjects(cdsObject->getID()) : std::vector<int> {};
                layout->processCdsObject(cdsObject, parent,
                    rootPath, contentType,
                    containerTypeMap,
                    refObjects);
            } else {
                log_debug("File ignored: {} autoscanDir={}, class={}, hasContent={}, mediaType={}", cdsObject->getLocation().string(), !!autoscanDir, cdsObject->getClass(), autoscanDir ? autoscanDir->hasContent(cdsObject->getClass()) : false, autoscanDir ? autoscanDir->getMediaType() : -2);
            }

        } catch (const std::runtime_error& ex) {
            log_error("{}", ex.what());
        }
    }
}

void ImportService::parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const
{
#ifdef HAVE_JS
    try {
        if (metafileParserScript)
            metafileParserScript->processObject(obj, path);
    } catch (const std::runtime_error& e) {
        log_error("{}: {}", path.string(), e.what());
    }
#else
    log_warning("Metadata file {} will not be parsed: Gerbera was compiled without JS support!", path.string());
#endif // HAVE_JS
}

void ImportService::updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype)
{
    if (hasDefaultDate && item->getMetaData(MetadataFields::M_DATE).empty())
        item->addMetaData(MetadataFields::M_DATE, fmt::format("{:%FT%T%z}", fmt::localtime(item->getMTime().count())));
    for (auto&& upnpPattern : upnpMap) {
        if (upnpPattern.isMatch(item, mimetype)) {
            item->setClass(upnpPattern.upnpClass);
            log_debug("UpnpClass '{}' for file {} from pattern {}", upnpPattern.upnpClass, item->getLocation().string(), upnpPattern.mimeType);
        }
    }
}

void ImportService::finishScan(
    const fs::path& location,
    const std::shared_ptr<CdsContainer>& parent,
    std::chrono::seconds lmt,
    const std::shared_ptr<CdsObject>& firstObject)
{
    if (autoscanDir) {
        autoscanDir->setCurrentLMT(location, lmt > std::chrono::seconds::zero() ? lmt : std::chrono::seconds(1));
        if (parent && lmt > std::chrono::seconds::zero()) {
            parent->setMTime(lmt);
            int count = 0;
            if (firstObject) {
                auto objectLocation = firstObject->getLocation(); // ensure same object is used
                count = std::distance(objectLocation.begin(), objectLocation.end()) - std::distance(location.begin(), location.end());
            }
            database->updateObject(parent, nullptr);
            assignFanArt(parent,
                firstObject,
                AutoscanMediaMode::Mixed,
                false /* isDir */,
                count,
                false /* isNew */);
        }
    }
}

void ImportService::assignFanArt(
    const std::shared_ptr<CdsContainer>& container,
    const std::shared_ptr<CdsObject>& refObj,
    AutoscanMediaMode mediaMode,
    bool isDir,
    int count,
    bool isNew)
{
    if (!container)
        return;

    bool doUpdate = false;
    if (refObj) {
        if (isDir && pcDirTypes && refObj->isItem() && container->getClass() != containerTypeMap.at(mediaMode)) {
            container->setClass(containerTypeMap.at(mediaMode));
            doUpdate = true;
        }
        if (refObj->getMTime() > container->getMTime()) {
            container->setMTime(refObj->getMTime());
            doUpdate = true;
        }
    }

    if (containersWithFanArt.find(container->getID()) != containersWithFanArt.end()) {
        log_debug("Already {} assigned fanart {}", container->getLocation().string(), container->getID());
        if (doUpdate)
            database->updateObject(container, nullptr);
        return;
    }

    auto fanart = container->getResource(ResourcePurpose::Thumbnail);
    if (fanart && fanart->getHandlerType() != ContentHandler::CONTAINERART) {
        // remove stale references
        auto fanartObjId = stoiString(fanart->getAttribute(ResourceAttribute::FANART_OBJ_ID));
        try {
            if (fanartObjId > CDS_ID_ROOT) {
                log_debug("Object {} assigned fanart {}", container->getLocation().string(), container->getID());
                database->loadObject(fanartObjId);
            }
        } catch (const ObjectNotFoundException&) {
            log_warning("Object fanart {} broken {}", container->getLocation().string(), fanartObjId);
            container->removeResource(fanart->getHandlerType());
            doUpdate = true;
            fanart = nullptr;
        }
    }
    if (!fanart || isNew || fanart->getHandlerType() != ContentHandler::CONTAINERART) {
        if (fanart) {
            container->clearResources();
            doUpdate = true;
        }
        metadataService->getHandler(ContentHandler::CONTAINERART)->fillMetadata(container);
        auto containerart = container->getResource(ResourcePurpose::Thumbnail);
        if (containerart) {
            fanart = containerart;
        } else if (fanart) {
            container->addResource(fanart); // restore if no image matches
        }
        log_debug("fanart {} from dir {}", container->getLocation().string(), containerart != nullptr);
    }

    if (refObj && !fanart) {
        auto location = container->getLocation();
        if (refObj->isContainer() || (count < containerImageParentCount && container->getParentID() != CDS_ID_ROOT && std::distance(location.begin(), location.end()) > containerImageMinDepth)) {
            auto refFanArt = refObj->getResource(ResourcePurpose::Thumbnail);
            if (refFanArt) {
                auto fanArtObj = refObj;
                auto fanArtObjId = refObj->getID();
                auto fanArtResId = refFanArt->getResId();
                if (refObj->getID() <= CDS_ID_ROOT) {
                    fanArtObj = database->loadObject(refObj->getRefID());
                    auto fanartRef = fanArtObj->getResource(ResourcePurpose::Thumbnail);
                    fanArtResId = fanartRef->getResId();
                }
                if (fanArtObj->isItem() && fanArtResId == 0) {
                    for (auto&& res : fanArtObj->getResources()) {
                        if (res->getPurpose() == ResourcePurpose::Thumbnail)
                            break;
                        fanArtResId++;
                    }
                }
                if (refFanArt->getAttribute(ResourceAttribute::RESOURCE_FILE).empty()) {
                    if (fanArtObjId > CDS_ID_ROOT) {
                        refFanArt->addAttribute(ResourceAttribute::FANART_OBJ_ID, fmt::to_string(fanArtObjId));
                        refFanArt->addAttribute(ResourceAttribute::FANART_RES_ID, fmt::to_string(fanArtResId));
                        fanart = refFanArt;
                    }
                } else {
                    fanart = refFanArt;
                }
                log_debug("fanart {} from ref {}", location.string(), fanart != nullptr);
            }
        }
    }
    if (fanart) {
        container->clearResources();
        container->addResource(fanart); // overwrite all other resources
        doUpdate = true;
        containersWithFanArt[container->getID()] = container;
    }
    if (doUpdate)
        database->updateObject(container, nullptr);
}

std::shared_ptr<CdsContainer> ImportService::getContainer(const std::string& location) const
{
    if (!hasCaseSensitiveNames) {
        auto llocation = toLower(location);
        return containerMap.at(llocation);
    }
    return containerMap.at(location);
}

std::shared_ptr<CdsContainer> ImportService::createSingleContainer(
    int parentContainerId,
    const fs::directory_entry& dirEntry,
    const std::string& upnpClass)
{
    std::vector<std::shared_ptr<CdsObject>> cVec;
    auto location = dirEntry.path();
    for (auto&& segment : location) {
        if (segment != "/")
            cVec.push_back(std::make_shared<CdsContainer>(segment.string(), upnpClass));
    }
    std::vector<int> createdIds;
    addContainerTree(parentContainerId, cVec, nullptr, createdIds);

    if (containerMap.find(location) != containerMap.end()) {
        auto result = containerMap.at(location);
        result->setMTime(toSeconds(dirEntry.last_write_time(ec)));
        return result;
    }
    return {};
}

/// \param createdIds used by messaging in ContentManager
std::pair<int, bool> ImportService::addContainerTree(
    int parentContainerId,
    const std::vector<std::shared_ptr<CdsObject>>& chain,
    const std::shared_ptr<CdsObject>& refItem,
    std::vector<int>& createdIds)
{
    log_debug("start '{}' {}", rootPath.string(), parentContainerId);
    std::string tree; // accumulate path to container here
    int result = parentContainerId;
    bool isNew = false;
    bool isVirtual = parentContainerId != CDS_ID_FS_ROOT;
    int count = 0;
    for (auto&& item : chain) {
        std::string subTree;
        if (item->getTitle().empty()) {
            log_error("Received chain item without title");
            return { INVALID_OBJECT_ID, false };
        }
        tree = fmt::format("{}{}{}", tree, VIRTUAL_CONTAINER_SEPARATOR, escape(item->getTitle(), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR));
        log_debug("Received container chain item {}", tree);
        if (isVirtual) {
            for (auto&& [key, val] : configLayoutMapping) {
                tree = std::regex_replace(tree, std::regex(key), val);
            }
            auto dirKeyValues = std::vector<std::string>();
            for (auto&& vdirSetting : virtualDirKeys) {
                std::string field;
                std::string upnpClass;
                for (auto&& [key, vdirField] : vdirSetting) {
                    if (key == "metadata")
                        field = vdirField;
                    if (key == "class")
                        upnpClass = vdirField;
                }
                if (!item->isSubClass(upnpClass))
                    continue;
                if (field == "LOCATION") {
                    std::string location = item->getLocation().c_str();
                    if (!location.empty()) {
                        dirKeyValues.push_back(location);
                        item->setLocation("");
                    }
                } else if (endswith(field, "_1")) {
                    auto metaField = MetaEnumMapper::remapMetaDataField(field.replace(field.end() - 2, field.end(), ""));
                    auto keyValue = item->getMetaData(metaField);
                    if (!keyValue.empty())
                        dirKeyValues.push_back(keyValue);
                } else if (!field.empty()) {
                    auto metaField = MetaEnumMapper::remapMetaDataField(field);
                    auto keyValueGroup = item->getMetaGroup(metaField);
                    if (!keyValueGroup.empty())
                        for (auto&& keyValue : keyValueGroup)
                            dirKeyValues.push_back(keyValue);
                }
            }
            if (!dirKeyValues.empty()) {
                subTree = fmt::format("{}@{}", tree, fmt::join(dirKeyValues, "@"));
            } else {
                subTree = tree;
            }
            if (!hasCaseSensitiveNames) {
                subTree = toLower(subTree);
            }
            if (containerMap.find(subTree) == containerMap.end() || !containerMap.at(subTree)) {
                auto cont = database->findObjectByPath(subTree, UNUSED_CLIENT_GROUP, DbFileType::Virtual);
                if (cont && cont->isContainer())
                    containerMap[subTree] = std::dynamic_pointer_cast<CdsContainer>(cont);
                else
                    containerMap[subTree] = nullptr;
                log_debug("Loaded container chain item {} -> {}", subTree, cont && cont->isContainer());
            }
        } else if (subTree.empty()) {
            subTree = tree;
        }
        if (containerMap.find(subTree) == containerMap.end() || !containerMap.at(subTree)) {
            item->removeMetaData(MetadataFields::M_TITLE);
            item->addMetaData(MetadataFields::M_TITLE, item->getTitle());
            item->setParentID(result);
            auto cont = std::dynamic_pointer_cast<CdsContainer>(item);
            cont->setVirtual(isVirtual);
            log_debug("Creating container chain item {} virtual {}", subTree, cont->isVirtual());
            if (database->addContainer(result, subTree, cont, &result)) {
                createdIds.push_back(result);
            }
            auto container = std::dynamic_pointer_cast<CdsContainer>(database->loadObject(result));
            containerMap[subTree] = container;
            if (item->getMTime() > container->getMTime()) {
                createdIds.push_back(result); // ensure update
            }
            isNew = true;
            if (item->isContainer()) {
                std::shared_ptr<CdsContainer> itemContainer = std::dynamic_pointer_cast<CdsContainer>(item);
                container->setUpnpShortcut(itemContainer->getUpnpShortcut());
            }
        } else {
            result = containerMap.at(subTree)->getID();
            if (item->getMTime() > containerMap.at(subTree)->getMTime()) {
                createdIds.push_back(result);
            }
        }
        count++;
        if (isVirtual) // && chain.size() - count < containerImageParentCount
            assignFanArt(containerMap.at(subTree),
                refItem && count > containerImageMinDepth ? refItem : item,
                AutoscanMediaMode::Mixed,
                false /* isDir */,
                chain.size() - count,
                isNew);
    }
    log_debug("end '{}' {} {}", tree, result, isNew);
    return { result, isNew };
}

void ImportService::updateFanArt(bool isDir)
{
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (!stateEntry || !stateEntry->getObject() || !stateEntry->getObject()->isContainer())
            continue;
        std::shared_ptr<CdsContainer> container = std::dynamic_pointer_cast<CdsContainer>(stateEntry->getObject());
        assignFanArt(container,
            stateEntry->getFirstObject(),
            stateEntry->getMediaMode(),
            isDir,
            1,
            false /* isNew */);
    }
}

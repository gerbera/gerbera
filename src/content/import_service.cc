/*GRB*

    Gerbera - https://gerbera.io/

    import_service.cc - this file is part of Gerbera.

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

/// \file import_service.cc
#define LOG_FAC log_facility_t::content

#include "import_service.h" // API

#include <fmt/chrono.h>
#include <regex>

#include "autoscan.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "content_manager.h"
#include "database/database.h"
#include "layout/builtin_layout.h"
#include "upnp_common.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_JS
#include "layout/js_layout.h"
#include "scripting/import_script.h"
#include "scripting/playlist_parser_script.h"
#endif

bool UpnpMap::checkValue(const std::string& op, const std::string& expect, const std::string& actual) const
{
    if (op == "=" && actual.find(expect) != std::string::npos)
        return true;
    if (op == "!=" && actual.find(expect) == std::string::npos)
        return true;
    if (op == "<" && actual < expect)
        return true;
    return (op == ">" && actual > expect);
}

bool UpnpMap::checkValue(const std::string& op, int expect, int actual) const
{
    if (op == "=" && actual != expect)
        return true;
    if (op == "!=" && actual == expect)
        return true;
    if (op == "<" && actual < expect)
        return true;
    return (op == ">" && actual > expect);
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
            } else if (std::find_if(MetadataHandler::mt_keys.begin(), MetadataHandler::mt_keys.end(), [val = root](auto&& kvp) { return kvp.second == val; }) != MetadataHandler::mt_keys.end()) {
                match = checkValue(op, expect, item->getMetaData(root));
            }
        }
    }
    return match;
}

void UpnpMap::initMap(std::vector<UpnpMap>& target, const std::map<std::string, std::string>& source)
{
    auto re = std::regex("^([A-Za-z0-9_:]+)(<|>|=|!=)([A-Za-z0-9_]+)$");
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
        log_debug("UpnpMap: {} -> {}", mt, cls);
        target.emplace_back(mt, cls, filters);
    }
}

ImportService::ImportService(std::shared_ptr<Context> context)
    : context(std::move(context))
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
{
    hasReadableNames = config->getBoolOption(CFG_IMPORT_READABLE_NAMES);
    hasDefaultDate = config->getBoolOption(CFG_IMPORT_DEFAULT_DATE);
    mimetypeContenttypeMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    mimetypeUpnpclassMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    configLayoutMapping = config->getDictionaryOption(CFG_IMPORT_LAYOUT_MAPPING);
    noMediaName = config->getOption(CFG_IMPORT_NOMEDIA_FILE);
    UpnpMap::initMap(upnpMap, mimetypeUpnpclassMap);
}

void ImportService::run(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> autoScan, fs::path path)
{
    this->content = std::move(content);
    if (autoScan) {
        this->autoscanDir = std::move(autoScan);
        this->rootPath = path;
    }
}

void ImportService::initLayout(LayoutType layoutType)
{
    if (!layout) {
        log_debug("virtual layout Type {}", layoutType);
        try {
            std::scoped_lock<decltype(layoutMutex)> lock(layoutMutex);

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
#endif
}

void ImportService::destroyLayout()
{
    layout = nullptr;
#ifdef HAVE_JS
    playlistParserScript = nullptr;
#endif
}

std::string ImportService::mimeTypeToUpnpClass(const std::string& mimeType)
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

    return getValueOrDefault(mimetypeUpnpclassMap, parts[0] + "/*");
}

void ImportService::clearCache()
{
    containerMap.clear();
    containersWithFanArt.clear();
}

void ImportService::doImport(const fs::path& location, AutoScanSetting& settings, std::unordered_set<int>& currentContent, const std::shared_ptr<GenericTask>& task)
{
    log_debug("start {}", location.string());
    contentStateCache.clear();

    auto rootEntry = fs::directory_entry(location);
    contentStateCache[location] = std::make_shared<ContentState>(rootEntry, ImportState::New, toSeconds(rootEntry.last_write_time(ec)));
    if (ec) {
        log_error("Failed to start {}, {}", location.c_str(), ec.message());
        return;
    }
    auto isDir = rootEntry.is_directory(ec);
    if (ec) {
        log_error("Failed to start {}, {}", location.c_str(), ec.message());
        return;
    }

    if (isDir) {
        readDir(location, settings);
    } else {
        readFile(location);
    }
    removeHidden(settings);
    createContainers(CDS_ID_FS_ROOT, settings);
    createItems(settings);
    updateFanArt();
    fillLayout(task);

    // update currentContent
    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        if (stateEntry && stateEntry->getObject() && stateEntry->getState() == ImportState::Existing) {
            auto entry = currentContent.find(stateEntry->getObject()->getID());
            if (entry != currentContent.end()) {
                currentContent.erase(stateEntry->getObject()->getID());
            }
        }
    }
    log_debug("import of {} left {} item(s) to be deleted", location.c_str(), currentContent.size());
}

std::shared_ptr<CdsObject> ImportService::getObject(const fs::path& location) const
{
    if (contentStateCache.find(location) != contentStateCache.end()) {
        return contentStateCache.at(location)->getObject();
    }
    return {};
}

void ImportService::readDir(const fs::path& location, AutoScanSetting& settings)
{
    log_debug("start {}", location.string());
    auto dirIterator = fs::directory_iterator(location, ec);
    if (ec) {
        log_error("Failed to iterate {}, {}", location.c_str(), ec.message());
        return;
    }
    for (auto&& dirEntry : dirIterator) {
        auto&& entryPath = dirEntry.path();
        if (isHiddenFile(entryPath, true, dirEntry, settings)) {
            continue;
        }
        contentStateCache[entryPath] = std::make_shared<ContentState>(dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));
        if (dirEntry.is_directory(ec) && settings.recursive) {
            if (!ec) {
                readDir(entryPath, settings);
            } else {
                contentStateCache[entryPath]->setObject(ImportState::Broken, nullptr);
            }
        } else if (ec) {
            contentStateCache[entryPath]->setObject(ImportState::Broken, nullptr);
        }
    }
    log_debug("end {}", location.string());
}

void ImportService::readFile(const fs::path& location)
{
    auto dirEntry = fs::directory_entry(location.parent_path());
    auto entryPath = dirEntry.path();
    while (entryPath != "/" && entryPath != rootPath) {
        contentStateCache[entryPath] = std::make_shared<ContentState>(dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));
        dirEntry.assign(entryPath.parent_path(), ec);
        if (ec) {
            contentStateCache[entryPath]->setObject(ImportState::Broken, nullptr);
            log_error("Failed to navigate up {}, {}", entryPath.c_str(), ec.message());
            break;
        }
        entryPath = dirEntry.path();
    }
}

void ImportService::removeHidden(AutoScanSetting& settings)
{
    auto hiddenPaths = std::vector<fs::path>();
    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        auto dirEntry = stateEntry->getDirEntry();
        if (isHiddenFile(itemPath, dirEntry.is_directory(ec), dirEntry, settings)) {
            hiddenPaths.push_back(itemPath);
        }
    }

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

bool ImportService::isHiddenFile(const fs::path& entryPath, bool isDirectory, const fs::directory_entry& dirEntry, AutoScanSetting& settings)
{
    auto&& name = entryPath.filename().string();
    if ((name[0] == '.' && !settings.hidden)
        || (!settings.followSymlinks && dirEntry.is_symlink())
        || config->getConfigFilename() == entryPath) {
        contentStateCache[entryPath] = std::make_shared<ContentState>(dirEntry, ImportState::ToDelete);
        return true;
    }
    if (!noMediaName.empty()) {
        auto noMediaFile = (isDirectory) ? entryPath / noMediaName : entryPath.parent_path() / noMediaName;
        if (contentStateCache.find(noMediaFile) != contentStateCache.end()) {
            return contentStateCache[entryPath]->getState() != ImportState::Broken; // broken means: file not found
        } else {
            auto noMediaEntry = fs::directory_entry(noMediaFile, ec);
            if (!noMediaEntry.exists(ec) || ec) {
                contentStateCache[noMediaEntry] = std::make_shared<ContentState>(noMediaEntry, ImportState::Broken, toSeconds(dirEntry.last_write_time(ec)));
            } else {
                // if file exists it will be automatically created
                return true;
            }
        }
    }
    return false;
}

void ImportService::createContainers(int parentContainerId, AutoScanSetting& settings)
{
    log_debug("start {} {}", rootPath.string(), parentContainerId);
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (stateEntry->getState() != ImportState::New)
            continue;
        auto dirEntry = stateEntry->getDirEntry();
        if (dirEntry.exists(ec) && dirEntry.is_directory(ec)) {
            auto cdsObj = database->findObjectByPath(contPath, DbFileType::Directory);

            if (cdsObj) {
                try {
                    std::shared_ptr<CdsObject> container = std::dynamic_pointer_cast<CdsContainer>(cdsObj);
                    if (!container || !cdsObj->isContainer()) {
                        throw_std_runtime_error("Object {} is not a container", cdsObj->getLocation().string());
                    }
                    stateEntry->setObject(ImportState::Existing, container);
                    auto lastModified = toSeconds(dirEntry.last_write_time(ec));
                    if (lastModified > container->getMTime()) {
                        container->setMTime(lastModified);
                    }
                    log_debug("Container found {} {}", contPath.string(), container->getID());

                } catch (const std::runtime_error& e) {
                    stateEntry->setObject(ImportState::Broken, cdsObj);
                    log_error("createContainers: Failed to load parent container {}, {}", contPath.c_str(), e.what());
                }
            } else {
                // Create container
                stateEntry->setObject(ImportState::Created, createSingleContainer(parentContainerId, dirEntry, UPNP_CLASS_CONTAINER));
            }
        }
    }
    log_debug("end {}", rootPath.string());
}

void ImportService::createItems(AutoScanSetting& settings)
{
    log_debug("start {}", rootPath.string());
    std::shared_ptr<CdsContainer> parentContainer = nullptr;
    auto lastModifiedCurrentMax = std::chrono::seconds::zero();
    auto lastModifiedNewMax = lastModifiedCurrentMax;
    fs::path contPath;

    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        if (stateEntry->getState() != ImportState::New)
            continue;
        auto dirEntry = stateEntry->getDirEntry();
        auto cdsObj = stateEntry->getObject();
        if (cdsObj && cdsObj->isContainer()) {
            std::shared_ptr<CdsContainer> container = std::dynamic_pointer_cast<CdsContainer>(cdsObj);
            if (contPath != "") {
                contentStateCache[contPath]->setMTime(lastModifiedNewMax);
            }
            parentContainer = container;
            contPath = itemPath;
            if (autoscanDir) {
                lastModifiedCurrentMax = autoscanDir->getPreviousLMT(contPath, parentContainer);
                lastModifiedNewMax = lastModifiedCurrentMax;
                autoscanDir->setCurrentLMT(contPath, std::chrono::seconds::zero());
            }
        } else if (isRegularFile(dirEntry, ec)) { // item
            auto contState = contentStateCache[itemPath.parent_path()];
            if (contState)
                parentContainer = std::dynamic_pointer_cast<CdsContainer>(contState->getObject());
            else
                log_error("No Container parent for Item {}", itemPath.string());

            if (!cdsObj) {
                log_debug("Searching Item {} in database", itemPath.string());
                cdsObj = database->findObjectByPath(itemPath, DbFileType::File);
            }
            if (cdsObj && cdsObj->isItem() && stateEntry->getMTime() > cdsObj->getMTime()) {
                log_debug("Updating Item {} in database {}", itemPath.string(), cdsObj->getID());
                auto item = std::dynamic_pointer_cast<CdsItem>(cdsObj);
                item->clearMetaData();
                item->clearAuxData();
                item->clearResources();
                updateSingleItem(dirEntry, item, item->getMimeType());
                if (lastModifiedNewMax < cdsObj->getMTime())
                    lastModifiedNewMax = cdsObj->getMTime();
                database->updateObject(cdsObj, nullptr);
                stateEntry->setObject(ImportState::Created, cdsObj);
                log_debug("Item changed {} {}", itemPath.string(), cdsObj->getID());
            } else if (cdsObj) {
                if (contState && contState->getMTime() < cdsObj->getMTime()) {
                    contState->setMTime(cdsObj->getMTime());
                    if (lastModifiedNewMax < cdsObj->getMTime())
                        lastModifiedNewMax = cdsObj->getMTime();
                }
                stateEntry->setObject(ImportState::Existing, cdsObj);
                log_debug("Item found {} {}", itemPath.string(), cdsObj->getID());
            } else {
                log_debug("Creating Item {}", itemPath.string());
                cdsObj = createSingleItem(dirEntry);
                if (cdsObj) {
                    if (contState && contState->getMTime() < cdsObj->getMTime()) {
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
                    log_error("Object not created for file {}", dirEntry.path().string());
                }
            }
            if (contState && cdsObj) {
                contState->setFirstObject(cdsObj);
            }
            if (parentContainer) {
                stateEntry->setParentObject(parentContainer);
            }
        }
    }
    if (autoscanDir && contPath != "") {
        autoscanDir->setCurrentLMT(contPath, lastModifiedNewMax);
    }
    log_debug("end {}", rootPath.string());
}

std::shared_ptr<CdsObject> ImportService::createSingleItem(const fs::directory_entry& dirEntry) // ToDo: Use StateEntry here
{
    auto objectPath = dirEntry.path();

    /* retrieve information about item and decide if it should be included */
    std::string mimetype = mime->getMimeType(objectPath, MIMETYPE_DEFAULT);
    if (mimetype.empty()) {
        log_error("Mime not found for file {}", objectPath.c_str());
        return nullptr;
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

    auto item = std::make_shared<CdsItem>();
    item->setLocation(objectPath);

    if (!mimetype.empty()) {
        item->setMimeType(mimetype);
    }
    if (!upnpClass.empty()) {
        item->setClass(upnpClass);
    }

    auto f2i = StringConverter::f2i(config);
    auto title = objectPath.filename().string();
    if (hasReadableNames && upnpClass != UPNP_CLASS_ITEM) {
        title = objectPath.stem().string();
        std::replace(title.begin(), title.end(), '_', ' ');
    }
    item->setTitle(f2i->convert(title));
    updateSingleItem(dirEntry, item, mimetype);

    return item;
}

void ImportService::updateSingleItem(const fs::directory_entry& dirEntry, std::shared_ptr<CdsItem> item, const std::string& mimetype)
{
    item->setMTime(toSeconds(dirEntry.last_write_time(ec)));
    item->setUTime(toSeconds(dirEntry.last_write_time(ec)));
    item->setSizeOnDisk(getFileSize(dirEntry));

    MetadataHandler::extractMetaData(context, content, item, dirEntry);
    updateItemData(item, mimetype);
}

void ImportService::fillLayout(const std::shared_ptr<GenericTask>& task)
{
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (!stateEntry || stateEntry->getState() != ImportState::Created)
            continue;
        contentStateCache[contPath]->setObject(ImportState::Loaded, stateEntry->getObject());
        fillSingleLayout(stateEntry, nullptr, stateEntry->getParentObject(), task);
    }
}

/// \param object used to make code compatible with legacy scan
void ImportService::fillSingleLayout(const std::shared_ptr<ContentState>& state, std::shared_ptr<CdsObject> object, const std::shared_ptr<CdsContainer>& parent, const std::shared_ptr<GenericTask>& task)
{
    std::shared_ptr<CdsObject> cdsObject = state ? state->getObject() : object;

    if (cdsObject && cdsObject->isItem() && layout) {
        try {
            std::string mimetype = std::static_pointer_cast<CdsItem>(cdsObject)->getMimeType();
            std::string contentType = getValueOrDefault(mimetypeContenttypeMap, mimetype);

            if (!autoscanDir || autoscanDir->hasContent(cdsObject->getClass())) {
                // only lock mutex while processing item layout
                std::scoped_lock<decltype(layoutMutex)> lock(layoutMutex);
                // get ref'd objects with last mod time
                auto listOrig = state ? database->getRefObjects(cdsObject->getID()) : std::vector<std::pair<int, std::chrono::seconds>> {};
                layout->processCdsObject(cdsObject, parent, rootPath, contentType, autoscanDir ? autoscanDir->getContainerTypes() : AutoscanDirectory::ContainerTypesDefaults);
                // compare ref'd objects last mod time
                auto listResult = state ? database->getRefObjects(cdsObject->getID()) : std::vector<std::pair<int, std::chrono::seconds>> {};
                for (auto&& origEntry : listOrig) {
                    auto newEntry = std::find_if(listResult.begin(), listResult.end(), [&](auto& entry) { return origEntry.first == entry.first && origEntry.second > entry.second; });
                    if (newEntry == listResult.end()) {
                        log_debug("Deleting ophaned virtual item {}", origEntry.first);
                        database->removeObject(origEntry.first, "", false);
                    }
                }
            } else {
                log_debug("file ignored: {} autoscanDir={}, class={}, hasContent={}, mediaType={}", cdsObject->getLocation().string(), !!autoscanDir, cdsObject->getClass(), autoscanDir ? autoscanDir->hasContent(cdsObject->getClass()) : false, autoscanDir ? autoscanDir->getMediaType() : -2);
            }

#ifdef HAVE_JS
            try {
                if (playlistParserScript && contentType == CONTENT_TYPE_PLAYLIST)
                    playlistParserScript->processPlaylistObject(cdsObject, std::move(task), rootPath);
            } catch (const std::runtime_error& e) {
                log_error("{}", e.what());
            }
#else
            if (contentType == CONTENT_TYPE_PLAYLIST)
                log_warning("Playlist {} will not be parsed: Gerbera was compiled without JS support!", cdsObject->getLocation().c_str());
#endif // HAVE_JS
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
        }
    }
}

void ImportService::updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype)
{
    if (hasDefaultDate && item->getMetaData(M_DATE).empty())
        item->addMetaData(M_DATE, fmt::format("{:%FT%T%z}", fmt::localtime(item->getMTime().count())));
    for (auto&& upnpPattern : upnpMap) {
        if (upnpPattern.isMatch(item, mimetype)) {
            item->setClass(upnpPattern.upnpClass);
            log_debug("UpnpClass '{}' for file {} from pattern {}", upnpPattern.upnpClass, item->getLocation().string(), upnpPattern.mimeType);
        }
    }
}

void ImportService::finishScan(const fs::path& location, const std::shared_ptr<CdsContainer>& parent, std::chrono::seconds lmt, const std::shared_ptr<CdsObject>& firstObject)
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
            assignFanArt(parent, firstObject, count);
        }
    }
}

void ImportService::assignFanArt(const std::shared_ptr<CdsContainer>& container, const std::shared_ptr<CdsObject>& refObj, int count)
{
    if (!container)
        return;

    if (refObj && refObj->getMTime() > container->getMTime()) {
        container->setMTime(refObj->getMTime());
        database->updateObject(container, nullptr);
    }

    if (containersWithFanArt.find(container->getID()) != containersWithFanArt.end()) {
        log_debug("Already assigned fanart {}", container->getID());
        return;
    }

    auto fanart = container->getResource(CdsResource::Purpose::Thumbnail);
    if (fanart && fanart->getHandlerType() != ContentHandler::CONTAINERART) {
        // remove stale references
        auto fanartObjId = stoiString(fanart->getAttribute(CdsResource::Attribute::FANART_OBJ_ID));
        try {
            if (fanartObjId > CDS_ID_ROOT) {
                database->loadObject(fanartObjId);
            }
        } catch (const ObjectNotFoundException&) {
            container->removeResource(fanart->getHandlerType());
            fanart = nullptr;
        }
    }
    if (!fanart) {
        MetadataHandler::createHandler(context, nullptr, ContentHandler::CONTAINERART)->fillMetadata(container);
        fanart = container->getResource(CdsResource::Purpose::Thumbnail);
        if (fanart)
            database->updateObject(container, nullptr);
    }
    auto location = container->getLocation();

    if (refObj) {
        if (!fanart && (refObj->isContainer() || (count < config->getIntOption(CFG_IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT) && container->getParentID() != CDS_ID_ROOT && std::distance(location.begin(), location.end()) > config->getIntOption(CFG_IMPORT_RESOURCES_CONTAINERART_MINDEPTH)))) {
            fanart = refObj->getResource(CdsResource::Purpose::Thumbnail);
            if (fanart) {
                if (fanart->getAttribute(CdsResource::Attribute::RESOURCE_FILE).empty()) {
                    fanart->addAttribute(CdsResource::Attribute::FANART_OBJ_ID, fmt::to_string(refObj->getID() != INVALID_OBJECT_ID ? refObj->getID() : refObj->getRefID()));
                    fanart->addAttribute(CdsResource::Attribute::FANART_RES_ID, fmt::to_string(fanart->getResId()));
                }
                container->addResource(fanart);
                log_debug("fanart from ref {}", fanart != nullptr);
                database->updateObject(container, nullptr);
            }
        }
    }
    if (fanart)
        containersWithFanArt[container->getID()] = container;
}

std::shared_ptr<CdsContainer> ImportService::getContainer(const fs::path& location) const
{
    return containerMap.at(location);
}

std::shared_ptr<CdsContainer> ImportService::createSingleContainer(int parentContainerId, const fs::directory_entry& dirEntry, const std::string& upnpClass)
{
    std::vector<std::shared_ptr<CdsObject>> cVec;
    auto location = dirEntry.path();
    for (auto&& segment : location) {
        if (segment != "/")
            cVec.push_back(std::make_shared<CdsContainer>(segment.string(), upnpClass));
    }
    std::vector<int> createdIds;
    addContainerTree(parentContainerId, cVec, createdIds);

    if (containerMap.find(location) != containerMap.end()) {
        auto result = containerMap.at(location);
        result->setMTime(toSeconds(dirEntry.last_write_time(ec)));
        return result;
    }
    return {};
}

/// \param createdIds used by messaging in ContentManager
std::pair<int, bool> ImportService::addContainerTree(int parentContainerId, const std::vector<std::shared_ptr<CdsObject>>& chain, std::vector<int>& createdIds)
{
    std::string tree; // accumulate path to container here
    int result = parentContainerId;
    bool isNew = false;
    bool isVirtual = parentContainerId != CDS_ID_FS_ROOT;
    int count = 0;
    auto dirKeys = config->getArrayOption(CFG_IMPORT_VIRTUAL_DIRECTORY_KEYS);
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
            for (auto&& field : dirKeys) {
                auto metaField = MetadataHandler::remapMetaDataField(field);
                auto keyValue = item->getMetaData(metaField);
                if (!keyValue.empty())
                    dirKeyValues.push_back(keyValue);
            }
            if (!dirKeyValues.empty()) {
                subTree = fmt::format("{}@{}", tree, fmt::join(dirKeyValues, "@"));
            } else {
                subTree = tree;
            }
            if (containerMap.find(subTree) == containerMap.end() || !containerMap[subTree]) {
                auto cont = database->findObjectByPath(subTree, DbFileType::Virtual);
                if (cont && cont->isContainer())
                    containerMap[subTree] = std::dynamic_pointer_cast<CdsContainer>(cont);
                else
                    containerMap[subTree] = nullptr;
                log_debug("Loaded container chain item {} -> {}", subTree, cont && cont->isContainer());
            }
        } else if (subTree.empty()) {
            subTree = tree;
        }
        if (containerMap.find(subTree) == containerMap.end() || !containerMap[subTree]) {
            item->removeMetaData(M_TITLE);
            item->addMetaData(M_TITLE, item->getTitle());
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
        } else {
            result = containerMap[subTree]->getID();
            if (item->getMTime() > containerMap[subTree]->getMTime()) {
                createdIds.push_back(result);
            }
        }
        count++;
        if (isVirtual)
            assignFanArt(containerMap[subTree], item, chain.size() - count);
    }
    return { result, isNew };
}

void ImportService::updateFanArt()
{
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        if (!stateEntry || !stateEntry->getObject() || !stateEntry->getObject()->isContainer())
            continue;
        std::shared_ptr<CdsContainer> container = std::dynamic_pointer_cast<CdsContainer>(stateEntry->getObject());
        assignFanArt(container, stateEntry->getFirstObject(), 1);
    }
}

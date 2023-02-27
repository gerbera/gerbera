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

#include "import_service.h"

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

ImportService::ImportService(std::shared_ptr<Context> context)
    : context(std::move(context))
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
{
    hasReadableNames = config->getBoolOption(CFG_IMPORT_READABLE_NAMES);
    mimetypeContenttypeMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    mimetypeUpnpclassMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    configLayoutMapping = config->getDictionaryOption(CFG_IMPORT_LAYOUT_MAPPING);
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
#ifdef HAVE_JS
    if (!playlistParserScript) {
        playlistParserScript = std::make_unique<PlaylistParserScript>(content);
    }
#endif
    if (!layout) {
        log_debug("virtual layout Type {}", layoutType);
        try {
            std::scoped_lock<decltype(layoutMutex)> lock(layoutMutex);

            if (layoutType == LayoutType::Js) {
#ifdef HAVE_JS
                layout = std::make_shared<JSLayout>(content);
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

void ImportService::doImport(std::unique_ptr<fs::directory_iterator>& dirIterator, AutoScanSetting& settings, std::unordered_set<int>& currentContent, const std::shared_ptr<GenericTask>& task)
{
    readDir(dirIterator, settings);
    createContainers();
    createItems(settings);
    fillLayout(task);
    // ToDo: update currentContent

    contentStateCache.clear();
}

void ImportService::readDir(std::unique_ptr<fs::directory_iterator>& dirIterator, AutoScanSetting& settings)
{
    for (auto&& dirEntry : *dirIterator) {
        auto&& newPath = dirEntry.path();
        auto&& name = newPath.filename().string();
        if ((name[0] == '.' && !settings.hidden)
            || (!settings.followSymlinks && dirEntry.is_symlink())
            || config->getConfigFilename() == newPath) {
            contentStateCache[newPath] = std::make_shared<ContentState>(dirEntry, ImportState::ToDelete);
            continue;
        }
        std::error_code ec;
        contentStateCache[newPath] = std::make_shared<ContentState>(dirEntry, ImportState::New, toSeconds(dirEntry.last_write_time(ec)));
        if (dirEntry.is_directory(ec) && settings.recursive) {
            if (!ec) {
                auto subIterator = std::make_unique<fs::directory_iterator>(newPath, ec);
                if (!ec) {
                    readDir(subIterator, settings);
                }
            }
        }
    }
}

void ImportService::createContainers()
{
    std::error_code ec;

    for (auto&& [contPath, stateEntry] : contentStateCache) {
        auto dirEntry = stateEntry->getDirEntry();
        if (dirEntry.exists(ec) && dirEntry.is_directory(ec)) {
            int containerID = database->findObjectIDByPath(contPath);

            if (containerID != INVALID_OBJECT_ID) {
                try {
                    std::shared_ptr<CdsObject> container = database->loadObject(containerID);
                    if (!container || !container->isContainer()) {
                        throw_std_runtime_error("Item {} is not a container", containerID);
                    }
                    stateEntry->setObject(ImportState::Existing, container);
                    auto lastModified = toSeconds(dirEntry.last_write_time(ec));
                    if (lastModified > container->getMTime()) {
                        container->setMTime(lastModified);
                    }

                } catch (const std::runtime_error& e) {
                    log_error("createObjects: Failed to load parent container {}, {}", contPath.c_str(), e.what());
                }
            } else {
                // Create container
                stateEntry->setObject(ImportState::Created, createSingleContainer(dirEntry, UPNP_CLASS_CONTAINER));
            }
        }
    }
}

void ImportService::createItems(AutoScanSetting& settings)
{
    std::error_code ec;
    std::shared_ptr<CdsContainer> parentContainer = nullptr;
    auto lastModifiedCurrentMax = std::chrono::seconds::zero();
    // auto_currentTme_=_currentTime();
    auto lastModifiedNewMax = lastModifiedCurrentMax;
    fs::path contPath;

    for (auto&& [itemPath, stateEntry] : contentStateCache) {
        auto dirEntry = stateEntry->getDirEntry();
        auto cdsObj = stateEntry->getObject();
        std::shared_ptr<CdsContainer> container = std::dynamic_pointer_cast<CdsContainer>(cdsObj);
        if (container) {
            parentContainer = container;
            contPath = itemPath;
            if (autoscanDir) {
                lastModifiedCurrentMax = autoscanDir->getPreviousLMT(contPath, parentContainer);
                lastModifiedNewMax = lastModifiedCurrentMax;
                autoscanDir->setCurrentLMT(contPath, std::chrono::seconds::zero());
            }
        } else if (isRegularFile(dirEntry, ec)) { // item
            cdsObj = database->findObjectByPath(itemPath);
            if (cdsObj) {
                stateEntry->setObject(ImportState::Existing, cdsObj);
            } else {
                cdsObj = createSingleItem(dirEntry);
                lastModifiedNewMax = lastModifiedNewMax < cdsObj->getMTime() ? lastModifiedNewMax : cdsObj->getMTime();
                stateEntry->setObject(ImportState::Created, cdsObj);
                cdsObj->setParentID(parentContainer ? parentContainer->getID() : INVALID_OBJECT_ID);
                database->addObject(cdsObj, nullptr);
            }
        }
    }
}

std::shared_ptr<CdsObject> ImportService::createSingleItem(const fs::directory_entry& dirEntry)
{
    auto objectPath = dirEntry.path();
    std::error_code ec;

    /* retrieve information about item and decide if it should be included */
    std::string mimetype = mime->getMimeType(objectPath, MIMETYPE_DEFAULT);
    if (mimetype.empty()) {
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
    std::shared_ptr<CdsObject> obj = item;
    item->setLocation(objectPath);
    item->setMTime(toSeconds(dirEntry.last_write_time(ec)));
    item->setSizeOnDisk(getFileSize(dirEntry));

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
    obj->setTitle(f2i->convert(title));

    MetadataHandler::extractMetaData(context, content, item, dirEntry);
    updateItemData(item, mimetype);

    return obj;
}

void ImportService::fillLayout(const std::shared_ptr<GenericTask>& task)
{
    for (auto&& [contPath, stateEntry] : contentStateCache) {
        contentStateCache[contPath]->setObject(ImportState::Loaded, stateEntry->getObject());
        fillSingleLayout(stateEntry, nullptr, task);
    }
}

/// \param object used to make code compatible with legacy scan
void ImportService::fillSingleLayout(const std::shared_ptr<ContentState>& state, std::shared_ptr<CdsObject> object, const std::shared_ptr<GenericTask>& task)
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
                layout->processCdsObject(cdsObject, rootPath, contentType, autoscanDir ? autoscanDir->getContainerTypes() : AutoscanDirectory::ContainerTypesDefaults);
                // compare ref'd objects last mod time
                auto listResult = state ? database->getRefObjects(cdsObject->getID()) : std::vector<std::pair<int, std::chrono::seconds>> {};
                for (auto&& origEntry : listOrig) {
                    auto newEntry = std::find_if(listResult.begin(), listResult.end(), [&](auto& entry) { return origEntry.first == entry.first && origEntry.second > entry.second; });
                    if (newEntry == listResult.end()) {
                        database->removeObject(origEntry.first, false);
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
    if (item->getMetaData(M_DATE).empty())
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

    if (containersWithFanArt.find(container->getID()) != containersWithFanArt.end())
        return;

    auto fanart = container->getResource(CdsResource::Purpose::Thumbnail);
    if (fanart && fanart->getHandlerType() != ContentHandler::CONTAINERART) {
        // remove stale references
        auto fanartObjId = stoiString(fanart->getAttribute(CdsResource::Attribute::FANART_OBJ_ID));
        try {
            if (fanartObjId > 0) {
                database->loadObject(fanartObjId);
            }
        } catch (const ObjectNotFoundException&) {
            container->removeResource(fanart->getHandlerType());
            fanart = nullptr;
        }
    }
    if (!fanart) {
        MetadataHandler::createHandler(context, nullptr, ContentHandler::CONTAINERART)->fillMetadata(container);
        database->updateObject(container, nullptr);
        fanart = container->getResource(CdsResource::Purpose::Thumbnail);
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
            }
            database->updateObject(container, nullptr);
        }
        containersWithFanArt[container->getID()] = container;
    }
}

std::shared_ptr<CdsContainer> ImportService::getContainer(const fs::path& location) const
{
    return containerMap.at(location);
}

std::shared_ptr<CdsContainer> ImportService::createSingleContainer(const fs::directory_entry& dirEntry, const std::string& upnpClass)
{
    std::vector<std::shared_ptr<CdsObject>> cVec;
    auto location = dirEntry.path();
    for (auto&& segment : location) {
        cVec.push_back(std::make_shared<CdsContainer>(segment.string(), upnpClass));
    }
    std::vector<int> createdIds;
    addContainerTree(cVec, createdIds);

    if (containerMap.find(location) != containerMap.end()) {
        auto result = containerMap.at(location);
        std::error_code ec;
        result->setMTime(toSeconds(dirEntry.last_write_time(ec)));
        return result;
    }
    return {};
}

/// \param createdIds used by messaging in ContentManager
std::pair<int, bool> ImportService::addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain, std::vector<int>& createdIds)
{
    std::string tree; // accumulate path to container here
    int result = CDS_ID_ROOT;
    bool isNew = false;
    int count = 0;
    for (auto&& item : chain) {
        if (item->getTitle().empty()) {
            log_error("Received chain item without title");
            return { INVALID_OBJECT_ID, false };
        }
        tree = fmt::format("{}{}{}", tree, VIRTUAL_CONTAINER_SEPARATOR, escape(item->getTitle(), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR));
        log_debug("Received container chain item {}", tree);
        for (auto&& [key, val] : configLayoutMapping) {
            tree = std::regex_replace(tree, std::regex(key), val);
        }
        if (containerMap.find(tree) == containerMap.end()) {
            item->removeMetaData(M_TITLE);
            item->addMetaData(M_TITLE, item->getTitle());
            auto cont = std::dynamic_pointer_cast<CdsContainer>(item);
            log_debug("Creating container chain item {}", tree);
            if (database->addContainer(result, tree, cont, &result)) {
                createdIds.push_back(result);
            }
            auto container = std::dynamic_pointer_cast<CdsContainer>(database->loadObject(result));
            containerMap[tree] = container;
            if (item->getMTime() > container->getMTime()) {
                createdIds.push_back(result); // ensure update
            }
            isNew = true;
        } else {
            result = containerMap[tree]->getID();
            if (item->getMTime() > containerMap[tree]->getMTime()) {
                createdIds.push_back(result);
            }
        }
        count++;
        assignFanArt(containerMap[tree], item, chain.size() - count);
    }
    return { result, isNew };
}
/*MT*

    MediaTomb - http://www.mediatomb.cc/

    content_manager.cc - this file is part of MediaTomb.

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

/// \file content_manager.cc
#define LOG_FAC log_facility_t::content

#include "content_manager.h" // API

#include <fmt/chrono.h>
#include <regex>

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/directory_tweak.h"
#include "database/database.h"
#include "layout/builtin_layout.h"
#include "metadata/metadata_handler.h"
#include "update_manager.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/timer.h"
#include "util/tools.h"
#include "util/upnp_clients.h"
#include "web/session_manager.h"

#ifdef HAVE_JS
#include "layout/js_layout.h"
#endif

#ifdef HAVE_LASTFMLIB
#include "onlineservice/lastfm_scrobbler.h"
#endif

#ifdef ATRAILERS
#include "onlineservice/atrailers_service.h"
#endif

#ifdef ONLINE_SERVICES
#include "onlineservice/task_processor.h"
#endif

#ifdef HAVE_JS
#include "scripting/scripting_runtime.h"
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

ContentManager::ContentManager(const std::shared_ptr<Context>& context,
    const std::shared_ptr<Server>& server, std::shared_ptr<Timer> timer)
    : config(context->getConfig())
    , mime(context->getMime())
    , database(context->getDatabase())
    , session_manager(context->getSessionManager())
    , context(context)
    , timer(std::move(timer))
#ifdef HAVE_JS
    , scripting_runtime(std::make_shared<ScriptingRuntime>())
#endif
#ifdef HAVE_LASTFMLIB
    , last_fm(std::make_shared<LastFm>(context))
#endif
{
    update_manager = std::make_shared<UpdateManager>(config, database, server);
#ifdef ONLINE_SERVICES
    task_processor = std::make_shared<TaskProcessor>(config);
#endif

    mimetypeContenttypeMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    mimetypeUpnpclassMap = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    initUpnpMap(upnpMap, mimetypeUpnpclassMap);
}

void ContentManager::initUpnpMap(std::vector<UpnpMap>& target, const std::map<std::string, std::string>& source)
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

        target.emplace_back(mt, cls, filters);
    }
}

void ContentManager::run()
{
    update_manager->run();
#ifdef ONLINE_SERVICES
    task_processor->run();
#endif
#ifdef HAVE_LASTFMLIB
    last_fm->run();
#endif
    threadRunner = std::make_unique<ThreadRunner<std::condition_variable_any, std::recursive_mutex>>(
        "ContentTaskThread", [](void* arg) -> void* {
            auto inst = static_cast<ContentManager*>(arg);
            inst->threadProc();
            return nullptr;
        },
        this);

    // wait for ContentTaskThread to become ready
    threadRunner->waitForReady();

    if (!threadRunner->isAlive()) {
        throw_std_runtime_error("Could not start ContentTaskThread thread");
    }

    autoscanList = database->getAutoscanList(AutoscanDirectory::ScanMode::Timed);
    for (auto& dir : config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST)) {
        fs::path path = dir.getLocation();
        if (fs::is_directory(path)) {
            dir.setObjectID(ensurePathExistence(path));
        }
        try {
            autoscanList->add(std::make_shared<AutoscanDirectory>(dir));
        } catch (const std::runtime_error& e) {
            // Work around existing config sourced autoscans that were stored to the DB for reasons
            log_warning(e.what());
            continue;
        }
    }

    auto self = shared_from_this();
#ifdef HAVE_INOTIFY
    inotify = std::make_unique<AutoscanInotify>(self);

    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        auto db = database->getAutoscanList(AutoscanDirectory::ScanMode::INotify);
        for (std::size_t i = 0; i < db->size(); i++) {
            auto dir = db->get(i);
            auto path = dir->getLocation();
            if (fs::is_directory(path)) {
                dir->setObjectID(ensurePathExistence(path));
            }
            try {
                autoscanList->add(dir);
            } catch (const std::runtime_error& e) {
                // Work around existing config sourced autoscans that were stored to the DB for reasons
                log_warning(e.what());
                continue;
            }
        }
        for (auto dir : config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST)) {
            fs::path path = dir.getLocation();
            if (fs::is_directory(path)) {
                dir.setObjectID(ensurePathExistence(path));
            }
            try {
                autoscanList->add(std::make_shared<AutoscanDirectory>(dir));
            } catch (const std::runtime_error& e) {
                // Work around existing config sourced autoscans that were stored to the DB for reasons
                log_warning(e.what());
                continue;
            }
        }
    }

    // Start INotify thread
    inotify->run();
#endif

    std::string layoutType = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    log_debug("virtual layout Type {}", layoutType);
    layoutEnabled = (layoutType == LAYOUT_TYPE_BUILTIN || layoutType == LAYOUT_TYPE_JS);

#ifdef ONLINE_SERVICES
    online_services = std::make_unique<OnlineServiceList>();

#ifdef ATRAILERS
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        try {
            auto at = std::make_shared<ATrailersService>(self);

            auto i = std::chrono::seconds(config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH));
            at->setRefreshInterval(i);

            i = std::chrono::seconds(config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER));
            at->setItemPurgeInterval(i);
            if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            auto atParam = std::make_shared<Timer::Parameter>(Timer::Parameter::IDOnlineContent, OS_ATrailers);
            at->setTimerParameter(std::move(atParam));
            online_services->registerService(at);
            if (i > std::chrono::seconds::zero()) {
                timer->addTimerSubscriber(this, i, at->getTimerParameter(), true);
            }
        } catch (const std::runtime_error& ex) {
            log_error("Could not setup Apple Trailers: {}", ex.what());
        }
    }
#endif // ATRAILERS

#endif // ONLINE_SERVICES

    initLayout();

#ifdef HAVE_JS
    initJS();
#endif

    autoscanList->notifyAll(this);

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        /// \todo change this (we need a new autoscan architecture)
        for (std::size_t i = 0; i < autoscanList->size(); i++) {
            auto adir = autoscanList->get(i);
            if (!adir) {
                continue;
            }
            if (adir->getScanMode() != AutoscanDirectory::ScanMode::INotify) {
                continue;
            }
            inotify->monitor(adir);
            auto param = std::make_shared<Timer::Parameter>(Timer::Parameter::timer_param_t::IDAutoscan, adir->getScanID());
            log_debug("Adding one-shot inotify scan");
            timer->addTimerSubscriber(this, std::chrono::minutes(1), std::move(param), true);
        }
    }
#endif

    for (std::size_t i = 0; i < autoscanList->size(); i++) {
        auto adir = autoscanList->get(i);
        if (!adir) {
            continue;
        }
        if (adir->getScanMode() != AutoscanDirectory::ScanMode::Timed) {
            continue;
        }
        auto param = std::make_shared<Timer::Parameter>(Timer::Parameter::timer_param_t::IDAutoscan, adir->getScanID());
        log_debug("Adding timed scan with interval {}", adir->getInterval().count());
        timer->addTimerSubscriber(this, adir->getInterval(), std::move(param), false);
    }
}

void ContentManager::registerExecutor(const std::shared_ptr<Executor>& exec)
{
    auto lock = threadRunner->lockGuard("registerExecutor");
    process_list.push_back(exec);
}

void ContentManager::unregisterExecutor(const std::shared_ptr<Executor>& exec)
{
    // when shutting down we will kill the transcoding processes,
    // which if given enough time will get a close in the io handler and
    // will try to unregister themselves - this would mess up the
    // transcoding_processes list
    // since we are anyway shutting down we can ignore the unregister call
    // and go through the list, ensuring that no zombie stays alive :>
    if (shutdownFlag)
        return;

    auto lock = threadRunner->lockGuard("unregisterExecutor");

    process_list.erase(std::remove(process_list.begin(), process_list.end(), exec), process_list.end());
}

void ContentManager::timerNotify(const std::shared_ptr<Timer::Parameter>& parameter)
{
    if (!parameter)
        return;

    if (parameter->whoami() == Timer::Parameter::IDAutoscan) {
        auto adir = autoscanList->get(parameter->getID());

        // do not rescan while other scans are still active
        if (!adir || adir->getActiveScanCount() > 0 || adir->getTaskCount() > 0)
            return;

        rescanDirectory(adir, adir->getObjectID());
    }
#ifdef ONLINE_SERVICES
    else if (parameter->whoami() == Timer::Parameter::IDOnlineContent) {
        fetchOnlineContent(service_type_t(parameter->getID()));
    }
#endif // ONLINE_SERVICES
}

void ContentManager::shutdown()
{
    log_debug("start");
    auto lock = threadRunner->uniqueLock();
    log_debug("updating last_modified data for autoscan in database...");
    if (autoscanList)
        autoscanList->updateLMinDB(*database);

    destroyLayout();
#ifdef HAVE_JS
    destroyJS();
#endif

    if (autoscanList) {
        // update modification time for database
        for (std::size_t i = 0; i < autoscanList->size(); i++) {
            log_debug("AutoScanDir {}", i);
            auto dir = autoscanList->get(i);
            if (dir) {
                auto dirEnt = fs::directory_entry(dir->getLocation());
                if (dirEnt.is_directory()) {
                    auto t = toSeconds(dirEnt.last_write_time());
                    dir->setCurrentLMT(dir->getLocation(), t);
                }
                dir->updateLMT();
            }
        }
        autoscanList->updateLMinDB(*database);

        autoscanList = nullptr;
#ifdef HAVE_INOTIFY
        inotify = nullptr;
#endif
    }

    shutdownFlag = true;

    for (auto&& exec : process_list) {
        if (exec)
            exec->kill();
    }

    log_debug("signalling...");
    threadRunner->notify();
    lock.unlock();
    log_debug("waiting for thread...");

    threadRunner->join();

#ifdef HAVE_LASTFMLIB
    last_fm->shutdown();
    last_fm = nullptr;
#endif
#ifdef HAVE_JS
    scripting_runtime = nullptr;
#endif
#ifdef ONLINE_SERVICES
    task_processor->shutdown();
    task_processor = nullptr;
#endif
    update_manager->shutdown();
    update_manager = nullptr;

    log_debug("end");
}

std::shared_ptr<GenericTask> ContentManager::getCurrentTask() const
{
    auto lock = threadRunner->lockGuard("getCurrentTask");

    return currentTask;
}

std::deque<std::shared_ptr<GenericTask>> ContentManager::getTasklist()
{
    auto lock = threadRunner->lockGuard("getTasklist");

    std::deque<std::shared_ptr<GenericTask>> taskList;
#ifdef ONLINE_SERVICES
    taskList = task_processor->getTasklist();
#endif
    auto t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // therefore we do not have to allocate the array
    if (!t)
        return taskList;

    taskList.push_back(std::move(t));
    std::copy_if(taskQueue1.begin(), taskQueue1.end(), std::back_inserter(taskList), [](auto&& task) { return task->isValid(); });

    for (auto&& task : taskQueue2) {
        if (task->isValid())
            taskList.clear();
    }

    return taskList;
}

void ContentManager::addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allowFifo)
{
    obj->validate();
    fs::path path = obj->getLocation();

    std::error_code ec;
    auto dirEnt = fs::directory_entry(path, ec);
    if (ec || !isRegularFile(dirEnt, ec))
        throw_std_runtime_error("Not a file: {} - {}", path.c_str(), ec.message());

    auto pcdir = database->findObjectByPath(path);
    if (!pcdir) {
        pcdir = createObjectFromFile(dirEnt, true, allowFifo);
        if (!pcdir) {
            throw_std_runtime_error("Could not add {}", path.c_str());
        }
        if (pcdir->isItem()) {
            this->addObject(pcdir, true);
            obj->setRefID(pcdir->getID());
        }
    }

    addObject(obj, true);
}

std::shared_ptr<CdsObject> ContentManager::createSingleItem(
    const fs::directory_entry& dirEnt,
    const fs::path& rootPath,
    bool followSymlinks,
    bool checkDatabase,
    bool processExisting,
    bool firstChild,
    const std::shared_ptr<AutoscanDirectory>& adir,
    std::shared_ptr<CMAddFileTask> task)
{
    auto obj = checkDatabase ? database->findObjectByPath(dirEnt.path()) : nullptr;
    bool isNew = false;

    if (!obj) {
        obj = createObjectFromFile(dirEnt, followSymlinks);
        if (!obj) { // object ignored
            log_debug("Link to file or directory ignored: {}", dirEnt.path().c_str());
            return nullptr;
        }
        if (obj->isItem()) {
            addObject(obj, firstChild);
            isNew = true;
        }
    } else if (obj->isItem() && processExisting) {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        MetadataHandler::extractMetaData(context, shared_from_this(), item, dirEnt);
        updateItemData(item, item->getMimeType());
    }
    if (obj->isItem() && layout && (processExisting || isNew)) {
        try {
            std::string mimetype = std::static_pointer_cast<CdsItem>(obj)->getMimeType();
            std::string contentType = getValueOrDefault(mimetypeContenttypeMap, mimetype);

            if (!adir || adir->hasContent(obj->getClass())) {
                // only lock mutex while processing item layout
                std::scoped_lock<decltype(layoutMutex)> lock(layoutMutex);
                layout->processCdsObject(obj, rootPath, contentType, adir ? adir->getContainerTypes() : AutoscanDirectory::ContainerTypesDefaults);
            } else {
                log_debug("file ignored: {} adir={}, class={}, hasContent={}", dirEnt.path().c_str(), !!adir, obj->getClass(), adir ? adir->hasContent(obj->getClass()) : false);
            }

#ifdef HAVE_JS
            try {
                if (playlist_parser_script && contentType == CONTENT_TYPE_PLAYLIST)
                    playlist_parser_script->processPlaylistObject(obj, std::move(task), rootPath);
            } catch (const std::runtime_error& e) {
                log_error("{}", e.what());
            }
#else
            if (contentType == CONTENT_TYPE_PLAYLIST)
                log_warning("Playlist {} will not be parsed: Gerbera was compiled without JS support!", obj->getLocation().c_str());
#endif // HAVE_JS
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
        }
    }
    return obj;
}

void ContentManager::parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const
{
#ifdef HAVE_JS
    try {
        if (metafileParserScript)
            metafileParserScript->processObject(obj, path);
    } catch (const std::runtime_error& e) {
        log_error("{}", e.what());
    }
#else
    log_warning("Metadata file {} will not be parsed: Gerbera was compiled without JS support!", path.string());
#endif // HAVE_JS
}

int ContentManager::_addFile(const fs::directory_entry& dirEnt, fs::path rootPath, AutoScanSetting& asSetting, const std::shared_ptr<CMAddFileTask>& task)
{
    if (!asSetting.hidden && dirEnt.path().is_relative())
        return INVALID_OBJECT_ID;

    // never add the server configuration file
    if (config->getConfigFilename() == dirEnt.path())
        return INVALID_OBJECT_ID;

    if (rootPath.empty() && task)
        rootPath = task->getRootPath();
    // checkDatabase, don't process existing
    auto obj = createSingleItem(dirEnt, rootPath, asSetting.followSymlinks, true, false, false, asSetting.adir, task);
    if (!obj) // object ignored
        return INVALID_OBJECT_ID;

    if (asSetting.recursive && obj->isContainer()) {
        addRecursive(asSetting.adir, dirEnt, asSetting.followSymlinks, asSetting.hidden, task);
    }

    if (asSetting.rescanResource && obj->hasResource(ContentHandler::RESOURCE)) {
        std::string parentPath = dirEnt.path().parent_path();
        updateAttachedResources(asSetting.adir, obj, parentPath, true);
    }

    return obj->getID();
}

bool ContentManager::updateAttachedResources(const std::shared_ptr<AutoscanDirectory>& adir, const std::shared_ptr<CdsObject>& obj, const fs::path& parentPath, bool all)
{
    if (!adir)
        return false;

    bool parentRemoved = false;
    int parentID = database->findObjectIDByPath(parentPath, false);
    if (parentID != INVALID_OBJECT_ID) {
        // as there is no proper way to force refresh of unchanged files, delete whole dir and rescan it
        _removeObject(adir, parentID, parentPath, false, all);
        // in order to rescan whole directory we have to set lmt to a very small value
        AutoScanSetting asSetting;
        asSetting.adir = adir;
        adir->setCurrentLMT(parentPath, std::chrono::seconds(1));
        asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
        asSetting.hidden = config->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
        asSetting.recursive = true;
        asSetting.rescanResource = false;
        asSetting.mergeOptions(config, parentPath);
        std::error_code ec;
        // addFile(const fs::directory_entry& path, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
        auto dirEntry = fs::directory_entry(parentPath, ec);
        if (!ec) {
            addFile(dirEntry, asSetting, true, true, false);
            log_debug("Forced rescan of {} for resource {}", parentPath.c_str(), obj->getLocation().c_str());
            parentRemoved = true;
        } else {
            log_error("Failed to read {}: {}", parentPath.c_str(), ec.message());
        }
    }
    return parentRemoved;
}

std::vector<int> ContentManager::_removeObject(const std::shared_ptr<AutoscanDirectory>& adir, int objectID, const fs::path& path, bool rescanResource, bool all)
{
    if (objectID == CDS_ID_ROOT)
        throw_std_runtime_error("cannot remove root container");
    if (objectID == CDS_ID_FS_ROOT)
        throw_std_runtime_error("cannot remove PC-Directory container");
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw_std_runtime_error("tried to remove illegal object id");

    bool parentRemoved = false;
    std::shared_ptr<CdsObject> obj = database->loadObject(objectID);

    if (rescanResource) {
        if (obj && obj->hasResource(ContentHandler::RESOURCE)) {
            auto parentPath = obj->getLocation().parent_path();
            parentRemoved = updateAttachedResources(adir, obj, parentPath, all);
        }
    }

    if (obj->isContainer()) {
        cleanupTasks(path);
    }
    // Removing a file can lead to virtual directories to drop empty and be removed
    // So current container cache must be invalidated
    containerMap.clear();
    containersWithFanArt.clear();

    if (!parentRemoved) {
        auto changedContainers = database->removeObject(objectID, all);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
            return changedContainers->upnp;
        }
    }

    if (rescanResource && obj && parentRemoved)
        return { obj->getParentID() };
    return {};
}

int ContentManager::ensurePathExistence(const fs::path& path) const
{
    int updateID;
    int containerID = database->ensurePathExistence(path, &updateID);
    if (updateID != INVALID_OBJECT_ID) {
        update_manager->containerChanged(updateID);
        session_manager->containerChangedUI(updateID);
    }
    return containerID;
}

void ContentManager::_rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int containerID, const std::shared_ptr<GenericTask>& task)
{
    log_debug("start");

    if (!adir)
        throw_std_runtime_error("ID valid but nullptr returned? this should never happen");

    fs::path rootpath = adir->getLocation();

    fs::path location;
    std::shared_ptr<CdsContainer> parentContainer;

    if (containerID != INVALID_OBJECT_ID) {
        try {
            std::shared_ptr<CdsObject> obj = database->loadObject(containerID);
            if (!obj || !obj->isContainer()) {
                throw_std_runtime_error("Item {} is not a container", containerID);
            }
            location = (containerID == CDS_ID_FS_ROOT) ? FS_ROOT_DIRECTORY : obj->getLocation();
            parentContainer = std::dynamic_pointer_cast<CdsContainer>(obj);
        } catch (const std::runtime_error&) {
            if (adir->persistent()) {
                containerID = INVALID_OBJECT_ID;
            } else {
                removeAutoscanDirectory(adir);
                return;
            }
        }
    }

    if (containerID == INVALID_OBJECT_ID) {
        if (!fs::is_directory(adir->getLocation())) {
            adir->setObjectID(INVALID_OBJECT_ID);
            database->updateAutoscanDirectory(adir);
            if (adir->persistent()) {
                return;
            }

            removeAutoscanDirectory(adir);
            return;
        }

        containerID = ensurePathExistence(adir->getLocation());
        adir->setObjectID(containerID);
        database->updateAutoscanDirectory(adir);
        location = adir->getLocation();

        std::shared_ptr<CdsObject> obj = database->loadObject(containerID);
        if (!obj || !obj->isContainer()) {
            throw_std_runtime_error("Item {} is not a container", containerID);
        }
        parentContainer = std::dynamic_pointer_cast<CdsContainer>(obj);
    }

    if (location.empty()) {
        log_error("Container with ID {} has no location information", containerID);
        return;
    }

    log_debug("Rescanning location: {}", location.c_str());

    std::error_code ec;
    auto rootDir = fs::directory_entry(location, ec);
    std::unique_ptr<fs::directory_iterator> dIter;

    if (!ec && rootDir.exists(ec) && rootDir.is_directory(ec)) {
        dIter = std::make_unique<fs::directory_iterator>(location, ec);
        if (ec) {
            log_error("_rescanDirectory: Failed to iterate {}, {}", location.c_str(), ec.message());
        }
    } else {
        log_error("Could not open {}: {}", location.c_str(), ec.message());
    }
    if (ec || !dIter) {
        if (adir->persistent()) {
            removeObject(adir, containerID, location, false);
            if (location == adir->getLocation()) {
                adir->setObjectID(INVALID_OBJECT_ID);
                database->updateAutoscanDirectory(adir);
            }
            return;
        }

        if (location == adir->getLocation()) {
            removeObject(adir, containerID, location, false);
            removeAutoscanDirectory(adir);
        }
        return;
    }

    AutoScanSetting asSetting;
    asSetting.adir = adir;
    asSetting.recursive = adir->getRecursive();
    asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
    asSetting.hidden = adir->getHidden();
    asSetting.mergeOptions(config, location);

    log_debug("Rescanning options {}: recursive={} hidden={} followSymlinks={}", location.c_str(), asSetting.recursive, asSetting.hidden, asSetting.followSymlinks);

    // request only items if non-recursive scan is wanted
    auto list = database->getObjects(containerID, !asSetting.recursive);

    unsigned int thisTaskID;
    if (task) {
        thisTaskID = task->getID();
    } else {
        thisTaskID = 0;
    }

    auto lastModifiedCurrentMax = adir->getPreviousLMT(location, parentContainer);
    auto currentTme = currentTime();
    auto lastModifiedNewMax = lastModifiedCurrentMax;
    adir->setCurrentLMT(location, std::chrono::seconds::zero());

    std::shared_ptr<CdsObject> firstObject;
    for (auto&& dirEnt : *dIter) {
        auto&& newPath = dirEnt.path();
        auto&& name = newPath.filename().string();
        if (name[0] == '.' && !asSetting.hidden) {
            continue;
        }

        if (shutdownFlag || (task && !task->isValid()))
            break;

        // it is possible that someone hits remove while the container is being scanned
        // in this case we will invalidate the autoscan entry
        if (!adir->isValid()) {
            log_info("lost autoscan for {}", newPath.c_str());
            finishScan(adir, location, parentContainer, lastModifiedNewMax);
            return;
        }

        if (!asSetting.followSymlinks && dirEnt.is_symlink()) {
            int objectID = database->findObjectIDByPath(newPath);
            if (objectID > 0) {
                list.erase(objectID);
                removeObject(adir, objectID, newPath, false);
            }
            log_debug("link {} skipped", newPath.c_str());
            continue;
        }

        asSetting.recursive = adir->getRecursive();
        asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
        asSetting.hidden = adir->getHidden();
        asSetting.mergeOptions(config, location);
        auto lwt = toSeconds(dirEnt.last_write_time(ec));

        if (isRegularFile(dirEnt, ec)) {
            int objectID = database->findObjectIDByPath(newPath);
            if (objectID > 0) {
                list.erase(objectID);

                // check modification time and update file if chagned
                if (lastModifiedCurrentMax < lwt) {
                    // re-add object - we have to do this in order to trigger
                    // layout
                    auto removedParents = removeObject(adir, objectID, newPath, false, false);
                    if (std::find_if(removedParents.begin(), removedParents.end(), [=](auto&& pId) { return containerID == pId; }) != removedParents.end()) {
                        // parent purged
                        parentContainer = nullptr;
                    }
                    asSetting.recursive = false;
                    asSetting.rescanResource = false;
                    objectID = addFileInternal(dirEnt, rootpath, asSetting, false);
                    // update time variable
                    if (lastModifiedNewMax < lwt && lwt <= currentTme)
                        lastModifiedNewMax = lwt;
                }
            } else {
                // add file, not recursive, not async, not forced
                asSetting.recursive = false;
                asSetting.rescanResource = false;
                objectID = addFileInternal(dirEnt, rootpath, asSetting, false);
                if (lastModifiedNewMax < lwt && lwt <= currentTme)
                    lastModifiedNewMax = lwt;
            }
            if (!firstObject && objectID > 0) {
                firstObject = database->loadObject(objectID);
                if (!firstObject->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                    firstObject = nullptr;
                }
            }
        } else if (dirEnt.is_directory(ec) && asSetting.recursive) {
            int objectID = database->findObjectIDByPath(newPath);
            if (lastModifiedNewMax < lwt && lwt <= currentTme)
                lastModifiedNewMax = lwt;
            if (objectID > 0) {
                log_debug("rescanSubDirectory {}", newPath.c_str());
                list.erase(objectID);
                // add a task to rescan the directory that was found
                rescanDirectory(adir, objectID, newPath, task->isCancellable());
            } else {
                log_debug("addSubDirectory {}", newPath.c_str());

                // we have to make sure that we will never add a path to the task list
                // if it is going to be removed by a pending remove task.
                // this lock will make sure that remove is not in the process of invalidating
                // the AutocsanDirectories in the autoscan_timed list at the time when we
                // are checking for validity.
                auto lock = threadRunner->lockGuard("addSubDirectory " + newPath.string());

                // it is possible that someone hits remove while the container is being scanned
                // in this case we will invalidate the autoscan entry
                if (!adir->isValid()) {
                    log_info("lost autoscan for {}", newPath.c_str());
                    finishScan(adir, location, parentContainer, lastModifiedNewMax);
                    return;
                }
                // add directory, recursive, async, hidden flag, low priority
                asSetting.recursive = true;
                asSetting.rescanResource = false;
                asSetting.mergeOptions(config, newPath);
                // const fs::path& path, const fs::path& rootpath, AutoScanSetting& asSetting, bool async, bool lowPriority, unsigned int parentTaskID, bool cancellable
                addFileInternal(dirEnt, rootpath, asSetting, true, true, thisTaskID, task->isCancellable());
                log_debug("addSubDirectory {} done", newPath.c_str());
            }
        }
        if (!parentContainer && !location.empty()) {
            std::shared_ptr<CdsObject> obj = database->findObjectByPath(location, false);
            if (!obj || !obj->isContainer()) {
                log_error("Updated parent {} is not available or no container", location.string());
            } else {
                parentContainer = std::dynamic_pointer_cast<CdsContainer>(obj);
                containerID = parentContainer->getID();
            }
        }
        if (ec) {
            log_error("_rescanDirectory: Failed to read {}, {}", newPath.c_str(), ec.message());
        }
    } // dIter

    finishScan(adir, location, parentContainer, lastModifiedNewMax, firstObject);

    if (shutdownFlag || (task && !task->isValid())) {
        return;
    }
    if (!list.empty()) {
        auto changedContainers = database->removeObjects(list);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(std::shared_ptr<AutoscanDirectory>& adir, const fs::directory_entry& subDir, bool followSymlinks, bool hidden, const std::shared_ptr<CMAddFileTask>& task)
{
    auto f2i = StringConverter::f2i(config);

    std::error_code ec;
    if (!subDir.exists(ec) || !subDir.is_directory(ec)) {
        throw_std_runtime_error("Could not list directory {}: {}", subDir.path().c_str(), ec.message());
    }

    int parentID = database->findObjectIDByPath(subDir.path());
    std::shared_ptr<CdsContainer> parentContainer;

    if (parentID != INVALID_OBJECT_ID) {
        try {
            std::shared_ptr<CdsObject> obj = database->loadObject(parentID);
            if (!obj || !obj->isContainer()) {
                throw_std_runtime_error("Item {} is not a container", parentID);
            }
            parentContainer = std::dynamic_pointer_cast<CdsContainer>(obj);
        } catch (const std::runtime_error& e) {
            log_error("addRecursive: Failed to load parent container {}, {}", subDir.path().c_str(), e.what());
        }
    }

    // abort loop if either:
    // no valid directory returned, server is about to shut down, the task is there and was invalidated
    if (task) {
        log_debug("Task valid? [{}], task path: [{}]", task->isValid(), subDir.path().c_str());
    }
    if (!adir) {
        for (std::size_t i = 0; i < autoscanList->size(); i++) {
            auto dir = autoscanList->get(i);
            if (dir) {
                log_debug("AutoscanDir ({}): {}", AutoscanDirectory::mapScanmode(dir->getScanMode()), i);
                if (isSubDir(dir->getLocation(), subDir) && fs::is_directory(dir->getLocation())) {
                    adir = std::move(dir);
                }
            }
        }
    }
    auto dIter = fs::directory_iterator(subDir, ec);
    if (ec) {
        log_error("addRecursive: Failed to iterate {}, {}", subDir.path().c_str(), ec.message());
        return;
    }

    auto lastModifiedCurrentMax = std::chrono::seconds::zero();
    auto currentTme = currentTime();
    auto lastModifiedNewMax = lastModifiedCurrentMax;
    if (adir) {
        lastModifiedCurrentMax = adir->getPreviousLMT(subDir.path(), parentContainer);
        lastModifiedNewMax = lastModifiedCurrentMax;
        adir->setCurrentLMT(subDir.path(), std::chrono::seconds::zero());
    }

    bool firstChild = true;
    std::shared_ptr<CdsObject> firstObject;
    for (auto&& subDirEnt : dIter) {
        auto&& newPath = subDirEnt.path();
        auto&& name = newPath.filename().string();
        if (name[0] == '.' && !hidden) {
            continue;
        }
        if (shutdownFlag || (task && !task->isValid()))
            break;

        if (config->getConfigFilename() == newPath)
            continue;

        // For the Web UI
        if (task) {
            task->setDescription(fmt::format("Importing: {}", newPath.string()));
        }

        try {
            fs::path rootPath = task ? task->getRootPath() : "";
            // check database if parent, process existing
            auto obj = createSingleItem(subDirEnt, rootPath, followSymlinks, (parentID > 0), true, firstChild, adir, task);

            if (obj) {
                firstChild = false;
                auto lwt = toSeconds(subDirEnt.last_write_time(ec));
                if (lastModifiedCurrentMax < lwt && lwt <= currentTme) {
                    lastModifiedNewMax = lwt;
                }
                if (obj->isItem()) {
                    parentID = obj->getParentID();
                    if (!firstObject && obj->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                        firstObject = obj;
                    }
                }
                if (obj->isContainer()) {
                    addRecursive(adir, subDirEnt, followSymlinks, hidden, task);
                }
            }
        } catch (const std::runtime_error& ex) {
            log_warning("skipping {} (ex:{})", newPath.c_str(), ex.what());
        }
    } // dIter

    if (parentID != INVALID_OBJECT_ID && !parentContainer) {
        try {
            std::shared_ptr<CdsObject> obj = database->loadObject(parentID);
            if (!obj || !obj->isContainer()) {
                throw_std_runtime_error("Item {} is not a container", parentID);
            }
            parentContainer = std::static_pointer_cast<CdsContainer>(obj);
        } catch (const std::runtime_error& e) {
            log_error("addRecursive: Failed to load parent container {}, {}", subDir.path().c_str(), e.what());
        }
    }
    finishScan(adir, subDir.path(), parentContainer, lastModifiedNewMax, firstObject);
}

void ContentManager::finishScan(const std::shared_ptr<AutoscanDirectory>& adir, const fs::path& location, const std::shared_ptr<CdsContainer>& parent, std::chrono::seconds lmt, const std::shared_ptr<CdsObject>& firstObject)
{
    if (adir) {
        adir->setCurrentLMT(location, lmt > std::chrono::seconds::zero() ? lmt : std::chrono::seconds(1));
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

template <typename T>
void ContentManager::updateCdsObject(const std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnpClass = getValueOrDefault(parameters, "class");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");
    std::string flags = getValueOrDefault(parameters, "flags");

    log_error("updateCdsObject: CdsObject {} not updated", title);
}

template <>
void ContentManager::updateCdsObject(const std::shared_ptr<CdsContainer>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnpClass = getValueOrDefault(parameters, "class");
    std::string flags = getValueOrDefault(parameters, "flags");

    log_debug("updateCdsObject: CdsContainer {} updated", title);

    auto clone = CdsObject::createObject(item->getObjectType());
    item->copyTo(clone);

    if (!title.empty())
        clone->setTitle(title);
    if (!upnpClass.empty())
        clone->setClass(upnpClass);
    if (!flags.empty())
        clone->setFlags(CdsObject::makeFlag(flags));
    else
        clone->clearFlag(item->getFlags());

    auto clonedItem = std::static_pointer_cast<CdsContainer>(clone);

    if (!item->equals(clonedItem, true)) {
        clone->validate();
        int containerChanged = INVALID_OBJECT_ID;
        database->updateObject(clone, &containerChanged);
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);
        update_manager->containerChanged(item->getParentID());
        session_manager->containerChangedUI(item->getParentID());
    }
}

template <>
void ContentManager::updateCdsObject(const std::shared_ptr<CdsItem>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnpClass = getValueOrDefault(parameters, "class");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");
    std::string flags = getValueOrDefault(parameters, "flags");
    log_debug("updateCdsObject: CdsItem {} updated", title);

    auto clone = CdsObject::createObject(item->getObjectType());
    item->copyTo(clone);

    if (!title.empty())
        clone->setTitle(title);
    if (!upnpClass.empty())
        clone->setClass(upnpClass);
    if (!location.empty())
        clone->setLocation(location);
    if (!flags.empty())
        clone->setFlags(CdsObject::makeFlag(flags));
    else
        clone->clearFlag(item->getFlags());

    auto clonedItem = std::static_pointer_cast<CdsItem>(clone);

    auto resource = clonedItem->getResource(ContentHandler::DEFAULT);
    std::string protocolInfo;
    if (!protocol.empty()) {
        std::vector<std::string> parts = splitString(protocol, ':');
        if (parts.size() > 1) {
            protocolInfo = protocol;
        }
    }
    if (!mimetype.empty()) {
        clonedItem->setMimeType(mimetype);
    }
    if (protocolInfo.empty()) {
        if (!mimetype.empty() && !protocol.empty()) {
            protocolInfo = renderProtocolInfo(mimetype, protocol);
        } else if (mimetype.empty() && !protocol.empty()) {
            protocolInfo = renderProtocolInfo(clonedItem->getMimeType(), protocol);
        } else if (!mimetype.empty()) {
            std::vector<std::string> parts = splitString(resource->getAttribute(CdsResource::Attribute::PROTOCOLINFO), ':');
            protocol = parts[0];
            protocolInfo = renderProtocolInfo(mimetype, protocol);
        }
    }
    if (!protocolInfo.empty()) {
        resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, protocolInfo);
    }

    clonedItem->removeMetaData(M_DESCRIPTION);
    if (!description.empty()) {
        clonedItem->addMetaData(M_DESCRIPTION, description);
    }

    log_debug("updateCdsObject: checking equality of item {}", item->getTitle());
    if (!item->equals(clonedItem, true)) {
        clonedItem->validate();
        int containerChanged = INVALID_OBJECT_ID;
        database->updateObject(clone, &containerChanged);
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);
        log_debug("updateObject: calling containerChanged on item {}", item->getTitle());
        update_manager->containerChanged(item->getParentID());
    }
}

void ContentManager::updateObject(int objectID, const std::map<std::string, std::string>& parameters)
{
    auto obj = database->loadObject(objectID);
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item) {
        updateCdsObject(item, parameters);
    } else {
        auto cont = std::dynamic_pointer_cast<CdsContainer>(obj);
        if (cont) {
            updateCdsObject(cont, parameters);
        } else {
            updateCdsObject(obj, parameters);
        }
    }
}

void ContentManager::addObject(const std::shared_ptr<CdsObject>& obj, bool firstChild)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is {}", obj->getParentID());

    database->addObject(obj, &containerChanged);
    log_debug("After adding: parent ID is {}", obj->getParentID());

    update_manager->containerChanged(containerChanged);
    session_manager->containerChangedUI(containerChanged);
    int parentId = obj->getParentID();
    // this is the first entry, so the container is new also, send update for parent of parent
    if (firstChild) {
        firstChild = (database->getChildCount(parentId) == 1);
    }
    if ((parentId != -1) && firstChild) {
        auto parent = database->loadObject(parentId);
        log_debug("Will update parent ID {}", parent->getParentID());
        update_manager->containerChanged(parent->getParentID());
    }

    update_manager->containerChanged(obj->getParentID());
    if (obj->isContainer())
        session_manager->containerChangedUI(obj->getParentID());
}

std::shared_ptr<CdsContainer> ContentManager::addContainer(int parentID, const std::string& title, const std::string& upnpClass)
{
    fs::path cPath = database->buildContainerPath(parentID, escape(title, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR));
    std::vector<std::shared_ptr<CdsObject>> cVec;
    for (auto&& segment : cPath) {
        cVec.push_back(std::make_shared<CdsContainer>(segment.string(), upnpClass));
    }
    addContainerTree(cVec);
    return containerMap[cPath];
}

std::pair<int, bool> ContentManager::addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain)
{
    if (chain.empty()) {
        log_error("Received empty chain");
        return { INVALID_OBJECT_ID, false };
    }
    std::string tree;
    int result = CDS_ID_ROOT;
    std::vector<int> createdIds;
    bool isNew = false;
    int count = 0;
    for (auto&& item : chain) {
        if (item->getTitle().empty()) {
            log_error("Received chain item without title");
            return { INVALID_OBJECT_ID, false };
        }
        tree = fmt::format("{}{}{}", tree, VIRTUAL_CONTAINER_SEPARATOR, escape(item->getTitle(), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR));
        log_debug("Received container chain item {}", tree);
        for (auto&& [key, val] : config->getDictionaryOption(CFG_IMPORT_LAYOUT_MAPPING)) {
            tree = std::regex_replace(tree, std::regex(key), val);
        }
        if (containerMap.find(tree) == containerMap.end()) {
            item->removeMetaData(M_TITLE);
            item->addMetaData(M_TITLE, item->getTitle());
            auto cont = std::dynamic_pointer_cast<CdsContainer>(item);
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

    if (!createdIds.empty()) {
        update_manager->containerChanged(result);
        session_manager->containerChangedUI(result);
    }
    return { result, isNew };
}

void ContentManager::assignFanArt(const std::shared_ptr<CdsContainer>& container, const std::shared_ptr<CdsObject>& origObj, int count)
{
    if (!container)
        return;

    if (origObj && origObj->getMTime() > container->getMTime()) {
        container->setMTime(origObj->getMTime());
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

    if (origObj) {
        if (!fanart && (origObj->isContainer() || (count < config->getIntOption(CFG_IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT) && container->getParentID() != CDS_ID_ROOT && std::distance(location.begin(), location.end()) > config->getIntOption(CFG_IMPORT_RESOURCES_CONTAINERART_MINDEPTH)))) {
            fanart = origObj->getResource(CdsResource::Purpose::Thumbnail);
            if (fanart) {
                if (fanart->getAttribute(CdsResource::Attribute::RESOURCE_FILE).empty()) {
                    fanart->addAttribute(CdsResource::Attribute::FANART_OBJ_ID, fmt::to_string(origObj->getID() != INVALID_OBJECT_ID ? origObj->getID() : origObj->getRefID()));
                    fanart->addAttribute(CdsResource::Attribute::FANART_RES_ID, fmt::to_string(fanart->getResId()));
                }
                container->addResource(fanart);
            }
            database->updateObject(container, nullptr);
        }
        containersWithFanArt[container->getID()] = container;
    }
}

void ContentManager::updateObject(const std::shared_ptr<CdsObject>& obj, bool sendUpdates)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    database->updateObject(obj, &containerChanged);

    if (sendUpdates) {
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);

        update_manager->containerChanged(obj->getParentID());
        if (obj->isContainer())
            session_manager->containerChangedUI(obj->getParentID());
    }
}

std::string ContentManager::mimeTypeToUpnpClass(const std::string& mimeType)
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

std::shared_ptr<CdsObject> ContentManager::createObjectFromFile(const fs::directory_entry& dirEnt, bool followSymlinks, bool allowFifo)
{
    std::error_code ec;

    if (!dirEnt.exists(ec)) {
        log_warning("File or directory does not exist: {} ({})", dirEnt.path().c_str(), ec.message());
        return nullptr;
    }

    if (!followSymlinks && dirEnt.is_symlink())
        return nullptr;

    std::shared_ptr<CdsObject> obj;
    if (isRegularFile(dirEnt, ec) || (allowFifo && dirEnt.is_fifo(ec))) { // item
        /* retrieve information about item and decide if it should be included */
        std::string mimetype = mime->getMimeType(dirEnt.path(), MIMETYPE_DEFAULT);
        if (mimetype.empty()) {
            return nullptr;
        }
        log_debug("Mime '{}' for file {}", mimetype, dirEnt.path().c_str());

        std::string upnpClass = mimeTypeToUpnpClass(mimetype);
        if (upnpClass.empty()) {
            std::string contentType = getValueOrDefault(mimetypeContenttypeMap, mimetype);
            if (contentType == CONTENT_TYPE_OGG) {
                upnpClass = isTheora(dirEnt.path())
                    ? UPNP_CLASS_VIDEO_ITEM
                    : UPNP_CLASS_MUSIC_TRACK;
            }
        }
        log_debug("UpnpClass '{}' for file {}", upnpClass, dirEnt.path().c_str());

        auto item = std::make_shared<CdsItem>();
        obj = item;
        item->setLocation(dirEnt.path());
        item->setMTime(toSeconds(dirEnt.last_write_time(ec)));
        item->setSizeOnDisk(getFileSize(dirEnt));

        if (!mimetype.empty()) {
            item->setMimeType(mimetype);
        }
        if (!upnpClass.empty()) {
            item->setClass(upnpClass);
        }

        auto f2i = StringConverter::f2i(config);
        auto title = dirEnt.path().filename().string();
        if (config->getBoolOption(CFG_IMPORT_READABLE_NAMES) && upnpClass != UPNP_CLASS_ITEM) {
            title = dirEnt.path().stem().string();
            std::replace(title.begin(), title.end(), '_', ' ');
        }
        obj->setTitle(f2i->convert(title));

        MetadataHandler::extractMetaData(context, shared_from_this(), item, dirEnt);
        updateItemData(item, mimetype);
    } else if (dirEnt.is_directory(ec)) {
        obj = std::make_shared<CdsContainer>();
        /* adding containers is done by Database now
         * this exists only to inform the caller that
         * this is a container
         */
        /*
        cont->setMTime(dirEnt.last_write_time(ec));
        cont->setLocation(path);
        auto f2i = StringConverter::f2i();
        obj->setTitle(f2i->convert(filename));
        */
    } else {
        // only regular files and directories are supported
        throw_std_runtime_error("ContentManager: skipping file {}", dirEnt.path().c_str());
    }
    if (ec) {
        log_error("File or directory cannot be read: {} ({})", dirEnt.path().c_str(), ec.message());
    }
    return obj;
}

void ContentManager::updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype)
{
    if (item->getMetaData(M_DATE).empty())
        item->addMetaData(M_DATE, fmt::format("{:%FT%T%z}", fmt::localtime(item->getMTime().count())));
    for (auto&& upnpPattern : upnpMap) {
        if (upnpPattern.isMatch(item, mimetype)) {
            item->setClass(upnpPattern.upnpClass);
        }
    }
}

void ContentManager::initLayout()
{
    if (layoutEnabled && !layout) {
        auto lock = threadRunner->lockGuard("initLayout");
        if (!layout) {
            std::string layoutType = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
            log_debug("virtual layout Type {}", layoutType);
            try {
                auto self = shared_from_this();
                if (layoutType == LAYOUT_TYPE_JS) {
#ifdef HAVE_JS
                    layout = std::make_shared<JSLayout>(self, scripting_runtime);
#else
                    log_error("Cannot init virtual layout:\nGerbera compiled without JavaScript support, but JavaScript was requested.");
#endif
                } else if (layoutType == LAYOUT_TYPE_BUILTIN) {
                    layout = std::make_shared<BuiltinLayout>(self);
                }
            } catch (const std::runtime_error& e) {
                layout = nullptr;
                log_error("Init virtual container layout failed: {}", e.what());
                throw;
            }
        }
    }
}

#ifdef HAVE_JS
void ContentManager::initJS()
{
    auto self = shared_from_this();
    if (!playlist_parser_script) {
        playlist_parser_script = std::make_unique<PlaylistParserScript>(self, scripting_runtime);
    }
    if (!metafileParserScript) {
        metafileParserScript = std::make_unique<MetafileParserScript>(self, scripting_runtime);
    }
}

void ContentManager::destroyJS()
{
    playlist_parser_script = nullptr;
    metafileParserScript = nullptr;
}

#endif // HAVE_JS

void ContentManager::destroyLayout()
{
    layout = nullptr;
}

void ContentManager::reloadLayout()
{
    std::scoped_lock<decltype(layoutMutex)> lock(layoutMutex);
    destroyLayout();
#ifdef HAVE_JS
    destroyJS();
#endif // HAVE_JS
    initLayout();
#ifdef HAVE_JS
    initJS();
#endif // HAVE_JS
}

void ContentManager::threadProc()
{
    std::shared_ptr<GenericTask> task;
    ThreadRunner<std::condition_variable_any, std::recursive_mutex>::waitFor("ContentManager", [this] { return threadRunner != nullptr; });
    auto lock = threadRunner->uniqueLockS("threadProc");

    // tell run() that we are ready
    threadRunner->setReady();

    working = true;
    while (!shutdownFlag) {
        currentTask = nullptr;

        task = nullptr;
        if (!taskQueue1.empty()) {
            task = std::move(taskQueue1.front());
            taskQueue1.pop_front();
        } else if (!taskQueue2.empty()) {
            task = std::move(taskQueue2.front());
            taskQueue2.pop_front();
        }

        if (!task) {
            working = false;
            /* if nothing to do, sleep until awakened */
            threadRunner->wait(lock);
            working = true;
            continue;
        }

        currentTask = std::move(task);
        lock.unlock();

        // log_debug("content manager Async START {}", task->getDescription());
        try {
            if (currentTask->isValid())
                currentTask->run();
        } catch (const ServerShutdownException&) {
            shutdownFlag = true;
        } catch (const std::runtime_error& e) {
            log_error("Exception caught: {}", e.what());
        }
        // log_debug("content manager ASYNC STOP  {}", task->getDescription());

        if (!shutdownFlag) {
            lock.lock();
        }
    }

    database->threadCleanup();
}

void ContentManager::addTask(std::shared_ptr<GenericTask> task, bool lowPriority)
{
    auto lock = threadRunner->lockGuard("addTask");

    task->setID(taskID++);

    if (!lowPriority)
        taskQueue1.push_back(std::move(task));
    else
        taskQueue2.push_back(std::move(task));
    threadRunner->notify();
}

int ContentManager::addFile(const fs::directory_entry& dirEnt, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
{
    fs::path rootpath;
    if (dirEnt.is_directory())
        rootpath = dirEnt.path();
    return addFileInternal(dirEnt, rootpath, asSetting, async, lowPriority, 0, cancellable);
}

int ContentManager::addFile(const fs::directory_entry& dirEnt, const fs::path& rootpath, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
{
    return addFileInternal(dirEnt, rootpath, asSetting, async, lowPriority, 0, cancellable);
}

int ContentManager::addFileInternal(
    const fs::directory_entry& dirEnt, const fs::path& rootpath, AutoScanSetting& asSetting, bool async, bool lowPriority, unsigned int parentTaskID, bool cancellable)
{
    if (async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMAddFileTask>(self, dirEnt, rootpath, asSetting, cancellable);
        task->setDescription(fmt::format("Importing: {}", dirEnt.path().string()));
        task->setParentID(parentTaskID);
        addTask(std::move(task), lowPriority);
        return INVALID_OBJECT_ID;
    }
    return _addFile(dirEnt, rootpath, asSetting);
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(service_type_t serviceType, bool lowPriority, bool cancellable, bool unscheduledRefresh)
{
    auto service = online_services->getService(serviceType);
    if (!service) {
        log_debug("No surch service! {}", serviceType);
        throw_std_runtime_error("Service not found");
    }

    unsigned int parentTaskID = 0;

    auto self = shared_from_this();
    auto task = std::make_shared<CMFetchOnlineContentTask>(self, task_processor, timer, service, layout, cancellable, unscheduledRefresh);
    task->setDescription(fmt::format("Updating content from {}", service->getServiceName()));
    task->setParentID(parentTaskID);
    service->incTaskCount();
    addTask(std::move(task), lowPriority);
}

void ContentManager::cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service)
{
    log_debug("Finished fetch cycle for service: {}", service->getServiceName());

    if (service->getItemPurgeInterval() > std::chrono::seconds::zero()) {
        auto ids = database->getServiceObjectIDs(service->getDatabasePrefix());

        auto current = currentTime();
        std::chrono::seconds last = {};
        std::string temp;

        for (int objectId : ids) {
            auto obj = database->loadObject(objectId);
            if (!obj)
                continue;

            temp = obj->getAuxData(ONLINE_SERVICE_LAST_UPDATE);
            if (temp.empty())
                continue;

            last = std::chrono::seconds(std::stoll(temp));

            if ((service->getItemPurgeInterval() > std::chrono::seconds::zero()) && ((current - last) > service->getItemPurgeInterval())) {
                log_debug("Purging old online service object {}", obj->getTitle());
                removeObject(nullptr, objectId, "", false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path)
{
    if (t->getType() == AddFile) {
        auto addTask = std::static_pointer_cast<CMAddFileTask>(t);
        log_debug("comparing, task path: {}, remove path: {}", addTask->getPath().c_str(), path.c_str());
        if (isSubDir(addTask->getPath(), path)) {
            log_debug("Invalidating task with path {}", addTask->getPath().c_str());
            addTask->invalidate();
        }
    }
}

void ContentManager::invalidateTask(unsigned int taskID, task_owner_t taskOwner)
{
    if (taskOwner == ContentManagerTask) {
        auto lock = threadRunner->lockGuard("invalidateTask");
        auto tc = getCurrentTask();
        if (tc && ((tc->getID() == taskID) || (tc->getParentID() == taskID)))
            tc->invalidate();

        for (auto&& t1 : taskQueue1) {
            if ((t1->getID() == taskID) || (t1->getParentID() == taskID))
                t1->invalidate();
        }

        for (auto&& t2 : taskQueue2) {
            if ((t2->getID() == taskID) || (t2->getParentID() == taskID))
                t2->invalidate();
        }
    }
#ifdef ONLINE_SERVICES
    else if (taskOwner == TaskProcessorTask)
        task_processor->invalidateTask(taskID);
#endif
}

std::vector<int> ContentManager::removeObject(const std::shared_ptr<AutoscanDirectory>& adir, int objectID, const fs::path& path, bool rescanResource, bool async, bool all)
{
    if (async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMRemoveObjectTask>(self, adir, objectID, path, rescanResource, all);
        std::shared_ptr<CdsObject> obj;

        try {
            obj = database->loadObject(objectID);
        } catch (const std::runtime_error&) {
            log_debug("Trying to remove an object ID which is no longer in the database! {}", objectID);
            return {};
        }

        if (obj->isContainer()) {
            cleanupTasks(path);
        }

        addTask(std::move(task));
        return {};
    }
    return _removeObject(adir, objectID, path, rescanResource, all);
}

void ContentManager::cleanupTasks(const fs::path& path)
{
    if (path.empty())
        return;

    // make sure to remove possible child autoscan directories from the scanlist
    std::shared_ptr<AutoscanList> rmList = autoscanList->removeIfSubdir(path);
    for (std::size_t i = 0; i < rmList->size(); i++) {
        auto dir = rmList->get(i);
        if (dir->getScanMode() == AutoscanDirectory::ScanMode::Timed) {
            timer->removeTimerSubscriber(this, rmList->get(i)->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
        } else if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
            inotify->unmonitor(dir);
#endif
        }
    }

    auto lock = threadRunner->lockGuard("removeObject " + path.string());

    // we have to make sure that a currently running autoscan task will not
    // launch add tasks for directories that anyway are going to be deleted
    for (auto&& t : taskQueue1) {
        invalidateAddTask(t, path);
    }

    for (auto&& t : taskQueue2) {
        invalidateAddTask(t, path);
    }

    auto t = getCurrentTask();
    if (t) {
        invalidateAddTask(t, path);
    }
}

void ContentManager::rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, fs::path descPath, bool cancellable)
{
    auto self = shared_from_this();
    auto task = std::make_shared<CMRescanDirectoryTask>(self, adir, objectId, cancellable);

    adir->incTaskCount();

    // building container path for the description
    if (descPath.empty())
        descPath = adir->getLocation();

    task->setDescription(fmt::format("Scan: {}", descPath.string()));
    addTask(std::move(task), true); // adding with low priority
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, AutoscanDirectory::ScanMode scanMode) const
{
    return autoscanList->get(scanID);
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int objectID) const
{
    return database->getAutoscanDirectory(objectID);
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(const fs::path& location) const
{
    return autoscanList->get(location);
}

std::vector<std::shared_ptr<AutoscanDirectory>> ContentManager::getAutoscanDirectories() const
{
    return autoscanList->getArrayCopy();
}

void ContentManager::removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw_std_runtime_error("can not remove autoscan directory - was not an autoscan");

    adir->setTaskCount(-1);
    autoscanList->remove(adir->getScanID());
    database->removeAutoscanDirectory(adir);
    session_manager->containerChangedUI(adir->getObjectID());

    if (adir->getScanMode() == AutoscanDirectory::ScanMode::Timed) {
        // if 3rd parameter is true: won't fail if scanID doesn't exist
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY) && (adir->getScanMode() == AutoscanDirectory::ScanMode::INotify)) {
        inotify->unmonitor(adir);
    }
#endif
}

void ContentManager::handlePeristentAutoscanRemove(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (adir->persistent()) {
        adir->setObjectID(INVALID_OBJECT_ID);
        database->updateAutoscanDirectory(adir);
    } else {
        removeAutoscanDirectory(adir);
    }
}

void ContentManager::handlePersistentAutoscanRecreate(const std::shared_ptr<AutoscanDirectory>& adir)
{
    int id = ensurePathExistence(adir->getLocation());
    adir->setObjectID(id);
    database->updateAutoscanDirectory(adir);
}

void ContentManager::setAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& dir)
{
    // We will have to change this for other scan modes
    auto original = autoscanList->getByObjectID(dir->getObjectID());

    if (original)
        dir->setDatabaseID(original->getDatabaseID());

    database->checkOverlappingAutoscans(dir);

    // adding a new autoscan directory
    if (!original) {
        if (dir->getObjectID() == CDS_ID_FS_ROOT)
            dir->setLocation(FS_ROOT_DIRECTORY);
        else {
            log_debug("objectID: {}", dir->getObjectID());
            auto obj = database->loadObject(dir->getObjectID());
            if (!obj || !obj->isContainer() || obj->isVirtual())
                throw_std_runtime_error("Tried to add an illegal object (id) to the list of the autoscan directories");

            log_debug("location: {}", obj->getLocation().c_str());

            if (obj->getLocation().empty())
                throw_std_runtime_error("Tried to add an illegal object as autoscan - no location information available");

            dir->setLocation(obj->getLocation());
        }
        dir->resetLMT();
        database->addAutoscanDirectory(dir);
        scanDir(dir, true);
        return;
    }

    if (original->getScanMode() == AutoscanDirectory::ScanMode::Timed)
        timer->removeTimerSubscriber(this, original->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY) && (original->getScanMode() == AutoscanDirectory::ScanMode::INotify))
        inotify->unmonitor(original);
#endif

    auto copy = std::make_shared<AutoscanDirectory>();
    original->copyTo(copy);

    copy->setHidden(dir->getHidden());
    copy->setRecursive(dir->getRecursive());
    copy->setInterval(dir->getInterval());
    copy->setScanContent(dir->getScanContent());

    autoscanList->remove(copy->getScanID());

    copy->setScanMode(dir->getScanMode());
    scanDir(copy, original->getScanMode() != copy->getScanMode());
    database->updateAutoscanDirectory(copy);
}

void ContentManager::scanDir(const std::shared_ptr<AutoscanDirectory>& dir, bool updateUI)
{
    autoscanList->add(dir);
    reloadLayout();

    if (dir->getScanMode() == AutoscanDirectory::ScanMode::Timed)
        timerNotify(dir->getTimerParameter());
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY) && (dir->getScanMode() == AutoscanDirectory::ScanMode::INotify))
        inotify->monitor(dir);
#endif

    if (updateUI)
        session_manager->containerChangedUI(dir->getObjectID());
}

void ContentManager::triggerPlayHook(const std::string& group, const std::shared_ptr<CdsObject>& obj)
{
    log_debug("start");

    auto item = std::static_pointer_cast<CdsItem>(obj);
    auto playStatus = item->getPlayStatus();
    if (!playStatus) {
        playStatus = std::make_shared<ClientStatusDetail>(group, item->getID(), 0, 0, 0, 0);
        item->setPlayStatus(playStatus);
    }
    playStatus->increasePlayCount();
    playStatus->setLastPlayed();
    database->savePlayStatus(playStatus);

    bool suppress = config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    log_debug("Marking object {} as played", obj->getTitle());
    if (!suppress)
        updateObject(obj, true);

#ifdef HAVE_LASTFMLIB
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED) && item->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
        last_fm->startedPlaying(item);
    }
#endif
    log_debug("end");
}

CMAddFileTask::CMAddFileTask(std::shared_ptr<ContentManager> content,
    fs::directory_entry dirEnt, fs::path rootpath, AutoScanSetting asSetting, bool cancellable)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , dirEnt(std::move(dirEnt))
    , rootpath(std::move(rootpath))
    , asSetting(std::move(asSetting))
{
    this->cancellable = cancellable;
    this->taskType = AddFile;
    if (this->asSetting.adir)
        this->asSetting.adir->incTaskCount();
}

fs::path CMAddFileTask::getPath() const { return dirEnt.path(); }

fs::path CMAddFileTask::getRootPath() const { return rootpath; }

void CMAddFileTask::run()
{
    log_debug("running add file task with path {} recursive: {}", dirEnt.path().c_str(), asSetting.recursive);
    auto self = shared_from_this();
    content->_addFile(dirEnt, rootpath, asSetting, self);
    if (asSetting.adir) {
        asSetting.adir->decTaskCount();
        if (asSetting.adir->updateLMT()) {
            log_debug("CMAddFileTask::run: Updating last_modified for autoscan directory {}", asSetting.adir->getLocation().c_str());
            content->getContext()->getDatabase()->updateAutoscanDirectory(asSetting.adir);
        }
    }
}

CMRemoveObjectTask::CMRemoveObjectTask(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> adir,
    int objectID, const fs::path& path, bool rescanResource, bool all)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , adir(std::move(adir))
    , objectID(objectID)
    , path(path)
    , all(all)
    , rescanResource(rescanResource)
{
    this->taskType = RemoveObject;
    this->cancellable = false;
}

void CMRemoveObjectTask::run()
{
    content->_removeObject(adir, objectID, path, rescanResource, all);
}

CMRescanDirectoryTask::CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<AutoscanDirectory> adir, int containerId, bool cancellable)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , adir(std::move(adir))
    , containerID(containerId)
{
    this->cancellable = cancellable;
    this->taskType = RescanDirectory;
}

void CMRescanDirectoryTask::run()
{
    if (!adir)
        return;

    auto self = shared_from_this();
    content->_rescanDirectory(adir, containerID, self);
    adir->decTaskCount();
    if (adir->updateLMT()) {
        log_debug("CMRescanDirectoryTask::run: Updating last_modified for autoscan directory {}", adir->getLocation().c_str());
        content->getContext()->getDatabase()->updateAutoscanDirectory(adir);
    }
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> taskProcessor, std::shared_ptr<Timer> timer,
    std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout, bool cancellable, bool unscheduledRefresh)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , task_processor(std::move(taskProcessor))
    , timer(std::move(timer))
    , service(std::move(service))
    , layout(std::move(layout))
    , unscheduled_refresh(unscheduledRefresh)
{
    this->cancellable = cancellable;
    this->taskType = FetchOnlineContent;
}

void CMFetchOnlineContentTask::run()
{
    if (!this->service) {
        log_debug("Received invalid service!");
        return;
    }
    try {
        auto t = std::make_shared<TPFetchOnlineContentTask>(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh);
        task_processor->addTask(std::move(t));
    } catch (const std::runtime_error& ex) {
        log_error("{}", ex.what());
    }
}
#endif // ONLINE_SERVICES

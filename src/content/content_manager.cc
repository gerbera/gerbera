/*MT*

    MediaTomb - http://www.mediatomb.cc/

    content_manager.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::content

#include "content_manager.h" // API

#include "autoscan_list.h"
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "cm_task.h"
#include "config/config_option_enum.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "context.h"
#include "database/database.h"
#include "exceptions.h"
#include "import_service.h"
#include "metadata/metadata_service.h"
#include "update_manager.h"
#include "upnp/clients.h"
#include "util/generic_task.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/timer.h"
#include "util/tools.h"
#include "web/session_manager.h"

#ifdef HAVE_JS
#include "scripting/metafile_parser_script.h"
#include "scripting/playlist_parser_script.h"
#include "scripting/scripting_runtime.h"
#endif

#ifdef HAVE_LASTFMLIB
#include "onlineservice/lastfm_scrobbler.h"
#endif

#ifdef ONLINE_SERVICES
#include "onlineservice/online_service.h"
#include "onlineservice/task_processor.h"
#endif

#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#endif

#include <algorithm>
#include <fmt/chrono.h>
#include <regex>

ContentManager::ContentManager(const std::shared_ptr<Context>& context,
    const std::shared_ptr<Server>& server, std::shared_ptr<Timer> timer)
    : config(context->getConfig())
    , mime(context->getMime())
    , database(context->getDatabase())
    , session_manager(context->getSessionManager())
    , converterManager(context->getConverterManager())
    , context(context)
    , timer(std::move(timer))
#ifdef HAVE_JS
    , scriptingRuntime(std::make_shared<ScriptingRuntime>())
#endif
#ifdef HAVE_LASTFMLIB
    , last_fm(std::make_shared<LastFm>(context))
#endif
{
    update_manager = std::make_shared<UpdateManager>(config, database, server);
#ifdef ONLINE_SERVICES
    task_processor = std::make_shared<TaskProcessor>(config);
#endif
    importService = std::make_shared<ImportService>(this->context, converterManager);
    importMode = EnumOption<ImportMode>::getEnumOption(config, ConfigVal::IMPORT_LAYOUT_MODE);
}

void ContentManager::run()
{
    auto self = shared_from_this();
    importService->run(self);

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

    autoscanList = database->getAutoscanList(AutoscanScanMode::Timed);
    for (auto& dir : config->getAutoscanListOption(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST)) {
        fs::path path = dir->getLocation();
        if (fs::is_directory(path)) {
            dir->setObjectID(ensurePathExistence(path));
        }
        try {
            autoscanList->add(dir);
        } catch (const std::logic_error& e) {
            log_warning(e.what());
        } catch (const std::runtime_error& e) {
            // Work around existing config sourced autoscans that were stored to the DB for reasons
            log_warning(e.what());
        }
    }

#ifdef HAVE_INOTIFY
    inotify = std::make_unique<AutoscanInotify>(self);

    if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)) {
        try {
            auto db = database->getAutoscanList(AutoscanScanMode::INotify);
            for (std::size_t i = 0; i < db->size(); i++) {
                auto dir = db->get(i);
                auto path = dir->getLocation();
                if (fs::is_directory(path)) {
                    dir->setObjectID(ensurePathExistence(path));
                }
                try {
                    dir->setInvalid();
                    autoscanList->add(dir);
                } catch (const std::logic_error& e) {
                    log_warning(e.what());
                } catch (const std::runtime_error& e) {
                    // Work around existing config sourced autoscans that were stored to the DB for reasons
                    log_warning(e.what());
                }
            }
        } catch (const std::logic_error& e) {
            log_warning(e.what());
        } catch (const std::runtime_error& e) {
            log_warning(e.what());
        }
        for (const auto& dir : config->getAutoscanListOption(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST)) {
            fs::path path = dir->getLocation();
            if (fs::is_directory(path)) {
                dir->setObjectID(ensurePathExistence(path));
            }
            try {
                autoscanList->add(dir);
            } catch (const std::logic_error& e) {
                log_warning(e.what());
            } catch (const std::runtime_error& e) {
                // Work around existing config sourced autoscans that were stored to the DB for reasons
                log_warning(e.what());
            }
        }
    }

    // Start INotify thread
    inotify->run();
#endif

    auto layoutType = EnumOption<LayoutType>::getEnumOption(config, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    log_debug("virtual layout Type {}", layoutType);
    layoutEnabled = (layoutType == LayoutType::Builtin || layoutType == LayoutType::Js);

#ifdef ONLINE_SERVICES
    online_services = std::make_unique<OnlineServiceList>();
#endif // ONLINE_SERVICES

    initLayout();

    log_debug("autoscan_timed");
    autoscanList->notifyAll(this);
    auto self_content = std::dynamic_pointer_cast<Content>(self);
#ifdef HAVE_INOTIFY
    autoscanList->initTimer(self_content, timer, config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY), inotify);
#else
    autoscanList->initTimer(self_content, timer);
#endif
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

    if (parameter->whoami() == Timer::TimerParamType::IDAutoscan) {
        auto adir = autoscanList ? autoscanList->get(parameter->getID()) : nullptr;

        // do not rescan while other scans are still active
        if (!adir || adir->getActiveScanCount() > 0 || adir->getTaskCount() > 0)
            return;

        rescanDirectory(adir, adir->getObjectID());
    }
#ifdef ONLINE_SERVICES
    else if (parameter->whoami() == Timer::TimerParamType::IDOnlineContent) {
        fetchOnlineContent(static_cast<OnlineServiceType>(parameter->getID()));
    }
#endif // ONLINE_SERVICES
}

void ContentManager::shutdown()
{
    log_debug("start");
    auto lock = threadRunner->uniqueLock();

    destroyLayout();

    log_debug("updating last_modified data for autoscan in database...");
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

        autoscanList.reset();
#ifdef HAVE_INOTIFY
        inotify.reset();
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
    last_fm.reset();
#endif
#ifdef HAVE_JS
    scriptingRuntime.reset();
#endif
#ifdef ONLINE_SERVICES
    task_processor->shutdown();
    task_processor.reset();
#endif
    update_manager->shutdown();
    update_manager.reset();

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

std::shared_ptr<ImportService> ContentManager::getImportService(const std::shared_ptr<AutoscanDirectory>& adir)
{
    auto result = adir ? adir->getImportService() : nullptr;
    if (!result)
        return importService;
    return result;
}

void ContentManager::addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allowFifo)
{
    obj->validate();
    fs::path path = obj->getLocation();
    std::error_code ec;
    auto dirEnt = fs::directory_entry(path, ec);

    if (ec || !isRegularFile(dirEnt, ec))
        throw_std_runtime_error("Not a file: {} - {}", path.c_str(), ec.message());

    auto pcdir = database->findObjectByPath(path, UNUSED_CLIENT_GROUP);
    if (!pcdir) {
        auto adir = findAutoscanDirectory(path);
        pcdir = createObjectFromFile(adir, dirEnt, true, allowFifo);
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
    const std::shared_ptr<CdsContainer>& parent,
    const fs::path& rootPath,
    bool followSymlinks,
    bool checkDatabase,
    bool processExisting,
    bool firstChild,
    const std::shared_ptr<AutoscanDirectory>& adir,
    const std::shared_ptr<CMAddFileTask>& task)
{
    auto obj = checkDatabase ? database->findObjectByPath(dirEnt.path(), DEFAULT_CLIENT_GROUP) : nullptr;
    bool isNew = false;

    if (!obj) {
        obj = createObjectFromFile(adir, dirEnt, followSymlinks);
        if (!obj) { // object ignored
            log_debug("Hidden file or directory ignored: {}", dirEnt.path().c_str());
            return obj;
        }
        if (obj->isItem()) {
            addObject(obj, firstChild); // creates parent containers if necessary and sets parentID
            isNew = true;
        } else {
            // addObject(obj, false); // creates parent containers if necessary and sets parentID
        }
    } else if (obj->isItem() && processExisting) {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        getImportService(adir)->getMetadataService()->extractMetaData(item, dirEnt);
        getImportService(adir)->updateItemData(item, item->getMimeType());
    }
    if (processExisting || isNew) {
        getImportService(adir)->fillSingleLayout(nullptr, obj, parent, task);
    }
    return obj;
}

void ContentManager::parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const
{
    importService->parseMetafile(obj, path);
}

std::shared_ptr<CdsObject> ContentManager::_addFile(
    const fs::directory_entry& dirEnt,
    fs::path rootPath,
    AutoScanSetting& asSetting,
    const std::shared_ptr<CMAddFileTask>& task)
{
    if (!asSetting.hidden && dirEnt.path().is_relative()) {
        return nullptr;
    }

    // never add the server configuration file
    if (config->getConfigFilename() == dirEnt.path()) {
        return nullptr;
    }

    if (rootPath.empty() && task)
        rootPath = task->getRootPath();

    if (importMode == ImportMode::Gerbera) {
        std::unordered_set<int> currentContent;
        if (asSetting.changedObject)
            importService->clearCache(); // may be called by layout
        getImportService(asSetting.adir)->doImport(dirEnt.path(), asSetting, currentContent, task);

        return getImportService(asSetting.adir)->getObject(dirEnt.path());
    }
    // checkDatabase, don't process existing
    std::shared_ptr<CdsContainer> parent;
    auto parentObject = database->findObjectByPath(dirEnt.path().parent_path(), UNUSED_CLIENT_GROUP, DbFileType::Directory);
    if (parentObject && parentObject->isContainer())
        parent = std::dynamic_pointer_cast<CdsContainer>(parentObject);
    auto obj = createSingleItem(dirEnt, parent, rootPath, asSetting.followSymlinks, true, false, false, asSetting.adir, task);
    if (!obj) // object ignored
        return obj;

    if (asSetting.recursive && obj->isContainer())
        addRecursive(asSetting.adir, dirEnt, std::dynamic_pointer_cast<CdsContainer>(obj), asSetting.followSymlinks, asSetting.hidden, task);

    if (asSetting.rescanResource && obj->hasResource(ContentHandler::RESOURCE)) {
        std::string parentPath = dirEnt.path().parent_path();
        updateAttachedResources(asSetting.adir, obj, parentPath, true);
    }

    return obj;
}

bool ContentManager::updateAttachedResources(const std::shared_ptr<AutoscanDirectory>& adir, const std::shared_ptr<CdsObject>& obj, const fs::path& parentPath, bool all)
{
    if (!adir)
        return false;

    bool parentRemoved = false;
    auto parentObject = database->findObjectByPath(parentPath, UNUSED_CLIENT_GROUP, DbFileType::Auto);
    if (parentObject) {
        // as there is no proper way to force refresh of unchanged files, delete whole dir and rescan it
        _removeObject(adir, parentObject, parentPath, false, all);
        // in order to rescan whole directory we have to set lmt to a very small value
        AutoScanSetting asSetting;
        asSetting.adir = adir;
        adir->setCurrentLMT(parentPath, std::chrono::seconds(1));
        asSetting.followSymlinks = adir->getFollowSymlinks();
        asSetting.hidden = adir->getHidden();
        asSetting.recursive = true;
        asSetting.rescanResource = false;
        asSetting.async = true;
        asSetting.mergeOptions(config, parentPath);
        std::error_code ec;
        auto dirEntry = fs::directory_entry(parentPath, ec);
        if (!ec) {
            // addFile(const fs::directory_entry& path, AutoScanSetting& asSetting, bool lowPriority, bool cancellable)
            addFile(dirEntry, asSetting, true, false);
            log_debug("Forced rescan of {} for resource {}", parentPath.c_str(), obj->getLocation().c_str());
            parentRemoved = true;
        } else {
            log_error("Failed to read {}: {}", parentPath.c_str(), ec.message());
        }
    }
    return parentRemoved;
}

std::vector<int> ContentManager::_removeObject(
    const std::shared_ptr<AutoscanDirectory>& adir,
    const std::shared_ptr<CdsObject>& obj,
    const fs::path& path,
    bool rescanResource,
    bool all)
{
    if (!obj || obj->getID() == INVALID_OBJECT_ID)
        return {};

    int objectID = obj->getID();
    if (objectID == CDS_ID_ROOT)
        throw_std_runtime_error("cannot remove root container");
    if (objectID == CDS_ID_FS_ROOT)
        throw_std_runtime_error("cannot remove PC-Directory container");
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw_std_runtime_error("tried to remove illegal object id");

    bool parentRemoved = false;

    if (rescanResource) {
        if (obj->hasResource(ContentHandler::RESOURCE)) {
            auto parentPath = obj->getLocation().parent_path();
            parentRemoved = updateAttachedResources(adir, obj, parentPath, all);
        }
    }

    if (obj->isContainer()) {
        cleanupTasks(path);
    }
    // Removing a file can lead to virtual directories to drop empty and be removed
    // So current container cache must be invalidated
    importService->clearCache();
    getImportService(adir)->clearCache();

    if (!parentRemoved) {
        auto changedContainers = database->removeObject(objectID, obj->getLocation(), all);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
            return changedContainers->upnp;
        }
    }

    if (rescanResource && parentRemoved)
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
    std::shared_ptr<CdsObject> obj;

    if (containerID != INVALID_OBJECT_ID) {
        try {
            obj = database->loadObject(containerID);
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

        obj = database->loadObject(containerID);
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
            removeObject(adir, obj, location, false);
            if (location == adir->getLocation()) {
                adir->setObjectID(INVALID_OBJECT_ID);
                database->updateAutoscanDirectory(adir);
            }
            return;
        }

        if (location == adir->getLocation()) {
            removeObject(adir, obj, location, false);
            removeAutoscanDirectory(adir);
        }
        return;
    }

    AutoScanSetting asSetting;
    asSetting.adir = adir;
    asSetting.recursive = adir->getRecursive();
    asSetting.followSymlinks = adir->getFollowSymlinks();
    asSetting.hidden = adir->getHidden();
    asSetting.mergeOptions(config, location);

    log_debug("Rescanning options {}: recursive={} hidden={} followSymlinks={}", location.c_str(), asSetting.recursive, asSetting.hidden, asSetting.followSymlinks);

    // request only items if non-recursive scan is wanted
    auto list = std::unordered_set<int>();
    database->getObjects(containerID, !asSetting.recursive || importMode != ImportMode::Gerbera, list, importMode == ImportMode::Gerbera);

    unsigned int thisTaskID;
    if (task) {
        thisTaskID = task->getID();
    } else {
        thisTaskID = 0;
    }

    if (importMode == ImportMode::Gerbera) {
        if (asSetting.changedObject)
            importService->clearCache(); // may be called by layout
        getImportService(adir)->doImport(location, asSetting, list, task);
    } else {
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
                getImportService(adir)->finishScan(location, parentContainer, lastModifiedNewMax);
                return;
            }

            if (!asSetting.followSymlinks && dirEnt.is_symlink()) {
                auto object = database->findObjectByPath(newPath, UNUSED_CLIENT_GROUP);
                if (object) {
                    list.erase(object->getID());
                    removeObject(adir, object, newPath, false);
                }
                log_debug("link {} skipped", newPath.c_str());
                continue;
            }

            asSetting.recursive = adir->getRecursive();
            asSetting.followSymlinks = adir->getFollowSymlinks();
            asSetting.hidden = adir->getHidden();
            asSetting.mergeOptions(config, location);
            auto lwt = toSeconds(dirEnt.last_write_time(ec));

            if (isRegularFile(dirEnt, ec)) {
                auto newObject = database->findObjectByPath(newPath, UNUSED_CLIENT_GROUP);
                if (newObject) {
                    list.erase(newObject->getID());

                    // check modification time and update file if changed
                    if (lastModifiedCurrentMax < lwt) {
                        // re-add object - we have to do this in order to trigger
                        // layout
                        auto removedParents = removeObject(adir, newObject, newPath, false, false);
                        if (std::find_if(removedParents.begin(), removedParents.end(), [=](auto&& pId) { return containerID == pId; }) != removedParents.end()) {
                            // parent purged
                            parentContainer = nullptr;
                        }
                        asSetting.recursive = false;
                        asSetting.rescanResource = false;
                        asSetting.async = false;
                        newObject = addFileInternal(dirEnt, rootpath, asSetting);
                        // update time variable
                        if (lastModifiedNewMax < lwt && lwt <= currentTme)
                            lastModifiedNewMax = lwt;
                    }
                } else {
                    // add file, not recursive, not async, not forced
                    asSetting.recursive = false;
                    asSetting.rescanResource = false;
                    asSetting.async = false;
                    newObject = addFileInternal(dirEnt, rootpath, asSetting);
                    if (lastModifiedNewMax < lwt && lwt <= currentTme)
                        lastModifiedNewMax = lwt;
                }
                if (!firstObject && newObject) {
                    firstObject = newObject;
                    if (!firstObject->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                        firstObject = nullptr;
                    }
                }
            } else if (dirEnt.is_directory(ec) && asSetting.recursive) {
                auto newObject = database->findObjectByPath(newPath, UNUSED_CLIENT_GROUP);
                if (lastModifiedNewMax < lwt && lwt <= currentTme)
                    lastModifiedNewMax = lwt;
                if (newObject) {
                    log_debug("rescanSubDirectory {}", newPath.c_str());
                    list.erase(newObject->getID());
                    // add a task to rescan the directory that was found
                    rescanDirectory(adir, newObject->getID(), newPath, task->isCancellable());
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
                        getImportService(adir)->finishScan(location, parentContainer, lastModifiedNewMax);
                        return;
                    }
                    // add directory, recursive, async, hidden flag, low priority
                    asSetting.recursive = true;
                    asSetting.rescanResource = false;
                    asSetting.async = true;
                    asSetting.mergeOptions(config, newPath);
                    // const fs::path& path, const fs::path& rootpath, AutoScanSetting& asSetting, bool lowPriority, unsigned int parentTaskID, bool cancellable
                    addFileInternal(dirEnt, rootpath, asSetting, true, thisTaskID, task->isCancellable());
                    log_debug("addSubDirectory {} done", newPath.c_str());
                }
            }
            if (!parentContainer && !location.empty()) {
                std::shared_ptr<CdsObject> obj = database->findObjectByPath(location, UNUSED_CLIENT_GROUP, DbFileType::Directory);
                if (!obj || !obj->isContainer()) {
                    log_error("Updated parent {} is not available or no container", location.string());
                } else {
                    parentContainer = std::dynamic_pointer_cast<CdsContainer>(obj);
                    containerID = parentContainer->getID();
                }
            }
            if (ec) {
                log_error("ContentManager::_rescanDirectory {}: Failed to read {}, {}", location.c_str(), newPath.c_str(), ec.message());
            }
        } // dIter

        getImportService(adir)->finishScan(location, parentContainer, lastModifiedNewMax, firstObject);
    }
    if (shutdownFlag || (task && !task->isValid())) {
        return;
    }
    // Items not touched during import do not exist anymore and can be removed
    if (!list.empty()) {
        log_debug("Deleting unreferenced physical objects {}", fmt::join(list, ","));
        auto changedContainers = database->removeObjects(list);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
    // Find all virtual items that do not have physical reps
    list = database->getUnreferencedObjects();
    if (!list.empty()) {
        // DELETE FROM mt_cds_object WHERE ref_id is not null and ref_id not in (SELECT id FROM mt_cds_object)
        log_debug("Deleting unreferenced virtual objects {}", fmt::join(list, ","));
        auto changedContainers = database->removeObjects(list);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
}

bool ContentManager::isHiddenFile(const fs::directory_entry& dirEntry, bool isDirectory, const AutoScanSetting& settings)
{
    return getImportService(settings.adir)->isHiddenFile(dirEntry.path(), isDirectory, dirEntry, settings);
}

std::shared_ptr<AutoscanDirectory> ContentManager::findAutoscanDirectory(fs::path path) const
{
    std::shared_ptr<AutoscanDirectory> autoscanDir;
    path.remove_filename();
    std::error_code ec;

    for (std::size_t i = 0; i < autoscanList->size(); i++) {
        auto dir = autoscanList->get(i);
        if (dir) {
            log_debug("AutoscanDir ({}): {}", AutoscanDirectory::mapScanmode(dir->getScanMode()), i);
            if (isSubDir(path, dir->getLocation()) && fs::is_directory(dir->getLocation(), ec)) {
                autoscanDir = std::move(dir);
            }
            if (ec) {
                log_warning("No AutoscanDir {} has problems '{}'", dir->getLocation().string(), ec.message());
            }
        }
    }
    if (!autoscanDir) {
        log_warning("No AutoscanDir found for {}", path.string());
    }

    return autoscanDir;
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(
    std::shared_ptr<AutoscanDirectory>& adir,
    const fs::directory_entry& subDir,
    const std::shared_ptr<CdsContainer>& parentContainer,
    bool followSymlinks,
    bool hidden,
    const std::shared_ptr<CMAddFileTask>& task)
{
    auto f2i = converterManager->f2i();

    std::error_code ec;
    if (!subDir.exists(ec) || !subDir.is_directory(ec)) {
        throw_std_runtime_error("Could not list directory {}: {}", subDir.path().c_str(), ec.message());
    }

    int parentID = parentContainer->getID();

    if (!parentContainer || !parentContainer->isContainer()) {
        log_error("addRecursive: Failed to load parent container of {}", subDir.path().c_str());
        return;
    }

    // abort loop if either:
    // no valid directory returned, server is about to shut down, the task is there and was invalidated
    if (task) {
        log_debug("Task valid? [{}], task path: [{}]", task->isValid(), subDir.path().c_str());
    }
    if (!adir) {
        adir = findAutoscanDirectory(subDir);
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

    auto list = std::unordered_set<int>();
    database->getObjects(parentID, false, list, false);
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
            auto obj = createSingleItem(subDirEnt, parentContainer, rootPath, followSymlinks, (parentID > 0), true, firstChild, adir, task);

            if (obj) {
                firstChild = false;
                auto lwt = toSeconds(subDirEnt.last_write_time(ec));
                if (lastModifiedCurrentMax < lwt && lwt <= currentTme) {
                    lastModifiedNewMax = lwt;
                }
                list.erase(obj->getID());
                if (obj->isItem()) {
                    parentID = obj->getParentID();
                    if (!firstObject && obj->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
                        firstObject = obj;
                    }
                }
                if (obj->isContainer()) {
                    addRecursive(adir, subDirEnt, std::static_pointer_cast<CdsContainer>(obj), followSymlinks, hidden, task);
                }
            }
        } catch (const std::runtime_error& ex) {
            log_warning("skipping {} (ex:{})", newPath.c_str(), ex.what());
        }
    } // dIter

    getImportService(adir)->finishScan(subDir.path(), parentContainer, lastModifiedNewMax, firstObject);
    // Items not touched during import do not exist anymore and can be removed
    if (!list.empty()) {
        log_debug("Deleting unreferenced physical objects {}", fmt::join(list, ","));
        auto changedContainers = database->removeObjects(list);
        if (changedContainers) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
}

template <typename T>
std::shared_ptr<CdsObject> ContentManager::updateCdsObject(const std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnpClass = getValueOrDefault(parameters, "class");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");
    std::string flags = getValueOrDefault(parameters, "flags");

    log_error("updateCdsObject: CdsObject {} not updated", title);
    return nullptr;
}

template <>
std::shared_ptr<CdsObject> ContentManager::updateCdsObject(const std::shared_ptr<CdsContainer>& item, const std::map<std::string, std::string>& parameters)
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
    return clone;
}

template <>
std::shared_ptr<CdsObject> ContentManager::updateCdsObject(const std::shared_ptr<CdsItem>& item, const std::map<std::string, std::string>& parameters)
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
            std::vector<std::string> parts = splitString(resource->getAttribute(ResourceAttribute::PROTOCOLINFO), ':');
            protocol = parts[0];
            protocolInfo = renderProtocolInfo(mimetype, protocol);
        }
    }
    if (!protocolInfo.empty()) {
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
    }

    clonedItem->removeMetaData(MetadataFields::M_DESCRIPTION);
    if (!description.empty()) {
        clonedItem->addMetaData(MetadataFields::M_DESCRIPTION, description);
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
    return clone;
}

std::shared_ptr<CdsObject> ContentManager::updateObject(int objectID, const std::map<std::string, std::string>& parameters)
{
    auto obj = database->loadObject(objectID);
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item) {
        return updateCdsObject(item, parameters);
    } else {
        auto cont = std::dynamic_pointer_cast<CdsContainer>(obj);
        if (cont) {
            return updateCdsObject(cont, parameters);
        } else {
            return updateCdsObject(obj, parameters);
        }
    }
}

void ContentManager::addObject(const std::shared_ptr<CdsObject>& obj, bool firstChild)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is {}", obj->getParentID());

    database->addObject(obj, &containerChanged);
    int parentId = obj->getParentID();
    log_debug("After adding: parent ID is {}", parentId);

    if (obj->isContainer()) {
        return;
    }
    update_manager->containerChanged(containerChanged);
    session_manager->containerChangedUI(containerChanged);
    // this is the first entry, so the container is new also, send update for parent of parent
    if (firstChild) {
        firstChild = (database->getChildCount(parentId) == 1);
    }
    if (parentId >= CDS_ID_ROOT) {
        if (firstChild) {
            auto parent = database->loadObject(parentId);
            log_debug("Will update parent ID {}", parent->getParentID());
            update_manager->containerChanged(parent->getParentID());
        }
        if (parentId != containerChanged) {
            update_manager->containerChanged(parentId);
            if (obj->isContainer())
                session_manager->containerChangedUI(parentId);
        }
    }
}

std::shared_ptr<CdsContainer> ContentManager::addContainer(int parentID, const std::string& title, const std::string& upnpClass)
{
    fs::path cPath = database->buildContainerPath(parentID, escape(title, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR));
    std::vector<std::shared_ptr<CdsObject>> cVec;
    cVec.reserve(std::distance(cPath.begin(), cPath.end()));
    for (auto&& segment : cPath) {
        cVec.push_back(std::make_shared<CdsContainer>(segment.string(), upnpClass));
    }
    addContainerTree(cVec, nullptr);
    return importService->getContainer(fmt::format("/{}", cPath.string()));
}

std::pair<int, bool> ContentManager::addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain, const std::shared_ptr<CdsObject>& refItem)
{
    if (chain.empty()) {
        log_error("Received empty chain");
        return { INVALID_OBJECT_ID, false };
    }
    std::vector<int> createdIds;

    auto result = importService->addContainerTree(CDS_ID_ROOT, chain, refItem, createdIds);

    if (!createdIds.empty()) {
        update_manager->containerChanged(result.first);
        session_manager->containerChangedUI(result.first);
    }
    return result;
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

std::shared_ptr<CdsObject> ContentManager::createObjectFromFile(const std::shared_ptr<AutoscanDirectory>& adir, const fs::directory_entry& dirEnt, bool followSymlinks, bool allowFifo)
{
    std::error_code ec;

    if (!dirEnt.exists(ec)) {
        log_warning("File or directory does not exist: {} ({})", dirEnt.path().c_str(), ec.message());
        return nullptr;
    }

    if (!followSymlinks && dirEnt.is_symlink())
        return nullptr;

    std::shared_ptr<CdsObject> obj;
    bool skip = false;
    if (isRegularFile(dirEnt, ec) || (allowFifo && dirEnt.is_fifo(ec))) { // item
        auto itemInfo = getImportService(adir)->createSingleItem(dirEnt);
        skip = itemInfo.first;
        obj = itemInfo.second;
    } else if (dirEnt.is_directory(ec)) {
        obj = getImportService(adir)->createSingleContainer(CDS_ID_FS_ROOT, dirEnt, UPNP_CLASS_CONTAINER);
    }
    if (!obj && !skip) {
        // only regular files and directories are supported
        throw_std_runtime_error("ContentManager: skipping file {}", dirEnt.path().c_str());
    }
    if (ec) {
        log_error("File or directory cannot be read: {} ({})", dirEnt.path().c_str(), ec.message());
    }
    return obj;
}

void ContentManager::initLayout()
{
    if (layoutEnabled) {
        auto self = shared_from_this();
        auto lock = threadRunner->lockGuard("initLayout");
        auto layoutType = EnumOption<LayoutType>::getEnumOption(config, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
        importService->initLayout(layoutType);
        for (std::size_t i = 0; i < autoscanList->size(); i++) {
            auto autoscanDir = autoscanList->get(i);

            auto asImportService = std::make_shared<ImportService>(context, converterManager);
            asImportService->run(self, autoscanDir, autoscanDir->getLocation());
            autoscanDir->setImportService(asImportService);
            asImportService->initLayout(layoutType);
        }
    }
}

void ContentManager::destroyLayout()
{
    importService->destroyLayout();
    for (std::size_t i = 0; i < autoscanList->size(); i++) {
        auto autoscanDir = autoscanList->get(i);
        if (autoscanDir) {
            getImportService(autoscanDir)->destroyLayout();
        }
    }
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

        log_debug("content manager Async START {}", currentTask->getDescription());
        try {
            if (currentTask->isValid())
                currentTask->run();
        } catch (const ServerShutdownException&) {
            shutdownFlag = true;
        } catch (const std::runtime_error& e) {
            log_error("Exception caught: {}", e.what());
        }
        log_debug("content manager ASYNC STOP  {}", currentTask->getDescription());

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

std::shared_ptr<CdsObject> ContentManager::addFile(
    const fs::directory_entry& dirEnt,
    AutoScanSetting& asSetting,
    bool lowPriority,
    bool cancellable)
{
    fs::path rootpath;
    if (dirEnt.is_directory())
        rootpath = dirEnt.path();
    return addFileInternal(dirEnt, rootpath, asSetting, lowPriority, 0, cancellable);
}

std::shared_ptr<CdsObject> ContentManager::addFile(
    const fs::directory_entry& dirEnt,
    const fs::path& rootpath,
    AutoScanSetting& asSetting,
    bool lowPriority,
    bool cancellable)
{
    return addFileInternal(dirEnt, rootpath, asSetting, lowPriority, 0, cancellable);
}

std::shared_ptr<CdsObject> ContentManager::addFileInternal(
    const fs::directory_entry& dirEnt,
    const fs::path& rootpath,
    AutoScanSetting& asSetting,
    bool lowPriority,
    unsigned int parentTaskID,
    bool cancellable)
{
    if (asSetting.async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMAddFileTask>(self, dirEnt, rootpath, asSetting, cancellable);
        task->setDescription(fmt::format("Importing: {}", dirEnt.path().string()));
        task->setParentID(parentTaskID);
        addTask(std::move(task), lowPriority);
        return nullptr;
    }
    return _addFile(dirEnt, rootpath, asSetting);
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(OnlineServiceType serviceType, bool lowPriority, bool cancellable, bool unscheduledRefresh)
{
    auto service = online_services->getService(serviceType);
    if (!service) {
        log_debug("No surch service! {}", serviceType);
        throw_std_runtime_error("Service not found");
    }

    unsigned int parentTaskID = 0;

    auto self = shared_from_this();
    auto task = std::make_shared<CMFetchOnlineContentTask>(self, task_processor, timer, service, importService->getLayout(), cancellable, unscheduledRefresh);
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
        std::chrono::seconds last = std::chrono::seconds::zero();
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
                removeObject(nullptr, obj, "", false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path)
{
    if (t->getType() == TaskType::AddFile) {
        auto addTask = std::static_pointer_cast<CMAddFileTask>(t);
        log_debug("comparing, task path: {}, remove path: {}", addTask->getPath().c_str(), path.c_str());
        if (isSubDir(addTask->getPath(), path)) {
            log_debug("Invalidating task with path {}", addTask->getPath().c_str());
            addTask->invalidate();
        }
    }
}

void ContentManager::invalidateTask(unsigned int taskID, TaskOwner taskOwner)
{
    if (taskOwner == TaskOwner::ContentManagerTask) {
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
    else if (taskOwner == TaskOwner::TaskProcessorTask)
        task_processor->invalidateTask(taskID);
#endif
}

std::vector<int> ContentManager::removeObject(
    const std::shared_ptr<AutoscanDirectory>& adir,
    const std::shared_ptr<CdsObject>& obj,
    const fs::path& path,
    bool rescanResource,
    bool async,
    bool all)
{
    if (async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMRemoveObjectTask>(self, adir, obj, path, rescanResource, all);

        if (obj && obj->isContainer()) {
            cleanupTasks(path);
        }

        addTask(std::move(task));
        return {};
    }
    return _removeObject(adir, obj, path, rescanResource, all);
}

void ContentManager::cleanupTasks(const fs::path& path)
{
    if (path.empty())
        return;

    // make sure to remove possible child autoscan directories from the scanlist
    std::shared_ptr<AutoscanList> rmList = autoscanList->removeIfSubdir(path);
    for (std::size_t i = 0; i < rmList->size(); i++) {
        auto dir = rmList->get(i);
        if (dir->getScanMode() == AutoscanScanMode::Timed) {
            timer->removeTimerSubscriber(this, rmList->get(i)->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
        } else if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)) {
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

void ContentManager::rescanDirectory(
    const std::shared_ptr<AutoscanDirectory>& adir,
    int objectId,
    fs::path descPath,
    bool cancellable)
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

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, AutoscanScanMode scanMode) const
{
    return autoscanList->get(scanID);
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int objectID) const
{
    return database->getAutoscanDirectory(objectID);
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(const fs::path& location) const
{
    return autoscanList->getKey(location);
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
    autoscanList->remove(adir);
    database->removeAutoscanDirectory(adir);
    session_manager->containerChangedUI(adir->getObjectID());

    if (adir->getScanMode() == AutoscanScanMode::Timed) {
        // if 3rd parameter is true: won't fail if scanID doesn't exist
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY) && (adir->getScanMode() == AutoscanScanMode::INotify)) {
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
        auto self = shared_from_this();
        auto asImportService = std::make_shared<ImportService>(context, converterManager);
        asImportService->run(self, dir, dir->getLocation());
        dir->setImportService(asImportService);
        auto layoutType = EnumOption<LayoutType>::getEnumOption(config, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
        asImportService->initLayout(layoutType);
        scanDir(dir, true);
        return;
    }

    if (original->getScanMode() == AutoscanScanMode::Timed)
        timer->removeTimerSubscriber(this, original->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY) && (original->getScanMode() == AutoscanScanMode::INotify))
        inotify->unmonitor(original);
#endif

    auto copy = std::make_shared<AutoscanDirectory>();
    original->copyTo(copy);

    copy->setHidden(dir->getHidden());
    copy->setRecursive(dir->getRecursive());
    copy->setInterval(dir->getInterval());
    copy->setScanContent(dir->getScanContent());
    copy->setDirTypes(dir->hasDirTypes());

    if (dir->isValid())
        autoscanList->remove(dir); // remove old version of dir

    scanDir(copy, original->getScanMode() != copy->getScanMode());
    copy->setScanMode(dir->getScanMode());
    database->updateAutoscanDirectory(copy);
}

void ContentManager::scanDir(const std::shared_ptr<AutoscanDirectory>& dir, bool updateUI)
{
    if (dir->isValid())
        autoscanList->add(dir, dir->getScanID());
    else
        autoscanList->add(dir);

    if (dir->getScanMode() == AutoscanScanMode::Timed)
        timerNotify(dir->getTimerParameter());
#ifdef HAVE_INOTIFY
    else if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY) && (dir->getScanMode() == AutoscanScanMode::INotify))
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

    bool suppress = config->getBoolOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && config->getBoolOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    log_debug("Marking object {} as played", obj->getTitle());
    if (!suppress)
        updateObject(obj, true);

#ifdef HAVE_LASTFMLIB
    if (config->getBoolOption(ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED) && item->isSubClass(UPNP_CLASS_AUDIO_ITEM)) {
        last_fm->startedPlaying(item);
    }
#endif
    log_debug("end");
}

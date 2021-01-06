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

#include "content_manager.h" // API

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <regex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "config/config_manager.h"
#include "config/directory_tweak.h"
#include "database/database.h"
#include "layout/fallback_layout.h"
#include "metadata/metadata_handler.h"
#include "update_manager.h"
#include "util/mime.h"
#include "util/process.h"
#include "util/string_converter.h"
#include "util/timer.h"
#include "util/tools.h"
#include "web/session_manager.h"

#ifdef HAVE_JS
#include "layout/js_layout.h"
#endif

#ifdef HAVE_LASTFMLIB
#include "onlineservice/lastfm_scrobbler.h"
#endif

#ifdef SOPCAST
#include "onlineservice/sopcast_service.h"
#endif

#ifdef ATRAILERS
#include "onlineservice/atrailers_service.h"
#endif

#ifdef ONLINE_SERVICES
#include "util/task_processor.h"
#endif

ContentManager::ContentManager(const std::shared_ptr<Config>& config, const std::shared_ptr<Database>& database,
    std::shared_ptr<UpdateManager> update_manager, std::shared_ptr<web::SessionManager> session_manager,
    std::shared_ptr<Timer> timer, std::shared_ptr<TaskProcessor> task_processor,
    std::shared_ptr<Runtime> scripting_runtime, std::shared_ptr<LastFm> last_fm)
    : config(config)
    , database(database)
    , update_manager(std::move(update_manager))
    , session_manager(std::move(session_manager))
    , timer(std::move(timer))
    , task_processor(std::move(task_processor))
    , scripting_runtime(std::move(scripting_runtime))
    , last_fm(std::move(last_fm))
{
    taskID = 1;
    working = false;
    shutdownFlag = false;
    layout_enabled = false;

    ignore_unknown_extensions = config->getBoolOption(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    auto extension_mimetype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    if (ignore_unknown_extensions && (extension_mimetype_map.empty())) {
        log_warning("Ignore unknown extensions set, but no mappings specified");
        log_warning("Please review your configuration!");
        ignore_unknown_extensions = false;
    }

    mimetype_contenttype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    auto config_timed_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    for (size_t i = 0; i < config_timed_list->size(); i++) {
        auto dir = config_timed_list->get(i);
        if (dir != nullptr) {
            fs::path path = dir->getLocation();
            if (fs::is_directory(path)) {
                dir->setObjectID(ensurePathExistence(path));
            }
        }
    }

    database->updateAutoscanList(ScanMode::Timed, config_timed_list);
    autoscan_timed = database->getAutoscanList(ScanMode::Timed);
}

void ContentManager::run()
{
#ifdef HAVE_INOTIFY
    auto self = shared_from_this();
    inotify = std::make_unique<AutoscanInotify>(database, self, config);

    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        auto config_inotify_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);
        for (size_t i = 0; i < config_inotify_list->size(); i++) {
            auto dir = config_inotify_list->get(i);
            if (dir != nullptr) {
                fs::path path = dir->getLocation();
                if (fs::is_directory(path)) {
                    dir->setObjectID(ensurePathExistence(path));
                }
            }
        }

        database->updateAutoscanList(ScanMode::INotify, config_inotify_list);
        autoscan_inotify = database->getAutoscanList(ScanMode::INotify);
    } else {
        // make an empty list so we do not have to do extra checks on shutdown
        autoscan_inotify = std::make_shared<AutoscanList>(database);
    }

    // Start INotify thread
    inotify->run();
#endif

    mime = std::make_shared<Mime>(config);

    std::string layout_type = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    if ((layout_type == "builtin") || (layout_type == "js"))
        layout_enabled = true;

#ifdef ONLINE_SERVICES
    online_services = std::make_unique<OnlineServiceList>();

#ifdef SOPCAST
    if (config->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_ENABLED)) {
        try {
            auto self = shared_from_this();
            auto sc = std::make_shared<SopCastService>(config, database, self);

            int i = config->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);
            sc->setRefreshInterval(i);

            i = config->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);
            sc->setItemPurgeInterval(i);

            if (config->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            auto sc_param = std::make_shared<Timer::Parameter>(Timer::Parameter::IDOnlineContent, OS_SopCast);
            sc->setTimerParameter(sc_param);
            online_services->registerService(sc);
            if (i > 0) {
                timer->addTimerSubscriber(this, i, sc->getTimerParameter(), true);
            }
        } catch (const std::runtime_error& ex) {
            log_error("Could not setup SopCast: {}", ex.what());
        }
    }
#endif // SOPCAST

#ifdef ATRAILERS
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        try {
            auto self = shared_from_this();
            auto at = std::make_shared<ATrailersService>(config, database, self);

            int i = config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
            at->setRefreshInterval(i);

            i = config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
            at->setItemPurgeInterval(i);
            if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            auto at_param = std::make_shared<Timer::Parameter>(Timer::Parameter::IDOnlineContent, OS_ATrailers);
            at->setTimerParameter(at_param);
            online_services->registerService(at);
            if (i > 0) {
                timer->addTimerSubscriber(this, i, at->getTimerParameter(), true);
            }
        } catch (const std::runtime_error& ex) {
            log_error("Could not setup Apple Trailers: {}", ex.what());
        }
    }
#endif // ATRAILERS

#endif // ONLINE_SERVICES

    int ret = pthread_create(&taskThread,
        nullptr, //&attr, // attr
        ContentManager::staticThreadProc, this);
    if (ret != 0) {
        throw_std_runtime_error("Could not start task thread");
    }

    autoscan_timed->notifyAll(this);

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        /// \todo change this (we need a new autoscan architecture)
        for (size_t i = 0; i < autoscan_inotify->size(); i++) {
            std::shared_ptr<AutoscanDirectory> adir = autoscan_inotify->get(i);
            if (adir != nullptr)
                inotify->monitor(adir);

            auto param = std::make_shared<Timer::Parameter>(Timer::Parameter::timer_param_t::IDAutoscan, adir->getScanID());
            log_debug("Adding one-shot inotify scan");
            timer->addTimerSubscriber(this, 60, param, true);
        }
    }
#endif

    for (size_t i = 0; i < autoscan_timed->size(); i++) {
        std::shared_ptr<AutoscanDirectory> adir = autoscan_timed->get(i);
        auto param = std::make_shared<Timer::Parameter>(Timer::Parameter::timer_param_t::IDAutoscan, adir->getScanID());
        log_debug("Adding timed scan with interval {}", adir->getInterval());
        timer->addTimerSubscriber(this, adir->getInterval(), param, false);
    }
}

ContentManager::~ContentManager() { log_debug("ContentManager destroyed"); }

void ContentManager::registerExecutor(const std::shared_ptr<Executor>& exec)
{
    AutoLock lock(mutex);
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

    AutoLock lock(mutex);

    process_list.erase(std::remove_if(process_list.begin(), process_list.end(), [&](const auto& e) { return e == exec; }), process_list.end());
}

void ContentManager::timerNotify(std::shared_ptr<Timer::Parameter> parameter)
{
    if (parameter == nullptr)
        return;

    if (parameter->whoami() == Timer::Parameter::IDAutoscan) {
        std::shared_ptr<AutoscanDirectory> adir = autoscan_timed->get(parameter->getID());
        if (adir == nullptr)
            return;

        rescanDirectory(adir, adir->getObjectID());
    }
#ifdef ONLINE_SERVICES
    else if (parameter->whoami() == Timer::Parameter::IDOnlineContent) {
        fetchOnlineContent(static_cast<service_type_t>(parameter->getID()));
    }
#endif // ONLINE_SERVICES
}

void ContentManager::shutdown()
{
    log_debug("start");
    AutoLockU lock(mutex);
    log_debug("updating last_modified data for autoscan in database...");
    autoscan_timed->updateLMinDB();

#ifdef HAVE_JS
    destroyJS();
#endif
    destroyLayout();

#ifdef HAVE_INOTIFY
    if (autoscan_inotify) {
        // update modification time for database
        for (size_t i = 0; i < autoscan_inotify->size(); i++) {
            log_debug("AutoScanDir {}", i);
            std::shared_ptr<AutoscanDirectory> dir = autoscan_inotify->get(i);
            if (dir != nullptr) {
                if (fs::is_directory(dir->getLocation())) {
                    auto t = getLastWriteTime(dir->getLocation());
                    dir->setCurrentLMT(dir->getLocation(), t);
                }
                dir->updateLMT();
            }
        }
        autoscan_inotify->updateLMinDB();

        autoscan_inotify = nullptr;
        inotify = nullptr;
    }
#endif

    shutdownFlag = true;

    for (const auto& exec : process_list) {
        if (exec != nullptr)
            exec->kill();
    }

    log_debug("signalling...");
    signal();
    lock.unlock();
    log_debug("waiting for thread...");

    if (taskThread)
        pthread_join(taskThread, nullptr);
    taskThread = 0;

    if (mime) {
        mime = nullptr;
    }

    log_debug("end");
    log_debug("ContentManager destroyed");
}

std::shared_ptr<GenericTask> ContentManager::getCurrentTask()
{
    AutoLock lock(mutex);

    auto task = currentTask;
    return task;
}

std::deque<std::shared_ptr<GenericTask>> ContentManager::getTasklist()
{
    AutoLock lock(mutex);

    std::deque<std::shared_ptr<GenericTask>> taskList;
#ifdef ONLINE_SERVICES
    taskList = task_processor->getTasklist();
#endif
    auto t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if (t == nullptr && taskList.empty())
        return taskList;

    if (t != nullptr)
        taskList.push_back(t);

    std::copy_if(taskQueue1.begin(), taskQueue1.end(), std::back_inserter(taskList), [](const auto& task) { return task->isValid(); });

    for (const auto& t : taskQueue2) {
        if (t->isValid())
            taskList.clear();
    }

    return taskList;
}

void ContentManager::addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allow_fifo)
{
    obj->validate();
    fs::path path = obj->getLocation();

    std::error_code ec;
    if (!isRegularFile(path, ec))
        throw_std_runtime_error("Not a file: " + path.string());

    auto pcdir = database->findObjectByPath(path);
    if (pcdir == nullptr) {
        pcdir = createObjectFromFile(path, true, allow_fifo);
        if (pcdir == nullptr) {
            throw_std_runtime_error("Could not add " + path.string());
        }
        if (pcdir->isItem()) {
            this->addObject(pcdir);
            obj->setRefID(pcdir->getID());
        }
    }

    addObject(obj);
}

std::shared_ptr<CdsObject> ContentManager::createSingleItem(const fs::path& path, fs::path& rootPath, bool followSymlinks, bool checkDatabase, bool processExisting, const std::shared_ptr<CMAddFileTask>& task)
{
    auto obj = checkDatabase ? database->findObjectByPath(path) : nullptr;
    bool isNew = false;

    if (obj == nullptr) {
        obj = createObjectFromFile(path, followSymlinks);
        if (obj == nullptr) { // object ignored
            log_debug("Link to file or directory ignored: {}", path.c_str());
            return nullptr;
        }
        if (obj->isItem()) {
            addObject(obj);
            isNew = true;
        }
    } else if (obj->isItem() && processExisting) {
        MetadataHandler::setMetadata(config, mime, std::static_pointer_cast<CdsItem>(obj));
    }
    if (obj->isItem() && layout != nullptr && (processExisting || isNew)) {
        try {
            if (rootPath.empty() && (task != nullptr))
                rootPath = task->getRootPath();

            layout->processCdsObject(obj, rootPath);

            std::string mimetype = std::static_pointer_cast<CdsItem>(obj)->getMimeType();
            std::string content_type = getValueOrDefault(mimetype_contenttype_map, mimetype);

#ifdef HAVE_JS
            if ((playlist_parser_script != nullptr) && (content_type == CONTENT_TYPE_PLAYLIST))
                playlist_parser_script->processPlaylistObject(obj, task);
#else
            if (content_type == CONTENT_TYPE_PLAYLIST)
                log_warning("Playlist {} will not be parsed: Gerbera was compiled without JS support!", obj->getLocation().c_str());
#endif // JS
        } catch (const std::runtime_error& e) {
            log_error("{}", e.what());
        }
    }
    return obj;
}

int ContentManager::_addFile(const fs::path& path, fs::path rootPath, AutoScanSetting& asSetting, const std::shared_ptr<CMAddFileTask>& task)
{
    if (!asSetting.hidden) {
        if (path.is_relative())
            return INVALID_OBJECT_ID;
    }

    // never add the server configuration file
    if (config->getConfigFilename() == path)
        return INVALID_OBJECT_ID;

    if (layout_enabled)
        initLayout();

#ifdef HAVE_JS
    initJS();
#endif

    // checkDatabase, don't process existing
    // TODO: stat file to compare with LMT
    auto obj = createSingleItem(path, rootPath, asSetting.followSymlinks, true, false, task);
    if (obj == nullptr) // object ignored
        return INVALID_OBJECT_ID;

    if (asSetting.recursive && obj->isContainer()) {
        addRecursive(asSetting.adir, path, asSetting.followSymlinks, asSetting.hidden, task);
    }

    if (asSetting.rescanResource && obj->hasResource(CH_RESOURCE)) {
        auto parentPath = path.parent_path().string();
        updateAttachedResources(asSetting.adir, obj->getLocation().c_str(), parentPath, true);
    }

    return obj->getID();
}

bool ContentManager::updateAttachedResources(std::shared_ptr<AutoscanDirectory> adir, const char* location, const std::string& parentPath, bool all)
{
    bool parentRemoved = false;
    int parentID = database->findObjectIDByPath(parentPath, false);
    if (parentID != INVALID_OBJECT_ID) {
        // as there is no proper way to force refresh of unchanged files, delete whole dir and rescan it
        _removeObject(adir, parentID, false, all);
        // in order to rescan whole directory we have to set lmt to a very small value
        AutoScanSetting asSetting;
        asSetting.adir = adir;
        adir->setCurrentLMT(parentPath, (time_t)1);
        asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
        asSetting.hidden = config->getBoolOption(CFG_IMPORT_HIDDEN_FILES);
        asSetting.recursive = true;
        asSetting.rescanResource = false;
        asSetting.mergeOptions(config, parentPath);
        // addFile(const fs::path& path, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
        addFile(parentPath, asSetting, true, true, false);
        log_debug("forced rescan of {} for resource {}", parentPath.c_str(), location);
        parentRemoved = true;
    }
    return parentRemoved;
}

void ContentManager::_removeObject(std::shared_ptr<AutoscanDirectory> adir, int objectID, bool rescanResource, bool all)
{
    if (objectID == CDS_ID_ROOT)
        throw_std_runtime_error("cannot remove root container");
    if (objectID == CDS_ID_FS_ROOT)
        throw_std_runtime_error("cannot remove PC-Directory container");
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw_std_runtime_error("tried to remove illegal object id");

    bool parentRemoved = false;
    if (rescanResource) {
        auto obj = database->loadObject(objectID);
        if (obj != nullptr && obj->hasResource(CH_RESOURCE)) {
            auto parentPath = obj->getLocation().parent_path().string();
            parentRemoved = updateAttachedResources(adir, obj->getLocation().c_str(), parentPath, all);
        }
    }

    if (!parentRemoved) {
        auto changedContainers = database->removeObject(objectID, all);
        if (changedContainers != nullptr) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
    // reload accounting
    // loadAccounting();
}

int ContentManager::ensurePathExistence(fs::path path)
{
    int updateID;
    int containerID = database->ensurePathExistence(std::move(path), &updateID);
    if (updateID != INVALID_OBJECT_ID) {
        update_manager->containerChanged(updateID);
        session_manager->containerChangedUI(updateID);
    }
    return containerID;
}

void ContentManager::_rescanDirectory(std::shared_ptr<AutoscanDirectory>& adir, int containerID, const std::shared_ptr<GenericTask>& task)
{
    log_debug("start");

    if (adir == nullptr)
        throw_std_runtime_error("ID valid but nullptr returned? this should never happen");

    fs::path rootpath = adir->getLocation();

    fs::path location;
    std::shared_ptr<CdsObject> obj;

    if (containerID != INVALID_OBJECT_ID) {
        try {
            obj = database->loadObject(containerID);
            if (!obj->isContainer()) {
                throw_std_runtime_error("Not a container");
            }
            if (containerID == CDS_ID_FS_ROOT)
                location = FS_ROOT_DIRECTORY;
            else
                location = obj->getLocation();
        } catch (const std::runtime_error& e) {
            if (adir->persistent()) {
                containerID = INVALID_OBJECT_ID;
            } else {
                adir->setTaskCount(-1);
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

            adir->setTaskCount(-1);
            removeAutoscanDirectory(adir);
            return;
        }

        containerID = ensurePathExistence(adir->getLocation());
        adir->setObjectID(containerID);
        database->updateAutoscanDirectory(adir);
        location = adir->getLocation();
    }

    log_debug("Rescanning location: {}", location.c_str());

    if (location.empty()) {
        log_error("Container with ID {} has no location information", containerID);
        return;
        //        continue;
        // throw_std_runtime_error("Container has no location information");
    }

    DIR* dir = opendir(location.c_str());
    if (!dir) {
        log_warning("Could not open {}: {}", location.c_str(), strerror(errno));
        if (adir->persistent()) {
            removeObject(adir, containerID, false);
            adir->setObjectID(INVALID_OBJECT_ID);
            database->updateAutoscanDirectory(adir);
            return;
        }
        adir->setTaskCount(-1);
        removeObject(adir, containerID, false);
        removeAutoscanDirectory(adir);
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
    if (task != nullptr) {
        thisTaskID = task->getID();
    } else
        thisTaskID = 0;

    time_t last_modified_current_max = adir->getPreviousLMT(location);
    time_t last_modified_new_max = last_modified_current_max;
    adir->setCurrentLMT(location, 0);

    struct dirent* dent;
    while ((dent = readdir(dir)) != nullptr) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            }
            if (name[1] == '.' && name[2] == 0) {
                continue;
            }
            if (!asSetting.hidden) {
                continue;
            }
        }
        fs::path newPath = location / name;

        if ((shutdownFlag) || ((task != nullptr) && !task->isValid()))
            break;

        struct stat statbuf;
        int ret = stat(newPath.c_str(), &statbuf);
        if (ret != 0) {
            log_error("Failed to stat {}, {}", newPath.c_str(), strerror(errno));
            continue;
        }

        // it is possible that someone hits remove while the container is being scanned
        // in this case we will invalidate the autoscan entry
        if (adir->getScanID() == INVALID_SCAN_ID) {
            log_info("lost autoscan for {}", newPath.c_str());
            adir->setCurrentLMT(location, last_modified_new_max > 0 ? last_modified_new_max : (time_t)1);
            closedir(dir);
            return;
        }

        if (isLink(newPath, asSetting.followSymlinks)) {
            int objectID = database->findObjectIDByPath(newPath);
            if (objectID > 0) {
                if (list != nullptr)
                    list->erase(objectID);
                removeObject(adir, objectID, false);
            }
            log_debug("link {} skipped", newPath.c_str());
            continue;
        }

        asSetting.recursive = adir->getRecursive();
        asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
        asSetting.hidden = adir->getHidden();
        asSetting.mergeOptions(config, location);

        if (S_ISREG(statbuf.st_mode)) {
            int objectID = database->findObjectIDByPath(newPath);
            if (objectID > 0) {
                if (list != nullptr)
                    list->erase(objectID);

                // check modification time and update file if chagned
                if (last_modified_current_max < statbuf.st_mtime) {
                    // re-add object - we have to do this in order to trigger
                    // layout
                    removeObject(adir, objectID, false, false);
                    asSetting.recursive = false;
                    asSetting.rescanResource = false;
                    addFileInternal(newPath, rootpath, asSetting, false);
                    // update time variable
                    if (last_modified_new_max < statbuf.st_mtime)
                        last_modified_new_max = statbuf.st_mtime;
                }
            } else {
                // add file, not recursive, not async, not forced
                asSetting.recursive = false;
                asSetting.rescanResource = false;
                addFileInternal(newPath, rootpath, asSetting, false);
                if (last_modified_new_max < statbuf.st_mtime)
                    last_modified_new_max = statbuf.st_mtime;
            }
        } else if (S_ISDIR(statbuf.st_mode) && asSetting.recursive) {
            int objectID = database->findObjectIDByPath(newPath);
            if (last_modified_new_max < statbuf.st_mtime)
                last_modified_new_max = statbuf.st_mtime;
            if (objectID > 0) {
                log_debug("rescanSubDirectory {}", newPath.c_str());
                if (list != nullptr)
                    list->erase(objectID);
                // add a task to rescan the directory that was found
                rescanDirectory(adir, objectID, newPath, task->isCancellable());
            } else {
                log_debug("addSubDirectory {}", newPath.c_str());

                // we have to make sure that we will never add a path to the task list
                // if it is going to be removed by a pending remove task.
                // this lock will make sure that remove is not in the process of invalidating
                // the AutocsanDirectories in the autoscan_timed list at the time when we
                // are checking for validity.
                AutoLock lock(mutex);

                // it is possible that someone hits remove while the container is being scanned
                // in this case we will invalidate the autoscan entry
                if (adir->getScanID() == INVALID_SCAN_ID) {
                    log_info("lost autoscan for {}", newPath.c_str());
                    adir->setCurrentLMT(location, last_modified_new_max > 0 ? last_modified_new_max : (time_t)1);
                    closedir(dir);
                    return;
                }
                // add directory, recursive, async, hidden flag, low priority
                asSetting.recursive = true;
                asSetting.rescanResource = false;
                asSetting.mergeOptions(config, newPath);
                addFileInternal(newPath, rootpath, asSetting, true, true, thisTaskID, task->isCancellable());
            }
        }
    } // while

    closedir(dir);

    adir->setCurrentLMT(location, last_modified_new_max > 0 ? last_modified_new_max : (time_t)1);

    if ((shutdownFlag) || ((task != nullptr) && !task->isValid())) {
        return;
    }
    if (list != nullptr && !list->empty()) {
        auto changedContainers = database->removeObjects(list);
        if (changedContainers != nullptr) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(std::shared_ptr<AutoscanDirectory>& adir, const fs::path& path, bool followSymlinks, bool hidden, const std::shared_ptr<CMAddFileTask>& task)
{
    if (!hidden) {
        log_debug("Checking path {}", path.c_str());
        if (path.is_relative())
            return;
    }

    auto f2i = StringConverter::f2i(config);

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw_std_runtime_error("could not list directory " + path.string() + " : " + strerror(errno));
    }

    int parentID = database->findObjectIDByPath(path);

    // abort loop if either:
    // no valid directory returned, server is about to shutdown, the task is there and was invalidated
    if (task != nullptr) {
        log_debug("IS TASK VALID? [{}], task path: [{}]", task->isValid(), path.c_str());
    }
#ifdef HAVE_INOTIFY
    if (adir == nullptr) {
        for (size_t i = 0; i < autoscan_inotify->size(); i++) {
            log_debug("AutoDir {}", i);
            std::shared_ptr<AutoscanDirectory> dir = autoscan_inotify->get(i);
            if (dir != nullptr && startswith(dir->getLocation(), path) && fs::is_directory(dir->getLocation())) {
                adir = dir;
            }
        }
    }
#endif
    time_t last_modified_current_max = 0;
    time_t last_modified_new_max = last_modified_current_max;
    if (adir != nullptr) {
        last_modified_current_max = adir->getPreviousLMT(path);
        last_modified_new_max = last_modified_current_max;
        adir->setCurrentLMT(path, 0);
    }

    struct dirent* dent;
    while ((dent = readdir(dir)) != nullptr) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            }
            if (name[1] == '.' && name[2] == 0) {
                continue;
            }
            if (!hidden)
                continue;
        }
        fs::path newPath = path / name;

        if ((shutdownFlag) || ((task != nullptr) && !task->isValid()))
            break;

        if (config->getConfigFilename() == newPath)
            continue;

        // For the Web UI
        if (task != nullptr) {
            task->setDescription("Importing: " + newPath.string());
        }
        struct stat statbuf;
        int ret = stat(newPath.c_str(), &statbuf);
        if (ret != 0) {
            log_error("Failed to stat {}, {}", newPath.c_str(), strerror(errno));
            continue;
        }

        try {
            fs::path rootPath("");
            // check database if parent, process existing
            auto obj = createSingleItem(newPath, rootPath, followSymlinks, (parentID > 0), true, task);

            if (obj != nullptr) {
                if (last_modified_current_max < statbuf.st_mtime) {
                    last_modified_new_max = statbuf.st_mtime;
                }
                if (obj->isItem()) {
                    parentID = obj->getParentID();
                }
                if (obj->isContainer()) {
                    addRecursive(adir, newPath, followSymlinks, hidden, task);
                }
            }
        } catch (const std::runtime_error& ex) {
            log_warning("skipping {} (ex:{})", newPath.c_str(), ex.what());
        }
    }
    closedir(dir);

    if (adir != nullptr) {
        adir->setCurrentLMT(path, last_modified_new_max > 0 ? last_modified_new_max : (time_t)1);
    }
}

template <typename T>
void ContentManager::updateCdsObject(std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnp_class = getValueOrDefault(parameters, "class");
    std::string autoscan = getValueOrDefault(parameters, "autoscan");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");

    log_error("updateCdsObject: CdsObject {} not updated", title);
}

template <>
void ContentManager::updateCdsObject(std::shared_ptr<CdsContainer>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnp_class = getValueOrDefault(parameters, "class");

    log_debug("updateCdsObject: CdsContainer {} updated", title);

    auto clone = CdsObject::createObject(item->getObjectType());
    item->copyTo(clone);

    if (!title.empty())
        clone->setTitle(title);
    if (!upnp_class.empty())
        clone->setClass(upnp_class);

    auto cloned_item = std::static_pointer_cast<CdsContainer>(clone);

    if (!item->equals(cloned_item, true)) {
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
void ContentManager::updateCdsObject(std::shared_ptr<CdsItem>& item, const std::map<std::string, std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnp_class = getValueOrDefault(parameters, "class");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");

    log_debug("updateCdsObject: CdsItem {} updated", title);

    auto clone = CdsObject::createObject(item->getObjectType());
    item->copyTo(clone);

    if (!title.empty())
        clone->setTitle(title);
    if (!upnp_class.empty())
        clone->setClass(upnp_class);
    if (!location.empty())
        clone->setLocation(location);

    auto cloned_item = std::static_pointer_cast<CdsItem>(clone);

    if (!mimetype.empty() && !protocol.empty()) {
        cloned_item->setMimeType(mimetype);
        auto resource = cloned_item->getResource(0);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimetype, protocol));
    } else if (mimetype.empty() && !protocol.empty()) {
        auto resource = cloned_item->getResource(0);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(cloned_item->getMimeType(), protocol));
    } else if (!mimetype.empty()) {
        cloned_item->setMimeType(mimetype);
        auto resource = cloned_item->getResource(0);
        std::vector<std::string> parts = splitString(resource->getAttribute(R_PROTOCOLINFO), ':');
        protocol = parts[0];
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimetype, protocol));
    }

    if (!description.empty()) {
        cloned_item->setMetadata(M_DESCRIPTION, description);
    } else {
        cloned_item->removeMetadata(M_DESCRIPTION);
    }

    log_debug("updateCdsObject: checking equality of item {}", item->getTitle().c_str());
    if (!item->equals(cloned_item, true)) {
        cloned_item->validate();
        int containerChanged = INVALID_OBJECT_ID;
        database->updateObject(clone, &containerChanged);
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);
        log_debug("updateObject: calling containerChanged on item {}", item->getTitle().c_str());
        update_manager->containerChanged(item->getParentID());
    }
}

void ContentManager::updateObject(int objectID, const std::map<std::string, std::string>& parameters)
{
    auto obj = database->loadObject(objectID);
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item != nullptr) {
        updateCdsObject(item, parameters);
    } else {
        auto cont = std::dynamic_pointer_cast<CdsContainer>(obj);
        if (cont != nullptr) {
            updateCdsObject(cont, parameters);
        } else {
            updateCdsObject(obj, parameters);
        }
    }
}

void ContentManager::addObject(const std::shared_ptr<CdsObject>& obj)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is {}", obj->getParentID());

    database->addObject(obj, &containerChanged);
    log_debug("After adding: parent ID is {}", obj->getParentID());

    update_manager->containerChanged(containerChanged);
    session_manager->containerChangedUI(containerChanged);

    int parent_id = obj->getParentID();
    if ((parent_id != -1) && (database->getChildCount(parent_id) == 1)) {
        auto parent = database->loadObject(parent_id);
        log_debug("Will update ID {}", parent->getParentID());
        update_manager->containerChanged(parent->getParentID());
    }

    update_manager->containerChanged(obj->getParentID());
    if (obj->isContainer())
        session_manager->containerChangedUI(obj->getParentID());
}

void ContentManager::addContainer(int parentID, std::string title, const std::string& upnpClass)
{
    addContainerChain(database->buildContainerPath(parentID, escape(std::move(title), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR)), upnpClass);
}

int ContentManager::addContainerChain(const std::string& chain, const std::string& lastClass, int lastRefID, const std::map<std::string, std::string>& lastMetadata)
{
    int updateID = INVALID_OBJECT_ID;
    int containerID;

    if (chain.empty())
        throw_std_runtime_error("addContainerChain() called with empty chain parameter");

    std::string newChain = chain;
    for (const auto& [key, val] : config->getDictionaryOption(CFG_IMPORT_LAYOUT_MAPPING)) {
        newChain = std::regex_replace(newChain, std::regex(key), val);
    }

    log_debug("received chain: {} -> {} ({}) [{}]", chain.c_str(), newChain.c_str(), lastClass.c_str(), dictEncodeSimple(lastMetadata).c_str());
    database->addContainerChain(newChain, lastClass, lastRefID, &containerID, &updateID, lastMetadata);

    // if (updateID != INVALID_OBJECT_ID)
    // an invalid updateID is checked by containerChanged()
    update_manager->containerChanged(updateID);
    session_manager->containerChangedUI(updateID);

    return containerID;
}

void ContentManager::updateObject(const std::shared_ptr<CdsObject>& obj, bool send_updates)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    database->updateObject(obj, &containerChanged);

    if (send_updates) {
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);

        update_manager->containerChanged(obj->getParentID());
        if (obj->isContainer())
            session_manager->containerChangedUI(obj->getParentID());
    }
}

bool ContentManager::isLink(const fs::path& path, bool allowLinks)
{
    if (!allowLinks) {
        struct stat statbuf;
        int lret = lstat(path.c_str(), &statbuf);

        if (lret != 0) {
            log_warning("File or directory does not exist: {} ({})", path.string(), strerror(errno));
            return true;
        }

        if (S_ISLNK(statbuf.st_mode)) {
            log_debug("link {} skipped", path.c_str());
            return true;
        }
    }
    return false;
}

std::shared_ptr<CdsObject> ContentManager::createObjectFromFile(const fs::path& path, bool followSymlinks, bool magic, bool allow_fifo)
{
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File or directory does not exist: {} ({})", path.string(), strerror(errno));
        return nullptr;
    }

    if (isLink(path, followSymlinks))
        return nullptr;

    std::shared_ptr<CdsObject> obj;
    if (S_ISREG(statbuf.st_mode) || (allow_fifo && S_ISFIFO(statbuf.st_mode))) { // item
        /* retrieve information about item and decide if it should be included */
        std::string mimetype = mime->extensionToMimeType(path);
        if (mimetype.empty()) {
            if (ignore_unknown_extensions)
                return nullptr; // item should be ignored
#ifdef HAVE_MAGIC
            mimetype = mime->fileToMimeType(path);
#endif
        }

        std::string upnp_class = mime->mimeTypeToUpnpClass(mimetype);
        if (upnp_class.empty()) {
            std::string content_type = getValueOrDefault(mimetype_contenttype_map, mimetype);
            if (content_type == CONTENT_TYPE_OGG) {
                upnp_class = isTheora(path)
                    ? UPNP_CLASS_VIDEO_ITEM
                    : UPNP_CLASS_MUSIC_TRACK;
            }
        }

        auto item = std::make_shared<CdsItem>();
        obj = item;
        item->setLocation(path);
        item->setMTime(statbuf.st_mtime);
        item->setSizeOnDisk(statbuf.st_size);

        if (!mimetype.empty()) {
            item->setMimeType(mimetype);
        }
        if (!upnp_class.empty()) {
            item->setClass(upnp_class);
        }

        auto f2i = StringConverter::f2i(config);
        obj->setTitle(f2i->convert(path.filename()));

        MetadataHandler::setMetadata(config, mime, item);
    } else if (S_ISDIR(statbuf.st_mode)) {
        auto cont = std::make_shared<CdsContainer>();
        obj = cont;
        /* adding containers is done by Database now
         * this exists only to inform the caller that
         * this is a container
         */
        /*
        cont->setLocation(path);
        auto f2i = StringConverter::f2i();
        obj->setTitle(f2i->convert(filename));
        */
    } else {
        // only regular files and directories are supported
        throw_std_runtime_error("ContentManager: skipping file " + path.string());
    }
    //    auto f2i = StringConverter::f2i();
    //    obj->setTitle(f2i->convert(filename));
    return obj;
}

void ContentManager::initLayout()
{
    if (layout == nullptr) {
        AutoLock lock(mutex);
        if (layout == nullptr) {
            std::string layout_type = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
            auto self = shared_from_this();
            try {
                if (layout_type == "js") {
#ifdef HAVE_JS
                    layout = std::make_shared<JSLayout>(config, database, self, scripting_runtime);
#else
                    log_error("Cannot init layout: Gerbera compiled without JS support, but JS was requested.");
#endif
                } else if (layout_type == "builtin") {
                    layout = std::make_shared<FallbackLayout>(config, database, self);
                }
            } catch (const std::runtime_error& e) {
                layout = nullptr;
                log_error("ContentManager virtual container layout: {}", e.what());
                if (layout_type != "disabled")
                    throw e;
            }
        }
    }
}

#ifdef HAVE_JS
void ContentManager::initJS()
{
    if (playlist_parser_script == nullptr) {
        auto self = shared_from_this();
        playlist_parser_script = std::make_unique<PlaylistParserScript>(config, database, self, scripting_runtime);
    }
}

void ContentManager::destroyJS() { playlist_parser_script = nullptr; }

#endif // HAVE_JS

void ContentManager::destroyLayout()
{
    layout = nullptr;
}

void ContentManager::reloadLayout()
{
    destroyLayout();
    initLayout();
}

void ContentManager::threadProc()
{
    std::shared_ptr<GenericTask> task;
    AutoLockU lock(mutex);
    working = true;
    while (!shutdownFlag) {
        currentTask = nullptr;

        task = nullptr;
        if (!taskQueue1.empty()) {
            task = taskQueue1.front();
            taskQueue1.pop_front();
        } else if (!taskQueue2.empty()) {
            task = taskQueue2.front();
            taskQueue2.pop_front();
        }

        if (task == nullptr) {
            working = false;
            /* if nothing to do, sleep until awakened */
            cond.wait(lock);
            working = true;
            continue;
        }

        currentTask = task;
        lock.unlock();

        // log_debug("content manager Async START {}", task->getDescription().c_str());
        try {
            if (task->isValid())
                task->run();
        } catch (const ServerShutdownException& se) {
            shutdownFlag = true;
        } catch (const std::runtime_error& e) {
            log_error("Exception caught: {}", e.what());
        }
        // log_debug("content manager ASYNC STOP  {}", task->getDescription().c_str());

        if (!shutdownFlag) {
            lock.lock();
        }
    }

    database->threadCleanup();
}

void* ContentManager::staticThreadProc(void* arg)
{
    auto inst = static_cast<ContentManager*>(arg);
    inst->threadProc();
    pthread_exit(nullptr);
}

void ContentManager::addTask(const std::shared_ptr<GenericTask>& task, bool lowPriority)
{
    AutoLock lock(mutex);

    task->setID(taskID++);

    if (!lowPriority)
        taskQueue1.push_back(task);
    else
        taskQueue2.push_back(task);
    signal();
}

int ContentManager::addFile(const fs::path& path, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
{
    fs::path rootpath;
    if (fs::is_directory(path))
        rootpath = path;
    return addFileInternal(path, rootpath, asSetting, async, lowPriority, 0, cancellable);
}

int ContentManager::addFile(const fs::path& path, const fs::path& rootpath, AutoScanSetting& asSetting, bool async, bool lowPriority, bool cancellable)
{
    return addFileInternal(path, rootpath, asSetting, async, lowPriority, 0, cancellable);
}

int ContentManager::addFileInternal(
    const fs::path& path, const fs::path& rootpath, AutoScanSetting& asSetting, bool async, bool lowPriority, unsigned int parentTaskID, bool cancellable)
{
    if (async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMAddFileTask>(self, path, rootpath, asSetting, cancellable);
        task->setDescription("Importing: " + path.string());
        task->setParentID(parentTaskID);
        addTask(task, lowPriority);
        return INVALID_OBJECT_ID;
    }
    return _addFile(path, rootpath, asSetting);
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(service_type_t serviceType, bool lowPriority, bool cancellable, bool unscheduled_refresh)
{
    auto service = online_services->getService(serviceType);
    if (service == nullptr) {
        log_debug("No surch service! {}", serviceType);
        throw_std_runtime_error("Service not found");
    }

    if (layout_enabled)
        initLayout();

    unsigned int parentTaskID = 0;

    auto self = shared_from_this();
    auto task = std::make_shared<CMFetchOnlineContentTask>(self, task_processor, timer, service, layout, cancellable, unscheduled_refresh);
    task->setDescription("Updating content from " + service->getServiceName());
    task->setParentID(parentTaskID);
    service->incTaskCount();
    addTask(task, lowPriority);
}

void ContentManager::cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service)
{
    log_debug("Finished fetch cycle for service: {}", service->getServiceName().c_str());

    if (service->getItemPurgeInterval() > 0) {
        auto ids = database->getServiceObjectIDs(service->getDatabasePrefix());

        struct timespec current, last;
        getTimespecNow(&current);
        last.tv_nsec = 0;
        std::string temp;

        for (int object_id : *ids) {
            auto obj = database->loadObject(object_id);
            if (obj == nullptr)
                continue;

            temp = obj->getAuxData(ONLINE_SERVICE_LAST_UPDATE);
            if (temp.empty())
                continue;

            last.tv_sec = std::stol(temp);

            if ((service->getItemPurgeInterval() > 0) && ((current.tv_sec - last.tv_sec) > service->getItemPurgeInterval())) {
                log_debug("Purging old online service object {}", obj->getTitle().c_str());
                removeObject(nullptr, object_id, false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path)
{
    if (t->getType() == AddFile) {
        auto add_task = std::static_pointer_cast<CMAddFileTask>(t);
        log_debug("comparing, task path: {}, remove path: {}", add_task->getPath().c_str(), path.c_str());
        if (startswith(add_task->getPath(), path)) {
            log_debug("Invalidating task with path {}", add_task->getPath().c_str());
            add_task->invalidate();
        }
    }
}

void ContentManager::invalidateTask(unsigned int taskID, task_owner_t taskOwner)
{
    if (taskOwner == ContentManagerTask) {
        AutoLock lock(mutex);
        auto t = getCurrentTask();
        if (t != nullptr) {
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }

        for (const auto& t : taskQueue1) {
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }

        for (const auto& t : taskQueue2) {
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }
    }
#ifdef ONLINE_SERVICES
    else if (taskOwner == TaskProcessorTask)
        task_processor->invalidateTask(taskID);
#endif
}

void ContentManager::removeObject(std::shared_ptr<AutoscanDirectory> adir, int objectID, bool rescanResource, bool async, bool all)
{
    if (async) {
        /*
        // building container path for the description
        auto objectPath = database->getObjectPath(objectID);
        std::ostringstream desc;
        desc << "Removing ";
        // skip root container, start from 1
        for (size_t i = 1; i < objectPath->size(); i++)
            desc << '/' << objectPath->get(i)->getTitle();
        */
        auto self = shared_from_this();
        auto task = std::make_shared<CMRemoveObjectTask>(self, adir, objectID, rescanResource, all);
        fs::path path;
        std::shared_ptr<CdsObject> obj;

        try {
            obj = database->loadObject(objectID);
            path = obj->getLocation();
        } catch (const std::runtime_error& e) {
            log_debug("trying to remove an object ID which is no longer in the database! {}", objectID);
            return;
        }

        if (obj->isContainer()) {
            // make sure to remove possible child autoscan directories from the scanlist
            std::shared_ptr<AutoscanList> rm_list = autoscan_timed->removeIfSubdir(path);
            for (size_t i = 0; i < rm_list->size(); i++) {
                timer->removeTimerSubscriber(this, rm_list->get(i)->getTimerParameter(), true);
            }
#ifdef HAVE_INOTIFY
            if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
                rm_list = autoscan_inotify->removeIfSubdir(path);
                for (size_t i = 0; i < rm_list->size(); i++) {
                    std::shared_ptr<AutoscanDirectory> dir = rm_list->get(i);
                    inotify->unmonitor(dir);
                }
            }
#endif

            AutoLock lock(mutex);

            // we have to make sure that a currently running autoscan task will not
            // launch add tasks for directories that anyway are going to be deleted
            for (const auto& t : taskQueue1) {
                invalidateAddTask(t, path);
            }

            for (const auto& t : taskQueue2) {
                invalidateAddTask(t, path);
            }

            auto t = getCurrentTask();
            if (t != nullptr) {
                invalidateAddTask(t, path);
            }
        }

        addTask(task);
    } else {
        _removeObject(adir, objectID, rescanResource, all);
    }
}

void ContentManager::rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, std::string descPath, bool cancellable)
{
    // building container path for the description
    auto self = shared_from_this();
    auto task = std::make_shared<CMRescanDirectoryTask>(self, adir, objectId, cancellable);

    adir->incTaskCount();

    if (descPath.empty())
        descPath = adir->getLocation();

    task->setDescription("Scan: " + descPath);
    addTask(task, true); // adding with low priority
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, ScanMode scanMode) const
{
    if (scanMode == ScanMode::Timed) {
        return autoscan_timed->get(scanID);
    }

#if HAVE_INOTIFY
    if (scanMode == ScanMode::INotify) {
        return autoscan_inotify->get(scanID);
    }
#endif
    return nullptr;
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int objectID)
{
    return database->getAutoscanDirectory(objectID);
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(const fs::path& location) const
{
    // \todo change this when more scanmodes become available
    std::shared_ptr<AutoscanDirectory> adir = autoscan_timed->get(location);
#if HAVE_INOTIFY
    if (adir == nullptr)
        adir = autoscan_inotify->get(location);
#endif
    return adir;
}

std::vector<std::shared_ptr<AutoscanDirectory>> ContentManager::getAutoscanDirectories() const
{
    auto all = autoscan_timed->getArrayCopy();

#if HAVE_INOTIFY
    auto ino = autoscan_inotify->getArrayCopy();
    std::copy(ino.begin(), ino.end(), std::back_inserter(all));
#endif
    return all;
}

void ContentManager::removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (adir == nullptr)
        throw_std_runtime_error("can not remove autoscan directory - was not an autoscan");

    if (adir->getScanMode() == ScanMode::Timed) {
        autoscan_timed->remove(adir->getScanID());
        database->removeAutoscanDirectory(adir);
        session_manager->containerChangedUI(adir->getObjectID());

        // if 3rd parameter is true: won't fail if scanID doesn't exist
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (adir->getScanMode() == ScanMode::INotify) {
            autoscan_inotify->remove(adir->getScanID());
            database->removeAutoscanDirectory(adir);
            session_manager->containerChangedUI(adir->getObjectID());
            inotify->unmonitor(adir);
        }
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
    std::shared_ptr<AutoscanDirectory> original;

    // We will have to change this for other scan modes
    original = autoscan_timed->getByObjectID(dir->getObjectID());
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (original == nullptr)
            original = autoscan_inotify->getByObjectID(dir->getObjectID());
    }
#endif

    if (original != nullptr)
        dir->setDatabaseID(original->getDatabaseID());

    database->checkOverlappingAutoscans(dir);

    // adding a new autoscan directory
    if (original == nullptr) {
        if (dir->getObjectID() == CDS_ID_FS_ROOT)
            dir->setLocation(FS_ROOT_DIRECTORY);
        else {
            log_debug("objectID: {}", dir->getObjectID());
            auto obj = database->loadObject(dir->getObjectID());
            if (obj == nullptr || !obj->isContainer() || obj->isVirtual())
                throw_std_runtime_error("tried to remove an illegal object (id) from the list of the autoscan directories");

            log_debug("location: {}", obj->getLocation().c_str());

            if (obj->getLocation().empty())
                throw_std_runtime_error("tried to add an illegal object as autoscan - no location information available");

            dir->setLocation(obj->getLocation());
        }
        dir->resetLMT();
        database->addAutoscanDirectory(dir);
        if (dir->getScanMode() == ScanMode::Timed) {
            autoscan_timed->add(dir);
            timerNotify(dir->getTimerParameter());
        }
#ifdef HAVE_INOTIFY
        if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
            if (dir->getScanMode() == ScanMode::INotify) {
                autoscan_inotify->add(dir);
                inotify->monitor(dir);
            }
        }
#endif
        session_manager->containerChangedUI(dir->getObjectID());
        return;
    }

    if (original->getScanMode() == ScanMode::Timed)
        timer->removeTimerSubscriber(this, original->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (original->getScanMode() == ScanMode::INotify) {
            inotify->unmonitor(original);
        }
    }
#endif

    auto copy = std::make_shared<AutoscanDirectory>();
    original->copyTo(copy);

    copy->setHidden(dir->getHidden());
    copy->setRecursive(dir->getRecursive());
    copy->setInterval(dir->getInterval());

    if (copy->getScanMode() == ScanMode::Timed) {
        autoscan_timed->remove(copy->getScanID());
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (copy->getScanMode() == ScanMode::INotify) {
            autoscan_inotify->remove(copy->getScanID());
        }
    }
#endif

    copy->setScanMode(dir->getScanMode());

    if (dir->getScanMode() == ScanMode::Timed) {
        autoscan_timed->add(copy);
        timerNotify(copy->getTimerParameter());
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (dir->getScanMode() == ScanMode::INotify) {
            autoscan_inotify->add(copy);
            inotify->monitor(copy);
        }
    }
#endif

    database->updateAutoscanDirectory(copy);
    if (original->getScanMode() != copy->getScanMode())
        session_manager->containerChangedUI(copy->getObjectID());
}

void ContentManager::triggerPlayHook(const std::shared_ptr<CdsObject>& obj)
{
    log_debug("start");

    if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && !obj->getFlag(OBJECT_FLAG_PLAYED)) {
        std::vector<std::string> mark_list = config->getArrayOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);

        bool mark = std::any_of(mark_list.begin(), mark_list.end(), [&](const auto& i) { return startswith(std::static_pointer_cast<CdsItem>(obj)->getMimeType(), i); });
        if (mark) {
            obj->setFlag(OBJECT_FLAG_PLAYED);

            bool supress = config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
            log_debug("Marking object {} as played", obj->getTitle().c_str());
            updateObject(obj, !supress);
        }
    }

#ifdef HAVE_LASTFMLIB
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED) && startswith(std::static_pointer_cast<CdsItem>(obj)->getMimeType(), ("audio"))) {
        last_fm->startedPlaying(std::static_pointer_cast<CdsItem>(obj));
    }
#endif
    log_debug("end");
}

CMAddFileTask::CMAddFileTask(std::shared_ptr<ContentManager> content,
    fs::path path, fs::path rootpath, AutoScanSetting& asSetting, bool cancellable)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , path(std::move(path))
    , rootpath(std::move(rootpath))
    , asSetting(asSetting)
{
    this->cancellable = cancellable;
    this->taskType = AddFile;
}

fs::path CMAddFileTask::getPath() { return path; }

fs::path CMAddFileTask::getRootPath() { return rootpath; }

void CMAddFileTask::run()
{
    log_debug("running add file task with path {} recursive: {}", path.c_str(), asSetting.recursive);
    auto self = shared_from_this();
    content->_addFile(path, rootpath, asSetting, self);
}

CMRemoveObjectTask::CMRemoveObjectTask(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> adir,
    int objectID, bool rescanResource, bool all)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , adir(std::move(adir))
    , objectID(objectID)
    , all(all)
    , rescanResource(rescanResource)
{
    this->taskType = RemoveObject;
    cancellable = false;
}

void CMRemoveObjectTask::run()
{
    content->_removeObject(adir, objectID, rescanResource, all);
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
    if (adir == nullptr)
        return;

    auto self = shared_from_this();
    content->_rescanDirectory(adir, containerID, self);
    adir->decTaskCount();

    if (adir->getTaskCount() == 0) {
        adir->updateLMT();
    }
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> task_processor, std::shared_ptr<Timer> timer,
    std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout, bool cancellable, bool unscheduled_refresh)
    : GenericTask(ContentManagerTask)
    , content(std::move(content))
    , task_processor(std::move(task_processor))
    , timer(std::move(timer))
    , service(std::move(service))
    , layout(std::move(layout))
{
    this->cancellable = cancellable;
    this->unscheduled_refresh = unscheduled_refresh;
    this->taskType = FetchOnlineContent;
}

void CMFetchOnlineContentTask::run()
{
    if (this->service == nullptr) {
        log_debug("Received invalid service!");
        return;
    }
    try {
        std::shared_ptr<GenericTask> t(
            new TPFetchOnlineContentTask(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh));
        task_processor->addTask(t);
    } catch (const std::runtime_error& ex) {
        log_error("{}", ex.what());
    }
}
#endif // ONLINE_SERVICES

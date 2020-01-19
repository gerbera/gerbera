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

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config_manager.h"
#include "storage/storage.h"
#include "content_manager.h"
#include "util/filesystem.h"
#include "layout/fallback_layout.h"
#include "metadata/metadata_handler.h"
#include "web/session_manager.h"
#include "util/string_converter.h"
#include "util/timer.h"
#include "util/tools.h"
#include "update_manager.h"
#include "util/process.h"

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

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}

struct magic_set* ms = nullptr;
#endif

using namespace std;

static std::string get_filename(std::string path)
{
    if (path.at(path.length() - 1) == DIR_SEPARATOR) // cut off trailing slash
        path = path.substr(0, path.length() - 1);
    size_t pos = path.rfind(DIR_SEPARATOR);
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

ContentManager::ContentManager(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<UpdateManager> update_manager, std::shared_ptr<web::SessionManager> session_manager,
    std::shared_ptr<Timer> timer, std::shared_ptr<TaskProcessor> task_processor,
    std::shared_ptr<Runtime> scripting_runtime, std::shared_ptr<LastFm> last_fm)
    : config(config)
    , storage(storage)
    , update_manager(update_manager)
    , session_manager(session_manager)
    , timer(timer)
    , task_processor(task_processor)
    , scripting_runtime(scripting_runtime)
    , last_fm(last_fm)
{
    ignore_unknown_extensions = false;
    extension_map_case_sensitive = false;

    taskID = 1;
    working = false;
    shutdownFlag = false;
    layout_enabled = false;

    // loading extension - mimetype map
    // we can always be sure to get a valid element because everything was prepared by the config manager
    extension_mimetype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    ignore_unknown_extensions = config->getBoolOption(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    if (ignore_unknown_extensions && (extension_mimetype_map.size() == 0)) {
        log_warning("Ignore unknown extensions set, but no mappings specified");
        log_warning("Please review your configuration!");
        ignore_unknown_extensions = false;
    }

    extension_map_case_sensitive = config->getBoolOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    mimetype_upnpclass_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    mimetype_contenttype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    auto config_timed_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    for (size_t i = 0; i < config_timed_list->size(); i++) {
        auto dir = config_timed_list->get(i);
        if (dir != nullptr) {
            std::string path = dir->getLocation();
            if (check_path(path, true)) {
                dir->setObjectID(ensurePathExistence(path));
            }
        }
    }

    storage->updateAutoscanPersistentList(ScanMode::Timed, config_timed_list);
    autoscan_timed = storage->getAutoscanList(ScanMode::Timed);
}

void ContentManager::init()
{
#ifdef HAVE_INOTIFY
    auto self = shared_from_this();
    inotify = std::make_unique<AutoscanInotify>(storage, self);

    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        auto config_inotify_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);
        for (size_t i = 0; i < config_inotify_list->size(); i++) {
            auto dir = config_inotify_list->get(i);
            if (dir != nullptr) {
                std::string path = dir->getLocation();
                if (check_path(path, true)) {
                    dir->setObjectID(ensurePathExistence(path));
                }
            }
        }

        storage->updateAutoscanPersistentList(ScanMode::INotify, config_inotify_list);
        autoscan_inotify = storage->getAutoscanList(ScanMode::INotify);
    } else {
        // make an empty list so we do not have to do extra checks on shutdown
        autoscan_inotify = std::make_shared<AutoscanList>(storage);
    }

    // Start INotify thread
    inotify->run();
#endif
/* init filemagic */
#ifdef HAVE_MAGIC
    if (!ignore_unknown_extensions) {
        ms = magic_open(MAGIC_MIME);
        if (ms == nullptr) {
            log_error("magic_open failed");
        } else {
            std::string optMagicFile = config->getOption(CFG_IMPORT_MAGIC_FILE);
            const char* magicFile = NULL;
            if (!optMagicFile.empty())
                magicFile = optMagicFile.c_str();
            if (magic_load(ms, magicFile) == -1) {
                log_warning("magic_load: {}", magic_error(ms));
                magic_close(ms);
                ms = nullptr;
            }
        }
    }
#endif // HAVE_MAGIC

    std::string layout_type = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    if ((layout_type == "builtin") || (layout_type == "js"))
        layout_enabled = true;

#ifdef ONLINE_SERVICES
    online_services = std::make_unique<OnlineServiceList>();

#ifdef SOPCAST
    if (config->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_ENABLED)) {
        try {
            auto self = shared_from_this();
            auto sc = std::make_shared<SopCastService>(config, storage, self);

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
            auto at = std::make_shared<ATrailersService>(config, storage, self);

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
        throw std::runtime_error("Could not start task thread");
    }

    autoscan_timed->notifyAll(this);

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        /// \todo change this (we need a new autoscan architecture)
        for (size_t i = 0; i < autoscan_inotify->size(); i++) {
            std::shared_ptr<AutoscanDirectory> dir = autoscan_inotify->get(i);
            if (dir != nullptr)
                inotify->monitor(dir);
        }
    }
#endif

    for (size_t i = 0; i < autoscan_timed->size(); i++) {
        std::shared_ptr<AutoscanDirectory> dir = autoscan_timed->get(i);
        auto param = std::make_shared<Timer::Parameter>(Timer::Parameter::timer_param_t::IDAutoscan, dir->getScanID());
        log_debug("Adding timed scan with interval {}", dir->getInterval());
        timer->addTimerSubscriber(this, dir->getInterval(), param, false);
    }
}

ContentManager::~ContentManager() { log_debug("ContentManager destroyed"); }

void ContentManager::registerExecutor(std::shared_ptr<Executor> exec)
{
    AutoLock lock(mutex);
    process_list.push_back(exec);
}

void ContentManager::unregisterExecutor(std::shared_ptr<Executor> exec)
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

    for (auto it = process_list.begin(); it != process_list.end(); /*++it*/) {
        auto& e = *it;

        if (e == exec)
            it = process_list.erase(it);
        else
            ++it;
    }
}

void ContentManager::timerNotify(std::shared_ptr<Timer::Parameter> parameter)
{
    if (parameter == nullptr)
        return;

    if (parameter->whoami() == Timer::Parameter::IDAutoscan) {
        std::shared_ptr<AutoscanDirectory> dir = autoscan_timed->get(parameter->getID());
        if (dir == nullptr)
            return;

        int objectID = dir->getObjectID();
        rescanDirectory(objectID, dir->getScanID(), dir->getScanMode());
    }
#ifdef ONLINE_SERVICES
    else if (parameter->whoami() == Timer::Parameter::IDOnlineContent) {
        fetchOnlineContent((service_type_t)(parameter->getID()));
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
    for (size_t i = 0; i < autoscan_inotify->size(); i++) {
        log_debug("AutoDir {}", i);
        std::shared_ptr<AutoscanDirectory> dir = autoscan_inotify->get(i);
        if (dir != nullptr) {
            try {
                dir->resetLMT();
                dir->setCurrentLMT(check_path_ex(dir->getLocation(), true));
                dir->updateLMT();
            } catch (const std::runtime_error& ex) {
                continue;
            }
        }
    }

    autoscan_inotify->updateLMinDB();
    autoscan_inotify = nullptr;
    inotify = nullptr;
#endif

    shutdownFlag = true;

    for (size_t i = 0; i < process_list.size(); i++) {
        auto exec = process_list[i];
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

#ifdef HAVE_MAGIC
    if (ms) {
        magic_close(ms);
        ms = nullptr;
    }
#endif
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

    for (size_t i = 0; i < taskQueue1.size(); i++) {
        auto t = taskQueue1[i];
        if (t->isValid())
            taskList.push_back(t);
    }

    for (size_t i = 0; i < taskQueue2.size(); i++) {
        auto t = taskQueue2[i];
        if (t->isValid())
            taskList.clear();
    }

    return taskList;
}

void ContentManager::addVirtualItem(std::shared_ptr<CdsObject> obj, bool allow_fifo)
{
    obj->validate();
    std::string path = obj->getLocation();
    check_path_ex(path, false, false);
    auto pcdir = storage->findObjectByPath(path);
    if (pcdir == nullptr) {
        pcdir = createObjectFromFile(path, true, allow_fifo);
        if (pcdir == nullptr) {
            throw std::runtime_error("Could not add " + path);
        }
        if (IS_CDS_ITEM(pcdir->getObjectType())) {
            this->addObject(pcdir);
            obj->setRefID(pcdir->getID());
        }
    }

    addObject(obj);
}

int ContentManager::_addFile(std::string path, std::string rootPath, bool recursive, bool hidden, std::shared_ptr<CMAddFileTask> task)
{
    if (hidden == false) {
        std::string filename = get_filename(path);
        if (string_ok(filename) && filename.at(0) == '.')
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

    auto obj = storage->findObjectByPath(path);
    if (obj == nullptr) {
        obj = createObjectFromFile(path);
        if (obj == nullptr) // object ignored
            return INVALID_OBJECT_ID;
        if (IS_CDS_ITEM(obj->getObjectType())) {
            addObject(obj);
            if (layout != nullptr) {
                try {
                    if (!string_ok(rootPath) && (task != nullptr))
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
                    throw e;
                }
            }
        }
    }

    if (recursive && IS_CDS_CONTAINER(obj->getObjectType())) {
        addRecursive(path, hidden, task);
    }
    return obj->getID();
}

void ContentManager::_removeObject(int objectID, bool all)
{
    if (objectID == CDS_ID_ROOT)
        throw std::runtime_error("cannot remove root container");
    if (objectID == CDS_ID_FS_ROOT)
        throw std::runtime_error("cannot remove PC-Directory container");
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw std::runtime_error("tried to remove illegal object id");

    auto changedContainers = storage->removeObject(objectID, all);
    if (changedContainers != nullptr) {
        session_manager->containerChangedUI(changedContainers->ui);
        update_manager->containersChanged(changedContainers->upnp);
    }

    // reload accounting
    // loadAccounting();
}

int ContentManager::ensurePathExistence(std::string path)
{
    int updateID;
    int containerID = storage->ensurePathExistence(path, &updateID);
    if (updateID != INVALID_OBJECT_ID) {
        update_manager->containerChanged(updateID);
        session_manager->containerChangedUI(updateID);
    }
    return containerID;
}

void ContentManager::_rescanDirectory(int containerID, int scanID, ScanMode scanMode, ScanLevel scanLevel, std::shared_ptr<GenericTask> task)
{
    log_debug("start");
    int ret;
    struct dirent* dent;
    struct stat statbuf;
    std::string location;
    std::string path;
    std::shared_ptr<CdsObject> obj;

    if (scanID == INVALID_SCAN_ID)
        return;

    std::shared_ptr<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    if (adir == nullptr)
        throw std::runtime_error("ID valid but nullptr returned? this should never happen");

    if (containerID != INVALID_OBJECT_ID) {
        try {
            obj = storage->loadObject(containerID);
            if (!IS_CDS_CONTAINER(obj->getObjectType())) {
                throw std::runtime_error("Not a container");
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
                removeAutoscanDirectory(scanID, scanMode);
                storage->removeAutoscanDirectory(adir->getStorageID());
                return;
            }
        }
    }

    if (containerID == INVALID_OBJECT_ID) {
        if (!check_path(adir->getLocation(), true)) {
            adir->setObjectID(INVALID_OBJECT_ID);
            storage->updateAutoscanDirectory(adir);
            if (adir->persistent()) {
                return;
            } else {
                adir->setTaskCount(-1);
                removeAutoscanDirectory(scanID, scanMode);
                storage->removeAutoscanDirectory(adir->getStorageID());
                return;
            }
        }

        containerID = ensurePathExistence(adir->getLocation());
        adir->setObjectID(containerID);
        storage->updateAutoscanDirectory(adir);
        location = adir->getLocation();
    }

    time_t last_modified_current_max = adir->getPreviousLMT();

    log_debug("Rescanning location: {}", location.c_str());

    if (!string_ok(location)) {
        log_error("Container with ID {} has no location information", containerID);
        return;
        //        continue;
        // throw std::runtime_error("Container has no location information!\n");
    }

    DIR* dir = opendir(location.c_str());
    if (!dir) {
        log_warning("Could not open {}: {}", location.c_str(), strerror(errno));
        if (adir->persistent()) {
            removeObject(containerID, false);
            adir->setObjectID(INVALID_OBJECT_ID);
            storage->updateAutoscanDirectory(adir);
            return;
        } else {
            adir->setTaskCount(-1);
            removeObject(containerID, false);
            removeAutoscanDirectory(scanID, scanMode);
            storage->removeAutoscanDirectory(adir->getStorageID());
            return;
        }
    }

    // request only items if non-recursive scan is wanted
    unique_ptr<unordered_set<int>> list = storage->getObjects(containerID, !adir->getRecursive());

    unsigned int thisTaskID;
    if (task != nullptr) {
        thisTaskID = task->getID();
    } else
        thisTaskID = 0;

    while (((dent = readdir(dir)) != nullptr) && (!shutdownFlag) && (task == nullptr || ((task != nullptr) && task->isValid()))) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            } else if (name[1] == '.' && name[2] == 0) {
                continue;
            } else if (!adir->getHidden()) {
                continue;
            }
        }

        path = location + DIR_SEPARATOR + name;
        ret = stat(path.c_str(), &statbuf);
        if (ret != 0) {
            log_error("Failed to stat {}, {}", path.c_str(), mt_strerror(errno).c_str());
            continue;
        }

        // it is possible that someone hits remove while the container is being scanned
        // in this case we will invalidate the autoscan entry
        if (adir->getScanID() == INVALID_SCAN_ID) {
            closedir(dir);
            return;
        }

        if (S_ISREG(statbuf.st_mode)) {
            int objectID = storage->findObjectIDByPath(std::string(path));
            if (objectID > 0) {
                if (list != nullptr)
                    list->erase(objectID);

                if (scanLevel == ScanLevel::Full) {
                    // check modification time and update file if chagned
                    if (last_modified_current_max < statbuf.st_mtime) {
                        // readd object - we have to do this in order to trigger
                        // layout
                        removeObject(objectID, false);
                        addFileInternal(path, location, false, false, adir->getHidden());
                        // update time variable
                        last_modified_current_max = statbuf.st_mtime;
                    }
                } else if (scanLevel == ScanLevel::Basic)
                    continue;
                else
                    throw std::runtime_error("Unsupported scan level!");

            } else {
                // add file, not recursive, not async
                // make sure not to add the current config.xml
                if (config->getConfigFilename() != path) {
                    addFileInternal(path, location, false, false, adir->getHidden());
                    if (last_modified_current_max < statbuf.st_mtime)
                        last_modified_current_max = statbuf.st_mtime;
                }
            }
        } else if (S_ISDIR(statbuf.st_mode) && (adir->getRecursive())) {
            int objectID = storage->findObjectIDByPath(path + DIR_SEPARATOR);
            if (objectID > 0) {
                if (list != nullptr)
                    list->erase(objectID);
                // add a task to rescan the directory that was found
                rescanDirectory(objectID, scanID, scanMode, path + DIR_SEPARATOR, task->isCancellable());
            } else {
                // we have to make sure that we will never add a path to the task list
                // if it is going to be removed by a pending remove task.
                // this lock will make sure that remove is not in the process of invalidating
                // the AutocsanDirectories in the autoscan_timed list at the time when we
                // are checking for validity.
                AutoLock lock(mutex);

                // it is possible that someone hits remove while the container is being scanned
                // in this case we will invalidate the autoscan entry
                if (adir->getScanID() == INVALID_SCAN_ID) {
                    closedir(dir);
                    return;
                }

                // add directory, recursive, async, hidden flag, low priority
                addFileInternal(path, location, true, true, adir->getHidden(), true, thisTaskID, task->isCancellable());
            }
        }
    } // while

    closedir(dir);

    if ((shutdownFlag) || ((task != nullptr) && !task->isValid()))
        return;

    if (list != nullptr && list->size() > 0) {
        auto changedContainers = storage->removeObjects(list);
        if (changedContainers != nullptr) {
            session_manager->containerChangedUI(changedContainers->ui);
            update_manager->containersChanged(changedContainers->upnp);
        }
    }

    adir->setCurrentLMT(last_modified_current_max);
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(std::string path, bool hidden, std::shared_ptr<CMAddFileTask> task)
{
    if (hidden == false) {
        log_debug("Checking path {}", path.c_str());
        if (path.at(0) == '.')
            return;
    }

    auto f2i = StringConverter::f2i(config);

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("could not list directory " + path + " : " + strerror(errno));
    }

    int parentID = storage->findObjectIDByPath(path + DIR_SEPARATOR);
    struct dirent* dent;
    // abort loop if either:
    // no valid directory returned, server is about to shutdown, the task is there and was invalidated
    if (task != nullptr) {
        log_debug("IS TASK VALID? [{}], taskoath: [{}]", task->isValid(), path.c_str());
    }
    while (((dent = readdir(dir)) != nullptr) && (!shutdownFlag) && (task == nullptr || ((task != nullptr) && task->isValid()))) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            } else if (name[1] == '.' && name[2] == 0) {
                continue;
            } else if (hidden == false)
                continue;
        }
        std::string newPath = path + DIR_SEPARATOR + name;

        if (config->getConfigFilename() == newPath)
            continue;

        // For the Web UI
        if (task != nullptr) {
            task->setDescription("Importing: " + newPath);
        }

        try {
            std::shared_ptr<CdsObject> obj = nullptr;
            if (parentID > 0)
                obj = storage->findObjectByPath(std::string(newPath));
            if (obj == nullptr) // create object
            {
                obj = createObjectFromFile(newPath);

                if (obj == nullptr) // object ignored
                {
                    log_warning("file ignored: {}", newPath.c_str());
                } else {
                    // obj->setParentID(parentID);
                    if (IS_CDS_ITEM(obj->getObjectType())) {
                        addObject(obj);
                        parentID = obj->getParentID();
                    }
                }
            }
            if (obj != nullptr) {
                //#ifdef HAVE_JS
                if (IS_CDS_ITEM(obj->getObjectType())) {
                    if (layout != nullptr) {
                        try {
                            std::string rootpath = "";
                            if (task != nullptr)
                                rootpath = task->getRootPath();
                            layout->processCdsObject(obj, rootpath);
#ifdef HAVE_JS
                            auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
                            std::string mimetype = std::static_pointer_cast<CdsItem>(obj)->getMimeType();
                            std::string content_type = getValueOrDefault(mappings, mimetype);

                            if ((playlist_parser_script != nullptr) && (content_type == CONTENT_TYPE_PLAYLIST))
                                playlist_parser_script->processPlaylistObject(obj, task);

#endif // JS
                        } catch (const std::runtime_error& e) {
                            throw e;
                        }
                    }

                    /// \todo Why was this statement here??? - It seems to be unnecessary
                    // obj = createObjectFromFile(newPath);
                }
                //#endif
                if (IS_CDS_CONTAINER(obj->getObjectType())) {
                    addRecursive(newPath, hidden, task);
                }
            }
        } catch (const std::runtime_error& e) {
            log_warning("skipping {} : {}", newPath.c_str(), e.what());
        }
    }
    closedir(dir);
}

void ContentManager::updateObject(int objectID, const std::map<std::string,std::string>& parameters)
{
    std::string title = getValueOrDefault(parameters, "title");
    std::string upnp_class = getValueOrDefault(parameters, "class");
    std::string autoscan = getValueOrDefault(parameters, "autoscan");
    std::string mimetype = getValueOrDefault(parameters, "mime-type");
    std::string description = getValueOrDefault(parameters, "description");
    std::string location = getValueOrDefault(parameters, "location");
    std::string protocol = getValueOrDefault(parameters, "protocol");

    auto obj = storage->loadObject(objectID);
    int objectType = obj->getObjectType();

    /// \todo if we have an active item, does it mean we first go through IS_ITEM and then thorugh IS_ACTIVE item? ask Gena
    if (IS_CDS_ITEM(objectType)) {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        auto clone = CdsObject::createObject(storage, objectType);
        item->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);
        if (string_ok(location))
            clone->setLocation(location);

        auto cloned_item = std::static_pointer_cast<CdsItem>(clone);

        if (string_ok(mimetype) && (string_ok(protocol))) {
            cloned_item->setMimeType(mimetype);
            auto resource = cloned_item->getResource(0);
            resource->addAttribute("protocolInfo", renderProtocolInfo(mimetype, protocol));
        } else if (!string_ok(mimetype) && (string_ok(protocol))) {
            auto resource = cloned_item->getResource(0);
            resource->addAttribute("protocolInfo", renderProtocolInfo(cloned_item->getMimeType(), protocol));
        } else if (string_ok(mimetype) && (!string_ok(protocol))) {
            cloned_item->setMimeType(mimetype);
            auto resource = cloned_item->getResource(0);
            std::vector<std::string> parts = split_string(resource->getAttribute("protocolInfo"), ':');
            protocol = parts[0];
            resource->addAttribute("protocolInfo", renderProtocolInfo(mimetype, protocol));
        }

        if (string_ok(description)) {
            cloned_item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), description);
        } else {
            cloned_item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
        }

        log_debug("updateObject: checking equality of item {}", item->getTitle().c_str());
        if (!item->equals(cloned_item, true)) {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            update_manager->containerChanged(containerChanged);
            session_manager->containerChangedUI(containerChanged);
            log_debug("updateObject: calling containerChanged on item {}", item->getTitle().c_str());
            update_manager->containerChanged(item->getParentID());
        }
    }
    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        std::string action = getValueOrDefault(parameters, "action");
        std::string state = getValueOrDefault(parameters, "state");

        auto item = std::static_pointer_cast<CdsActiveItem>(obj);
        auto clone = CdsObject::createObject(storage, objectType);
        item->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);

        auto cloned_item = std::static_pointer_cast<CdsActiveItem>(clone);

        // state and description can be an empty strings - if you want to clear it
        if (string_ok(description)) {
            cloned_item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), description);
        } else {
            cloned_item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
        }

        if (!state.empty())
            cloned_item->setState(state);

        if (string_ok(mimetype))
            cloned_item->setMimeType(mimetype);
        if (string_ok(action))
            cloned_item->setAction(action);

        if (!item->equals(cloned_item, true)) {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            update_manager->containerChanged(containerChanged);
            session_manager->containerChangedUI(containerChanged);
            update_manager->containerChanged(item->getParentID());
        }
    } else if (IS_CDS_CONTAINER(objectType)) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto clone = CdsObject::createObject(storage, objectType);
        cont->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);

        auto cloned_item = std::static_pointer_cast<CdsContainer>(clone);

        if (!cont->equals(cloned_item, true)) {
            clone->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            update_manager->containerChanged(containerChanged);
            session_manager->containerChangedUI(containerChanged);
            update_manager->containerChanged(cont->getParentID());
            session_manager->containerChangedUI(cont->getParentID());
        }
    }
}

void ContentManager::addObject(std::shared_ptr<CdsObject> obj)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is {}", obj->getParentID());
    if (!IS_CDS_ITEM_EXTERNAL_URL(obj->getObjectType())) {
        obj->setLocation(reduce_string(obj->getLocation(), DIR_SEPARATOR));
    }

    storage->addObject(obj, &containerChanged);
    log_debug("After adding: parent ID is {}", obj->getParentID());

    update_manager->containerChanged(containerChanged);
    session_manager->containerChangedUI(containerChanged);

    int parent_id = obj->getParentID();
    if ((parent_id != -1) && (storage->getChildCount(parent_id) == 1)) {
        auto parent = storage->loadObject(parent_id);
        log_debug("Will update ID {}", parent->getParentID());
        update_manager->containerChanged(parent->getParentID());
    }

    update_manager->containerChanged(obj->getParentID());
    if (IS_CDS_CONTAINER(obj->getObjectType()))
        session_manager->containerChangedUI(obj->getParentID());
}

void ContentManager::addContainer(int parentID, std::string title, std::string upnpClass)
{
    addContainerChain(storage->buildContainerPath(parentID, escape(title, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR)), upnpClass);
}

int ContentManager::addContainerChain(std::string chain, std::string lastClass, int lastRefID, const std::map<std::string,std::string>& lastMetadata)
{
    int updateID = INVALID_OBJECT_ID;
    int containerID;

    if (!string_ok(chain))
        throw std::runtime_error("addContainerChain() called with empty chain parameter");

    log_debug("received chain: {} ({}) [{}]", chain.c_str(), lastClass.c_str(), dict_encode_simple(lastMetadata).c_str());
    storage->addContainerChain(chain, lastClass, lastRefID, &containerID, &updateID, lastMetadata);

    // if (updateID != INVALID_OBJECT_ID)
    // an invalid updateID is checked by containerChanged()
    update_manager->containerChanged(updateID);
    session_manager->containerChangedUI(updateID);

    return containerID;
}

void ContentManager::updateObject(std::shared_ptr<CdsObject> obj, bool send_updates)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    storage->updateObject(obj, &containerChanged);

    if (send_updates) {
        update_manager->containerChanged(containerChanged);
        session_manager->containerChangedUI(containerChanged);

        update_manager->containerChanged(obj->getParentID());
        if (IS_CDS_CONTAINER(obj->getObjectType()))
            session_manager->containerChangedUI(obj->getParentID());
    }
}

std::shared_ptr<CdsObject> ContentManager::convertObject(std::shared_ptr<CdsObject> oldObj, int newType)
{
    int oldType = oldObj->getObjectType();
    if (oldType == newType)
        return oldObj;
    if (!IS_CDS_ITEM(oldType) || !IS_CDS_ITEM(newType)) {
        throw std::runtime_error("Cannot convert object type " + std::to_string(oldType) + " to " + std::to_string(newType));
    }

    auto newObj = CdsObject::createObject(storage, newType);
    oldObj->copyTo(newObj);

    return newObj;
}

// returns nullptr if file ignored due to configuration
std::shared_ptr<CdsObject> ContentManager::createObjectFromFile(std::string path, bool magic, bool allow_fifo)
{
    std::string filename = get_filename(path);

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw std::runtime_error("Failed to stat " + path + " , " + mt_strerror(errno));
    }

    std::shared_ptr<CdsObject> obj;
    if (S_ISREG(statbuf.st_mode) || (allow_fifo && S_ISFIFO(statbuf.st_mode))) { // item
        /* retrieve information about item and decide if it should be included */
        std::string mimetype;
        std::string upnp_class;
        std::string extension;

        // get file extension
        size_t dotIndex = filename.rfind('.');
        if (dotIndex != std::string::npos)
            extension = filename.substr(dotIndex + 1);

        if (magic) {
            mimetype = extension2mimetype(extension);

            if (!string_ok(mimetype)) {
                if (ignore_unknown_extensions)
                    return nullptr; // item should be ignored
#ifdef HAVE_MAGIC
                mimetype = getMIMETypeFromFile(path);
#endif
            }
        }

        if (!mimetype.empty()) {
            upnp_class = mimetype2upnpclass(mimetype);
        }

        if (!string_ok(upnp_class)) {
            std::string content_type = getValueOrDefault(mimetype_contenttype_map, mimetype);
            if (content_type == CONTENT_TYPE_OGG) {
                if (isTheora(path))
                    upnp_class = UPNP_DEFAULT_CLASS_VIDEO_ITEM;
                else
                    upnp_class = UPNP_DEFAULT_CLASS_MUSIC_TRACK;
            }
        }

        auto item = std::make_shared<CdsItem>(storage);
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
        obj->setTitle(f2i->convert(filename));

        if (magic) {
            MetadataHandler::setMetadata(config, item);
        }
    } else if (S_ISDIR(statbuf.st_mode)) {
        auto cont = std::make_shared<CdsContainer>(storage);
        obj = cont;
        /* adding containers is done by Storage now
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
        throw std::runtime_error("ContentManager: skipping file " + path);
    }
    //    auto f2i = StringConverter::f2i();
    //    obj->setTitle(f2i->convert(filename));
    return obj;
}

std::string ContentManager::extension2mimetype(std::string extension)
{
    if (!extension_map_case_sensitive)
        extension = tolower_string(extension);

    return getValueOrDefault(extension_mimetype_map, extension);
}

std::string ContentManager::mimetype2upnpclass(std::string mimeType)
{
    std::string upnpClass = getValueOrDefault(mimetype_upnpclass_map, mimeType);
    if (!upnpClass.empty())
        return upnpClass;
    // try to match foo
    std::vector<std::string> parts = split_string(mimeType, '/');
    if (parts.size() != 2)
        return "";
    return getValueOrDefault(mimetype_upnpclass_map, parts[0] + "/*");
}

void ContentManager::initLayout()
{

    if (layout == nullptr) {
        AutoLock lock(mutex);
        if (layout == nullptr)
            try {
                std::string layout_type = config->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
                auto self = shared_from_this();
                if (layout_type == "js") {
#ifdef HAVE_JS
                    layout = std::make_shared<JSLayout>(config, storage, self, scripting_runtime);
#else
                    log_error("Cannot init layout: Gerbera compiled without JS support, but JS was requested.");
#endif
                } else if (layout_type == "builtin") {
                    layout = std::make_shared<FallbackLayout>(config, storage, self);
                }
            } catch (const std::runtime_error& e) {
                layout = nullptr;
                log_error("ContentManager virtual container layout: {}", e.what());
            }
    }
}

#ifdef HAVE_JS
void ContentManager::initJS()
{
    if (playlist_parser_script == nullptr) {
        auto self = shared_from_this();
        playlist_parser_script = std::make_unique<PlaylistParserScript>(config, storage, self, scripting_runtime);
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
        } else {
            currentTask = task;
        }
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

    storage->threadCleanup();
}

void* ContentManager::staticThreadProc(void* arg)
{
    auto* inst = (ContentManager*)arg;
    inst->threadProc();
    pthread_exit(nullptr);
    return nullptr;
}

void ContentManager::addTask(std::shared_ptr<GenericTask> task, bool lowPriority)
{
    AutoLock lock(mutex);

    task->setID(taskID++);

    if (!lowPriority)
        taskQueue1.push_back(task);
    else
        taskQueue2.push_back(task);
    signal();
}

int ContentManager::addFile(std::string path, bool recursive, bool async, bool hidden, bool lowPriority, bool cancellable)
{
    std::string rootpath;
    if (check_path(path, true))
        rootpath = path;
    return addFileInternal(path, rootpath, recursive, async, hidden, lowPriority, 0, cancellable);
}

int ContentManager::addFile(std::string path, std::string rootpath, bool recursive, bool async, bool hidden, bool lowPriority, bool cancellable)
{
    return addFileInternal(path, rootpath, recursive, async, hidden, lowPriority, 0, cancellable);
}

int ContentManager::addFileInternal(
    std::string path, std::string rootpath, bool recursive, bool async, bool hidden, bool lowPriority, unsigned int parentTaskID, bool cancellable)
{
    if (async) {
        auto self = shared_from_this();
        auto task = std::make_shared<CMAddFileTask>(self, path, rootpath, recursive, hidden, cancellable);
        task->setDescription("Importing: " + path);
        task->setParentID(parentTaskID);
        addTask(task, lowPriority);
        return INVALID_OBJECT_ID;
    } else {
        return _addFile(path, rootpath, recursive, hidden);
    }
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(service_type_t serviceType, bool lowPriority, bool cancellable, bool unscheduled_refresh)
{
    auto service = online_services->getService(serviceType);
    if (service == nullptr) {
        log_debug("No surch service! {}", serviceType);
        throw std::runtime_error("Service not found!");
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

void ContentManager::cleanupOnlineServiceObjects(std::shared_ptr<OnlineService> service)
{
    log_debug("Finished fetch cycle for service: {}", service->getServiceName().c_str());

    if (service->getItemPurgeInterval() > 0) {
        auto ids = storage->getServiceObjectIDs(service->getStoragePrefix());

        struct timespec current, last;
        getTimespecNow(&current);
        last.tv_nsec = 0;
        std::string temp;

        for (int object_id : *ids) {
            auto obj = storage->loadObject(object_id);
            if (obj == nullptr)
                continue;

            temp = obj->getAuxData(ONLINE_SERVICE_LAST_UPDATE);
            if (!string_ok(temp))
                continue;

            last.tv_sec = std::stol(temp);

            if ((service->getItemPurgeInterval() > 0) && ((current.tv_sec - last.tv_sec) > service->getItemPurgeInterval())) {
                log_debug("Purging old online service object {}", obj->getTitle().c_str());
                removeObject(object_id, false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(std::shared_ptr<GenericTask> t, std::string path)
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

        for (size_t i = 0; i < taskQueue1.size(); i++) {
            auto t = taskQueue1[i];
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }

        for (size_t i = 0; i < taskQueue2.size(); i++) {
            auto t = taskQueue2[i];
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

void ContentManager::removeObject(int objectID, bool async, bool all)
{
    if (async) {
        /*
        // building container path for the description
        auto objectPath = storage->getObjectPath(objectID);
        std::ostringstream desc;
        desc << "Removing ";
        // skip root container, start from 1
        for (int i = 1; i < objectPath->size(); i++)
            desc << '/' << objectPath->get(i)->getTitle();
        */
        auto self = shared_from_this();
        auto task = std::make_shared<CMRemoveObjectTask>(self, objectID, all);
        std::string path;
        std::shared_ptr<CdsObject> obj;

        try {
            obj = storage->loadObject(objectID);
            path = obj->getLocation();

            std::string vpath = obj->getVirtualPath();
            if (string_ok(vpath))
                task->setDescription("Removing: " + obj->getVirtualPath());
        } catch (const std::runtime_error& e) {
            log_debug("trying to remove an object ID which is no longer in the database! {}", objectID);
            return;
        }

        if (IS_CDS_CONTAINER(obj->getObjectType())) {
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
            for (size_t i = 0; i < taskQueue1.size(); i++) {
                auto t = taskQueue1[i];
                invalidateAddTask(t, path);
            }

            for (size_t i = 0; i < taskQueue2.size(); i++) {
                auto t = taskQueue2[i];
                invalidateAddTask(t, path);
            }

            auto t = getCurrentTask();
            if (t != nullptr) {
                invalidateAddTask(t, path);
            }
        }

        addTask(task);
    } else {
        _removeObject(objectID, all);
    }
}

void ContentManager::rescanDirectory(int objectID, int scanID, ScanMode scanMode, std::string descPath, bool cancellable)
{
    // building container path for the description
    auto self = shared_from_this();
    auto task = std::make_shared<CMRescanDirectoryTask>(self, objectID, scanID, scanMode, cancellable);
    std::shared_ptr<AutoscanDirectory> dir = getAutoscanDirectory(scanID, scanMode);
    if (dir == nullptr)
        return;

    dir->incTaskCount();
    std::string level;
    if (dir->getScanLevel() == ScanLevel::Basic)
        level = "basic";
    else
        level = "full";

    if (!string_ok(descPath))
        descPath = dir->getLocation();

    task->setDescription("Performing " + level + " scan: " + descPath);
    addTask(task, true); // adding with low priority
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, ScanMode scanMode)
{
    if (scanMode == ScanMode::Timed) {
        return autoscan_timed->get(scanID);
    }

#if HAVE_INOTIFY
    else if (scanMode == ScanMode::INotify) {
        return autoscan_inotify->get(scanID);
    }
#endif
    return nullptr;
}

std::vector<std::shared_ptr<AutoscanDirectory>> ContentManager::getAutoscanDirectories(ScanMode scanMode)
{
    if (scanMode == ScanMode::Timed) {
        return autoscan_timed->getArrayCopy();
    }

#if HAVE_INOTIFY
    else if (scanMode == ScanMode::INotify) {
        return autoscan_inotify->getArrayCopy();
    }
#endif
    return std::vector<std::shared_ptr<AutoscanDirectory>>();
}

std::vector<std::shared_ptr<AutoscanDirectory>> ContentManager::getAutoscanDirectories()
{
    auto all = autoscan_timed->getArrayCopy();

#if HAVE_INOTIFY
    auto ino = autoscan_inotify->getArrayCopy();
    for (size_t i = 0; i < ino.size(); i++)
        all.push_back(ino[i]);
#endif
    return all;
}

std::shared_ptr<AutoscanDirectory> ContentManager::getAutoscanDirectory(std::string location)
{
    // \todo change this when more scanmodes become available
    std::shared_ptr<AutoscanDirectory> dir = autoscan_timed->get(location);
#if HAVE_INOTIFY
    if (dir == nullptr)
        dir = autoscan_inotify->get(location);
#endif
    return dir;
}

void ContentManager::removeAutoscanDirectory(int scanID, ScanMode scanMode)
{
    if (scanMode == ScanMode::Timed) {
        std::shared_ptr<AutoscanDirectory> adir = autoscan_timed->get(scanID);
        if (adir == nullptr)
            throw std::runtime_error("can not remove autoscan directory - was not an autoscan");

        autoscan_timed->remove(scanID);
        storage->removeAutoscanDirectory(adir->getStorageID());
        session_manager->containerChangedUI(adir->getObjectID());

        // if 3rd parameter is true: won't fail if scanID doesn't exist
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (scanMode == ScanMode::INotify) {
            std::shared_ptr<AutoscanDirectory> adir = autoscan_inotify->get(scanID);
            if (adir == nullptr)
                throw std::runtime_error("can not remove autoscan directory - was not an autoscan");
            autoscan_inotify->remove(scanID);
            storage->removeAutoscanDirectory(adir->getStorageID());
            session_manager->containerChangedUI(adir->getObjectID());
            inotify->unmonitor(adir);
        }
    }
#endif
}

void ContentManager::removeAutoscanDirectory(int objectID)
{
    std::shared_ptr<AutoscanDirectory> adir = storage->getAutoscanDirectory(objectID);
    if (adir == nullptr)
        throw std::runtime_error("can not remove autoscan directory - was not an autoscan");

    if (adir->getScanMode() == ScanMode::Timed) {
        autoscan_timed->remove(adir->getLocation());
        storage->removeAutoscanDirectoryByObjectID(objectID);
        session_manager->containerChangedUI(objectID);
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (adir->getScanMode() == ScanMode::INotify) {
            autoscan_inotify->remove(adir->getLocation());
            storage->removeAutoscanDirectoryByObjectID(objectID);
            session_manager->containerChangedUI(objectID);
            inotify->unmonitor(adir);
        }
    }
#endif
}

void ContentManager::removeAutoscanDirectory(std::string location)
{
    /// \todo change this when more scanmodes become avaiable
    std::shared_ptr<AutoscanDirectory> adir = autoscan_timed->get(location);
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (adir == nullptr)
            adir = autoscan_inotify->get(location);
    }
#endif
    if (adir == nullptr)
        throw std::runtime_error("can not remove autoscan directory - was not an autoscan");

    removeAutoscanDirectory(adir->getObjectID());
}

void ContentManager::handlePeristentAutoscanRemove(int scanID, ScanMode scanMode)
{
    std::shared_ptr<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    if (adir->persistent()) {
        adir->setObjectID(INVALID_OBJECT_ID);
        storage->updateAutoscanDirectory(adir);
    } else {
        removeAutoscanDirectory(adir->getScanID(), adir->getScanMode());
        storage->removeAutoscanDirectory(adir->getStorageID());
    }
}

void ContentManager::handlePersistentAutoscanRecreate(int scanID, ScanMode scanMode)
{
    std::shared_ptr<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    int id = ensurePathExistence(adir->getLocation());
    adir->setObjectID(id);
    storage->updateAutoscanDirectory(adir);
}

void ContentManager::setAutoscanDirectory(std::shared_ptr<AutoscanDirectory> dir)
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
        dir->setStorageID(original->getStorageID());

    storage->checkOverlappingAutoscans(dir);

    // adding a new autoscan directory
    if (original == nullptr) {
        if (dir->getObjectID() == CDS_ID_FS_ROOT)
            dir->setLocation(FS_ROOT_DIRECTORY);
        else {
            log_debug("objectID: {}", dir->getObjectID());
            auto obj = storage->loadObject(dir->getObjectID());
            if (obj == nullptr || !IS_CDS_CONTAINER(obj->getObjectType()) || obj->isVirtual())
                throw std::runtime_error("tried to remove an illegal object (id) from the list of the autoscan directories");

            log_debug("location: {}", obj->getLocation().c_str());

            if (!string_ok(obj->getLocation()))
                throw std::runtime_error("tried to add an illegal object as autoscan - no location information available!");

            dir->setLocation(obj->getLocation());
        }
        dir->resetLMT();
        storage->addAutoscanDirectory(dir);
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

    // changing from full scan to basic scan need to reset last modification time
    if ((copy->getScanLevel() == ScanLevel::Full) && (dir->getScanLevel() == ScanLevel::Basic)) {
        copy->setScanLevel(ScanLevel::Basic);
        copy->resetLMT();
    } else if (((copy->getScanLevel() == ScanLevel::Full) && (dir->getScanLevel() == ScanLevel::Full)) && (!copy->getRecursive() && dir->getRecursive())) {
        copy->resetLMT();
    }

    copy->setScanLevel(dir->getScanLevel());
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

    storage->updateAutoscanDirectory(copy);
    if (original->getScanMode() != copy->getScanMode())
        session_manager->containerChangedUI(copy->getObjectID());
}

void ContentManager::triggerPlayHook(std::shared_ptr<CdsObject> obj)
{
    log_debug("start");

    if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && !obj->getFlag(OBJECT_FLAG_PLAYED)) {
        std::vector<std::string>  mark_list = config->getStringArrayOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        for (size_t i = 0; i < mark_list.size(); i++) {
            if (startswith(std::static_pointer_cast<CdsItem>(obj)->getMimeType(), mark_list[i])) {
                obj->setFlag(OBJECT_FLAG_PLAYED);

                bool supress = config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
                log_debug("Marking object {} as played", obj->getTitle().c_str());
                updateObject(obj, !supress);
                break;
            }
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
    std::string path, std::string rootpath, bool recursive, bool hidden, bool cancellable)
    : GenericTask(ContentManagerTask)
    , content(content)
    , path(path)
    , rootpath(rootpath)
    , recursive(recursive)
    , hidden(hidden)
{
    this->cancellable = cancellable;
    this->taskType = AddFile;
}

std::string CMAddFileTask::getPath() { return path; }

std::string CMAddFileTask::getRootPath() { return rootpath; }

void CMAddFileTask::run()
{
    log_debug("running add file task with path {} recursive: {}", path.c_str(), recursive);
    auto self = shared_from_this();
    content->_addFile(path, rootpath, recursive, hidden, self);
}

CMRemoveObjectTask::CMRemoveObjectTask(std::shared_ptr<ContentManager> content,
    int objectID, bool all)
    : GenericTask(ContentManagerTask)
    , content(content)
    , objectID(objectID)
    , all(all)
{
    this->taskType = RemoveObject;
    cancellable = false;
}

void CMRemoveObjectTask::run()
{
    content->_removeObject(objectID, all);
}

CMRescanDirectoryTask::CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
    int objectID, int scanID, ScanMode scanMode, bool cancellable)
    : GenericTask(ContentManagerTask)
    , content(content)
    , objectID(objectID)
    , scanID(scanID)
    , scanMode(scanMode)
{
    this->cancellable = cancellable;
    this->taskType = RescanDirectory;
}

void CMRescanDirectoryTask::run()
{
    std::shared_ptr<AutoscanDirectory> dir = content->getAutoscanDirectory(scanID, scanMode);
    if (dir == nullptr)
        return;

    auto self = shared_from_this();
    content->_rescanDirectory(objectID, dir->getScanID(), dir->getScanMode(), dir->getScanLevel(), self);
    dir->decTaskCount();

    if (dir->getTaskCount() == 0) {
        dir->updateLMT();
    }
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> task_processor, std::shared_ptr<Timer> timer,
    std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout, bool cancellable, bool unscheduled_refresh)
    : GenericTask(ContentManagerTask)
    , content(content)
    , task_processor(task_processor)
    , timer(timer)
    , service(service)
    , layout(layout)
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
            new TPFetchOnlineContentTask(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh)
        );
        task_processor->addTask(t);
    } catch (const std::runtime_error& ex) {
        log_error("{}", ex.what());
    }
}
#endif // ONLINE_SERVICES

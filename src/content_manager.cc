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
#include "util/rexp.h"
#include "web/session_manager.h"
#include "util/string_converter.h"
#include "util/timer.h"
#include "util/tools.h"
#include "update_manager.h"

#ifdef HAVE_JS
#include "layout/js_layout.h"
#endif

#include "util/process.h"

#ifdef SOPCAST
#include "onlineservice/sopcast_service.h"
#endif

#ifdef ATRAILERS
#include "onlineservice/atrailers_service.h"
#endif

#ifdef ONLINE_SERVICES
#include "util/task_processor.h"
#endif

#define DEFAULT_DIR_CACHE_CAPACITY 10
#define CM_INITIAL_QUEUE_SIZE 20

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}

struct magic_set* ms = nullptr;
#endif

using namespace zmm;
using namespace mxml;
using namespace std;

#define MIMETYPE_REGEXP "^([a-z0-9_-]+/[a-z0-9_-]+)"

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

    acct = Ref<CMAccounting>(new CMAccounting());
    taskQueue1 = Ref<ObjectQueue<GenericTask>>(new ObjectQueue<GenericTask>(CM_INITIAL_QUEUE_SIZE));
    taskQueue2 = Ref<ObjectQueue<GenericTask>>(new ObjectQueue<GenericTask>(CM_INITIAL_QUEUE_SIZE));

    Ref<Element> tmpEl;

    // loading extension - mimetype map
    // we can always be sure to get a valid element because everything was prepared by the config manager
    extension_mimetype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    ignore_unknown_extensions = config->getBoolOption(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    if (ignore_unknown_extensions && (extension_mimetype_map.size() == 0)) {
        log_warning("Ignore unknown extensions set, but no mappings specified\n");
        log_warning("Please review your configuration!\n");
        ignore_unknown_extensions = false;
    }

    extension_map_case_sensitive = config->getBoolOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    mimetype_upnpclass_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    mimetype_contenttype_map = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    auto config_timed_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    int i;
    for (i = 0; i < config_timed_list->size(); i++) {
        Ref<AutoscanDirectory> dir = config_timed_list->get(i);
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
    int i;

#ifdef HAVE_INOTIFY
    auto self = shared_from_this();
    inotify = std::make_unique<AutoscanInotify>(storage, self);

    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        auto config_inotify_list = config->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);
        for (i = 0; i < config_inotify_list->size(); i++) {
            Ref<AutoscanDirectory> dir = config_inotify_list->get(i);
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
        autoscan_inotify = Ref<AutoscanList>(new AutoscanList(storage));
    }

    // Start INotify thread
    inotify->run();
#endif
/* init filemagic */
#ifdef HAVE_MAGIC
    if (!ignore_unknown_extensions) {
        ms = magic_open(MAGIC_MIME);
        if (ms == nullptr) {
            log_error("magic_open failed\n");
        } else {
            std::string optMagicFile = config->getOption(CFG_IMPORT_MAGIC_FILE);
            const char* magicFile = NULL;
            if (!optMagicFile.empty())
                magicFile = optMagicFile.c_str();
            if (magic_load(ms, magicFile) == -1) {
                log_warning("magic_load: %s\n", magic_error(ms));
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
    online_services = Ref<OnlineServiceList>(new OnlineServiceList());

#ifdef SOPCAST
    if (config->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_ENABLED)) {
        try {
            auto self = shared_from_this();
            Ref<OnlineService> sc((OnlineService*)new SopCastService(config, storage, self));

            i = config->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);
            sc->setRefreshInterval(i);

            i = config->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);
            sc->setItemPurgeInterval(i);

            if (config->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<Timer::Parameter> sc_param(new Timer::Parameter(Timer::Parameter::IDOnlineContent, OS_SopCast));
            sc->setTimerParameter(sc_param);
            online_services->registerService(sc);
            if (i > 0) {
                timer->addTimerSubscriber(this, i, sc->getTimerParameter(), true);
            }
        } catch (const Exception& ex) {
            log_error("Could not setup SopCast: %s\n", ex.getMessage().c_str());
        }
    }
#endif // SOPCAST

#ifdef ATRAILERS
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        try {
            auto self = shared_from_this();
            Ref<OnlineService> at((OnlineService*)new ATrailersService(config, storage, self));
            i = config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
            at->setRefreshInterval(i);

            i = config->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
            at->setItemPurgeInterval(i);
            if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<Timer::Parameter> at_param(new Timer::Parameter(Timer::Parameter::IDOnlineContent, OS_ATrailers));
            at->setTimerParameter(at_param);
            online_services->registerService(at);
            if (i > 0) {
                timer->addTimerSubscriber(this, i, at->getTimerParameter(), true);
            }
        } catch (const Exception& ex) {
            log_error("Could not setup Apple Trailers: %s\n", ex.getMessage().c_str());
        }
    }
#endif // ATRAILERS

#endif // ONLINE_SERVICES

    reMimetype = Ref<RExp>(new RExp());
    reMimetype->compile(MIMETYPE_REGEXP);

    int ret = pthread_create(&taskThread,
        nullptr, //&attr, // attr
        ContentManager::staticThreadProc, this);
    if (ret != 0) {
        throw _Exception("Could not start task thread");
    }

    autoscan_timed->notifyAll(this);

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        /// \todo change this (we need a new autoscan architecture)
        for (int i = 0; i < autoscan_inotify->size(); i++) {
            Ref<AutoscanDirectory> dir = autoscan_inotify->get(i);
            if (dir != nullptr)
                inotify->monitor(dir);
        }
    }
#endif

    for (int i = 0; i < autoscan_timed->size(); i++) {
        Ref<AutoscanDirectory> dir = autoscan_timed->get(i);
        Ref<Timer::Parameter> param(new Timer::Parameter(Timer::Parameter::timer_param_t::IDAutoscan, dir->getScanID()));
        log_debug("Adding timed scan with interval %d\n", dir->getInterval());
        timer->addTimerSubscriber(this, dir->getInterval(), param, false);
    }

    process_list = Ref<Array<Executor>>(new Array<Executor>());
}

ContentManager::~ContentManager() { log_debug("ContentManager destroyed\n"); }

void ContentManager::registerExecutor(Ref<Executor> exec)
{
    AutoLock lock(mutex);
    process_list->append(exec);
}

void ContentManager::unregisterExecutor(Ref<Executor> exec)
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
    for (int i = 0; i < process_list->size(); i++) {
        if (process_list->get(i) == exec)
            process_list->remove(i);
    }
}

void ContentManager::timerNotify(Ref<Timer::Parameter> parameter)
{
    if (parameter == nullptr)
        return;

    if (parameter->whoami() == Timer::Parameter::IDAutoscan) {
        Ref<AutoscanDirectory> dir = autoscan_timed->get(parameter->getID());
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
    log_debug("start\n");
    AutoLockU lock(mutex);
    log_debug("updating last_modified data for autoscan in database...\n");
    autoscan_timed->updateLMinDB();

#ifdef HAVE_JS
    destroyJS();
#endif
    destroyLayout();

#ifdef HAVE_INOTIFY
    for (int i = 0; i < autoscan_inotify->size(); i++) {
        log_debug("AutoDir %d\n", i);
        Ref<AutoscanDirectory> dir = autoscan_inotify->get(i);
        if (dir != nullptr) {
            try {
                dir->resetLMT();
                dir->setCurrentLMT(check_path_ex(dir->getLocation(), true));
                dir->updateLMT();
            } catch (const Exception& ex) {
                continue;
            }
        }
    }

    autoscan_inotify->updateLMinDB();
    autoscan_inotify = nullptr;
    inotify = nullptr;
#endif

    shutdownFlag = true;

    for (int i = 0; i < process_list->size(); i++) {
        Ref<Executor> exec = process_list->get(i);
        if (exec != nullptr)
            exec->kill();
    }

    log_debug("signalling...\n");
    signal();
    lock.unlock();
    log_debug("waiting for thread...\n");

    if (taskThread)
        pthread_join(taskThread, nullptr);
    taskThread = 0;

#ifdef HAVE_MAGIC
    if (ms) {
        magic_close(ms);
        ms = nullptr;
    }
#endif
    log_debug("end\n");
    log_debug("ContentManager destroyed\n");
}

Ref<CMAccounting> ContentManager::getAccounting() { return acct; }
Ref<GenericTask> ContentManager::getCurrentTask()
{
    Ref<GenericTask> task;
    AutoLock lock(mutex);
    task = currentTask;
    return task;
}

Ref<Array<GenericTask>> ContentManager::getTasklist()
{
    int i;

    AutoLock lock(mutex);

    Ref<Array<GenericTask>> taskList = nullptr;
#ifdef ONLINE_SERVICES
    taskList = task_processor->getTasklist();
#endif
    Ref<GenericTask> t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if ((t == nullptr) && (taskList == nullptr))
        return nullptr;

    if (taskList == nullptr)
        taskList = Ref<Array<GenericTask>>(new Array<GenericTask>());

    if (t != nullptr)
        taskList->append(t);

    int qsize = taskQueue1->size();

    for (i = 0; i < qsize; i++) {
        Ref<GenericTask> t = taskQueue1->get(i);
        if (t->isValid())
            taskList->append(t);
    }

    qsize = taskQueue2->size();
    for (i = 0; i < qsize; i++) {
        Ref<GenericTask> t = taskQueue2->get(i);
        if (t->isValid())
            taskList = Ref<Array<GenericTask>>(new Array<GenericTask>());
    }

    return taskList;
}

void ContentManager::_loadAccounting()
{
    acct->totalFiles = storage->getTotalFiles();
}

void ContentManager::addVirtualItem(Ref<CdsObject> obj, bool allow_fifo)
{
    obj->validate();
    std::string path = obj->getLocation();
    check_path_ex(path, false, false);
    Ref<CdsObject> pcdir = storage->findObjectByPath(path);
    if (pcdir == nullptr) {
        pcdir = createObjectFromFile(path, true, allow_fifo);
        if (pcdir == nullptr) {
            throw _Exception("Could not add " + path);
        }
        if (IS_CDS_ITEM(pcdir->getObjectType())) {
            this->addObject(pcdir);
            obj->setRefID(pcdir->getID());
        }
    }

    addObject(obj);
}

int ContentManager::_addFile(std::string path, std::string rootPath, bool recursive, bool hidden, Ref<GenericTask> task)
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

    Ref<CdsObject> obj = storage->findObjectByPath(path);
    if (obj == nullptr) {
        obj = createObjectFromFile(path);
        if (obj == nullptr) // object ignored
            return INVALID_OBJECT_ID;
        if (IS_CDS_ITEM(obj->getObjectType())) {
            addObject(obj);
            if (layout != nullptr) {
                try {
                    if (!string_ok(rootPath) && (task != nullptr))
                        rootPath = RefCast(task, CMAddFileTask)->getRootPath();

                    layout->processCdsObject(obj, rootPath);

                    std::string mimetype = RefCast(obj, CdsItem)->getMimeType();
                    std::string content_type = getValueOrDefault(mimetype_contenttype_map, mimetype);
#ifdef HAVE_JS
                    if ((playlist_parser_script != nullptr) && (content_type == CONTENT_TYPE_PLAYLIST))
                        playlist_parser_script->processPlaylistObject(obj, task);
#else
                    if (content_type == CONTENT_TYPE_PLAYLIST)
                        log_warning("Playlist %s will not be parsed: Gerbera was compiled without JS support!\n", obj->getLocation().c_str());
#endif // JS
                } catch (const Exception& e) {
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
        throw _Exception("cannot remove root container");
    if (objectID == CDS_ID_FS_ROOT)
        throw _Exception("cannot remove PC-Directory container");
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw _Exception("tried to remove illegal object id");

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

void ContentManager::_rescanDirectory(int containerID, int scanID, ScanMode scanMode, ScanLevel scanLevel, Ref<GenericTask> task)
{
    log_debug("start\n");
    int ret;
    struct dirent* dent;
    struct stat statbuf;
    std::string location;
    std::string path;
    Ref<CdsObject> obj;

    if (scanID == INVALID_SCAN_ID)
        return;

    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    if (adir == nullptr)
        throw _Exception("ID valid but nullptr returned? this should never happen");

    if (containerID != INVALID_OBJECT_ID) {
        try {
            obj = storage->loadObject(containerID);
            if (!IS_CDS_CONTAINER(obj->getObjectType())) {
                throw _Exception("Not a container");
            }
            if (containerID == CDS_ID_FS_ROOT)
                location = FS_ROOT_DIRECTORY;
            else
                location = obj->getLocation();
        } catch (const Exception& e) {
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

    log_debug("Rescanning location: %s\n", location.c_str());

    if (!string_ok(location)) {
        log_error("Container with ID %d has no location information\n", containerID);
        return;
        //        continue;
        // throw _Exception("Container has no location information!\n");
    }

    DIR* dir = opendir(location.c_str());
    if (!dir) {
        log_warning("Could not open %s: %s\n", location.c_str(), strerror(errno));
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
            log_error("Failed to stat %s, %s\n", path.c_str(), mt_strerror(errno).c_str());
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
                    throw _Exception("Unsupported scan level!");

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
void ContentManager::addRecursive(std::string path, bool hidden, Ref<GenericTask> task)
{
    if (hidden == false) {
        log_debug("Checking path %s\n", path.c_str());
        if (path.at(0) == '.')
            return;
    }

    Ref<StringConverter> f2i = StringConverter::f2i(config);

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw _Exception("could not list directory " + path + " : " + strerror(errno));
    }

    int parentID = storage->findObjectIDByPath(path + DIR_SEPARATOR);
    struct dirent* dent;
    // abort loop if either:
    // no valid directory returned, server is about to shutdown, the task is there and was invalidated
    if (task != nullptr) {
        log_debug("IS TASK VALID? [%d], taskoath: [%s]\n", task->isValid(), path.c_str());
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
            Ref<CdsObject> obj = nullptr;
            if (parentID > 0)
                obj = storage->findObjectByPath(std::string(newPath));
            if (obj == nullptr) // create object
            {
                obj = createObjectFromFile(newPath);

                if (obj == nullptr) // object ignored
                {
                    log_warning("file ignored: %s\n", newPath.c_str());
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
                                rootpath = RefCast(task, CMAddFileTask)->getRootPath();
                            layout->processCdsObject(obj, rootpath);
#ifdef HAVE_JS
                            auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
                            std::string mimetype = RefCast(obj, CdsItem)->getMimeType();
                            std::string content_type = getValueOrDefault(mappings, mimetype);

                            if ((playlist_parser_script != nullptr) && (content_type == CONTENT_TYPE_PLAYLIST))
                                playlist_parser_script->processPlaylistObject(obj, task);

#endif // JS
                        } catch (const Exception& e) {
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
        } catch (const Exception& e) {
            log_warning("skipping %s : %s\n", newPath.c_str(), e.getMessage().c_str());
        }
    }
    closedir(dir);
}

void ContentManager::updateObject(int objectID, Ref<Dictionary> parameters)
{
    std::string title = parameters->get("title");
    std::string upnp_class = parameters->get("class");
    std::string autoscan = parameters->get("autoscan");
    std::string mimetype = parameters->get("mime-type");
    std::string description = parameters->get("description");
    std::string location = parameters->get("location");
    std::string protocol = parameters->get("protocol");

    Ref<CdsObject> obj = storage->loadObject(objectID);
    int objectType = obj->getObjectType();

    /// \todo if we have an active item, does it mean we first go through IS_ITEM and then thorugh IS_ACTIVE item? ask Gena
    if (IS_CDS_ITEM(objectType)) {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        Ref<CdsObject> clone = CdsObject::createObject(storage, objectType);
        item->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);
        if (string_ok(location))
            clone->setLocation(location);

        Ref<CdsItem> cloned_item = RefCast(clone, CdsItem);

        if (string_ok(mimetype) && (string_ok(protocol))) {
            cloned_item->setMimeType(mimetype);
            Ref<CdsResource> resource = cloned_item->getResource(0);
            resource->addAttribute("protocolInfo", renderProtocolInfo(mimetype, protocol));
        } else if (!string_ok(mimetype) && (string_ok(protocol))) {
            Ref<CdsResource> resource = cloned_item->getResource(0);
            resource->addAttribute("protocolInfo", renderProtocolInfo(cloned_item->getMimeType(), protocol));
        } else if (string_ok(mimetype) && (!string_ok(protocol))) {
            cloned_item->setMimeType(mimetype);
            Ref<CdsResource> resource = cloned_item->getResource(0);
            std::vector<std::string> parts = split_string(resource->getAttribute("protocolInfo"), ':');
            protocol = parts[0];
            resource->addAttribute("protocolInfo", renderProtocolInfo(mimetype, protocol));
        }

        if (string_ok(description)) {
            cloned_item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), description);
        } else {
            cloned_item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
        }

        log_debug("updateObject: checking equality of item %s\n", item->getTitle().c_str());
        if (!item->equals(clone, true)) {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            update_manager->containerChanged(containerChanged);
            session_manager->containerChangedUI(containerChanged);
            log_debug("updateObject: calling containerChanged on item %s\n", item->getTitle().c_str());
            update_manager->containerChanged(item->getParentID());
        }
    }
    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        std::string action = parameters->get("action");
        std::string state = parameters->get("state");

        Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
        Ref<CdsObject> clone = CdsObject::createObject(storage, objectType);
        item->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);

        Ref<CdsActiveItem> cloned_item = RefCast(clone, CdsActiveItem);

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

        if (!item->equals(clone, true)) {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            update_manager->containerChanged(containerChanged);
            session_manager->containerChangedUI(containerChanged);
            update_manager->containerChanged(item->getParentID());
        }
    } else if (IS_CDS_CONTAINER(objectType)) {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        Ref<CdsObject> clone = CdsObject::createObject(storage, objectType);
        cont->copyTo(clone);

        if (string_ok(title))
            clone->setTitle(title);
        if (string_ok(upnp_class))
            clone->setClass(upnp_class);

        if (!cont->equals(clone, true)) {
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

void ContentManager::addObject(zmm::Ref<CdsObject> obj)
{
    obj->validate();

    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is %d\n", obj->getParentID());
    if (!IS_CDS_ITEM_EXTERNAL_URL(obj->getObjectType())) {
        obj->setLocation(reduce_string(obj->getLocation(), DIR_SEPARATOR));
    }

    storage->addObject(obj, &containerChanged);
    log_debug("After adding: parent ID is %d\n", obj->getParentID());

    update_manager->containerChanged(containerChanged);
    session_manager->containerChangedUI(containerChanged);

    int parent_id = obj->getParentID();
    if ((parent_id != -1) && (storage->getChildCount(parent_id) == 1)) {
        Ref<CdsObject> parent; //(new CdsObject());
        parent = storage->loadObject(parent_id);
        log_debug("Will update ID %d\n", parent->getParentID());
        update_manager->containerChanged(parent->getParentID());
    }

    update_manager->containerChanged(obj->getParentID());
    if (IS_CDS_CONTAINER(obj->getObjectType()))
        session_manager->containerChangedUI(obj->getParentID());

    if (!obj->isVirtual() && IS_CDS_ITEM(obj->getObjectType()))
        getAccounting()->totalFiles++;
}

void ContentManager::addContainer(int parentID, std::string title, std::string upnpClass)
{
    addContainerChain(storage->buildContainerPath(parentID, escape(title, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR)), upnpClass);
}

int ContentManager::addContainerChain(std::string chain, std::string lastClass, int lastRefID, Ref<Dictionary> lastMetadata)
{
    int updateID = INVALID_OBJECT_ID;
    int containerID;

    if (!string_ok(chain))
        throw _Exception("addContainerChain() called with empty chain parameter");

    log_debug("received chain: %s (%s) [%s]\n", chain.c_str(), lastClass.c_str(), lastMetadata != nullptr ? lastMetadata->encodeSimple().c_str() : "null");
    storage->addContainerChain(chain, lastClass, lastRefID, &containerID, &updateID, lastMetadata);

    // if (updateID != INVALID_OBJECT_ID)
    // an invalid updateID is checked by containerChanged()
    update_manager->containerChanged(updateID);
    session_manager->containerChangedUI(updateID);

    return containerID;
}

void ContentManager::updateObject(Ref<CdsObject> obj, bool send_updates)
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

Ref<CdsObject> ContentManager::convertObject(Ref<CdsObject> oldObj, int newType)
{
    int oldType = oldObj->getObjectType();
    if (oldType == newType)
        return oldObj;
    if (!IS_CDS_ITEM(oldType) || !IS_CDS_ITEM(newType)) {
        throw _Exception("Cannot convert object type " + std::to_string(oldType) + " to " + std::to_string(newType));
    }

    Ref<CdsObject> newObj = CdsObject::createObject(storage, newType);

    oldObj->copyTo(newObj);

    return newObj;
}

// returns nullptr if file ignored due to configuration
Ref<CdsObject> ContentManager::createObjectFromFile(std::string path, bool magic, bool allow_fifo)
{
    std::string filename = get_filename(path);

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw _Exception("Failed to stat " + path + " , " + mt_strerror(errno));
    }

    Ref<CdsObject> obj;
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

        Ref<CdsItem> item(new CdsItem(storage));
        obj = RefCast(item, CdsObject);
        item->setLocation(path);
        item->setMTime(statbuf.st_mtime);
        item->setSizeOnDisk(statbuf.st_size);

        if (!mimetype.empty()) {
            item->setMimeType(mimetype);
        }
        if (!upnp_class.empty()) {
            item->setClass(upnp_class);
        }

        Ref<StringConverter> f2i = StringConverter::f2i(config);
        obj->setTitle(f2i->convert(filename));

        if (magic) {
            MetadataHandler::setMetadata(config, item);
        }
    } else if (S_ISDIR(statbuf.st_mode)) {
        Ref<CdsContainer> cont(new CdsContainer(storage));
        obj = RefCast(cont, CdsObject);
        /* adding containers is done by Storage now
         * this exists only to inform the caller that
         * this is a container
         */
        /*
        cont->setLocation(path);
        Ref<StringConverter> f2i = StringConverter::f2i();
        obj->setTitle(f2i->convert(filename));
        */
    } else {
        // only regular files and directories are supported
        throw _Exception("ContentManager: skipping file " + path);
    }
    //    Ref<StringConverter> f2i = StringConverter::f2i();
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
                    layout = Ref<Layout>((Layout*)new JSLayout(config, storage, self, scripting_runtime));
#else
                    log_error("Cannot init layout: Gerbera compiled without JS support, but JS was requested.");
#endif
                } else if (layout_type == "builtin") {
                    layout = Ref<Layout>((FallbackLayout*)new FallbackLayout(config, storage, self));
                }
            } catch (const Exception& e) {
                layout = nullptr;
                log_error("ContentManager virtual container layout: %s\n", e.getMessage().c_str());
            }
    }
}

#ifdef HAVE_JS
void ContentManager::initJS()
{
    if (playlist_parser_script == nullptr) {
        auto self = shared_from_this();
        playlist_parser_script = Ref<PlaylistParserScript>(new PlaylistParserScript(config, storage, self, scripting_runtime));
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
    Ref<GenericTask> task;
    AutoLockU lock(mutex);
    working = true;
    while (!shutdownFlag) {
        currentTask = nullptr;
        if (((task = taskQueue1->dequeue()) == nullptr) && ((task = taskQueue2->dequeue()) == nullptr)) {
            working = false;
            /* if nothing to do, sleep until awakened */
            cond.wait(lock);
            working = true;
            continue;
        } else {
            currentTask = task;
        }
        lock.unlock();

        // log_debug("content manager Async START %s\n", task->getDescription().c_str());
        try {
            if (task->isValid())
                task->run();
        } catch (const ServerShutdownException& se) {
            shutdownFlag = true;
        } catch (const Exception& e) {
            log_error("Exception caught: %s\n", e.getMessage().c_str());
            e.printStackTrace();
        }
        // log_debug("content manager ASYNC STOP  %s\n", task->getDescription().c_str());

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

void ContentManager::addTask(zmm::Ref<GenericTask> task, bool lowPriority)
{
    AutoLock lock(mutex);

    task->setID(taskID++);

    if (!lowPriority)
        taskQueue1->enqueue(task);
    else
        taskQueue2->enqueue(task);
    signal();
}

/* sync / async methods */
void ContentManager::loadAccounting(bool async)
{
    if (async) {
        auto self = shared_from_this();
        Ref<GenericTask> task(new CMLoadAccountingTask(self));
        task->setDescription("Initializing statistics");
        addTask(task);
    } else {
        _loadAccounting();
    }
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
        Ref<GenericTask> task(new CMAddFileTask(self, path, rootpath, recursive, hidden, cancellable));
        task->setDescription("Importing: " + path);
        task->setParentID(parentTaskID);
        addTask(task, lowPriority);
        return INVALID_OBJECT_ID;
    } else {
        return _addFile(path, rootpath, recursive, hidden);
    }
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(service_type_t service, bool lowPriority, bool cancellable, bool unscheduled_refresh)
{
    Ref<OnlineService> os = online_services->getService(service);
    if (os == nullptr) {
        log_debug("No surch service! %d\n", service);
        throw _Exception("Service not found!");
    }
    fetchOnlineContentInternal(os, lowPriority, cancellable, 0, unscheduled_refresh);
}

void ContentManager::fetchOnlineContentInternal(
    Ref<OnlineService> service, bool lowPriority, bool cancellable, unsigned int parentTaskID, bool unscheduled_refresh)
{
    if (layout_enabled)
        initLayout();

    auto self = shared_from_this();
    Ref<GenericTask> task(new CMFetchOnlineContentTask(self, task_processor, timer, service, layout, cancellable, unscheduled_refresh));
    task->setDescription("Updating content from " + service->getServiceName());
    task->setParentID(parentTaskID);
    service->incTaskCount();
    addTask(task, lowPriority);
}

void ContentManager::cleanupOnlineServiceObjects(zmm::Ref<OnlineService> service)
{
    log_debug("Finished fetch cycle for service: %s\n", service->getServiceName().c_str());

    if (service->getItemPurgeInterval() > 0) {
        auto ids = storage->getServiceObjectIDs(service->getStoragePrefix());

        struct timespec current, last;
        getTimespecNow(&current);
        last.tv_nsec = 0;
        std::string temp;

        for (int object_id : *ids) {
            Ref<CdsObject> obj = storage->loadObject(object_id);
            if (obj == nullptr)
                continue;

            temp = obj->getAuxData(ONLINE_SERVICE_LAST_UPDATE);
            if (!string_ok(temp))
                continue;

            last.tv_sec = std::stol(temp);

            if ((service->getItemPurgeInterval() > 0) && ((current.tv_sec - last.tv_sec) > service->getItemPurgeInterval())) {
                log_debug("Purging old online service object %s\n", obj->getTitle().c_str());
                removeObject(object_id, false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(Ref<GenericTask> t, std::string path)
{
    if (t->getType() == AddFile) {
        log_debug("comparing, task path: %s, remove path: %s\n", RefCast(t, CMAddFileTask)->getPath().c_str(), path.c_str());
        if (startswith(RefCast(t, CMAddFileTask)->getPath(), path)) {
            log_debug("Invalidating task with path %s\n", RefCast(t, CMAddFileTask)->getPath().c_str());
            t->invalidate();
        }
    }
}

void ContentManager::invalidateTask(unsigned int taskID, task_owner_t taskOwner)
{
    int i;

    if (taskOwner == ContentManagerTask) {
        AutoLock lock(mutex);
        Ref<GenericTask> t = getCurrentTask();
        if (t != nullptr) {
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }

        int qsize = taskQueue1->size();

        for (i = 0; i < qsize; i++) {
            Ref<GenericTask> t = taskQueue1->get(i);
            if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
                t->invalidate();
            }
        }

        qsize = taskQueue2->size();
        for (i = 0; i < qsize; i++) {
            Ref<GenericTask> t = taskQueue2->get(i);
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
        Ref<Array<CdsObject> > objectPath = storage->getObjectPath(objectID);
        std::ostringstream desc;
        desc << "Removing ";
        // skip root container, start from 1
        for (int i = 1; i < objectPath->size(); i++)
            desc << '/' << objectPath->get(i)->getTitle();
        */
        auto self = shared_from_this();
        Ref<GenericTask> task(new CMRemoveObjectTask(self, objectID, all));
        std::string path;
        Ref<CdsObject> obj;

        try {
            obj = storage->loadObject(objectID);
            path = obj->getLocation();

            std::string vpath = obj->getVirtualPath();
            if (string_ok(vpath))
                task->setDescription("Removing: " + obj->getVirtualPath());
        } catch (const Exception& e) {
            log_debug("trying to remove an object ID which is no longer in the database! %d\n", objectID);
            return;
        }

        if (IS_CDS_CONTAINER(obj->getObjectType())) {
            int i;

            // make sure to remove possible child autoscan directories from the scanlist
            Ref<AutoscanList> rm_list = autoscan_timed->removeIfSubdir(path);
            for (i = 0; i < rm_list->size(); i++) {
                timer->removeTimerSubscriber(this, rm_list->get(i)->getTimerParameter(), true);
            }
#ifdef HAVE_INOTIFY
            if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
                rm_list = autoscan_inotify->removeIfSubdir(path);
                for (i = 0; i < rm_list->size(); i++) {
                    Ref<AutoscanDirectory> dir = rm_list->get(i);
                    inotify->unmonitor(dir);
                }
            }
#endif

            AutoLock lock(mutex);
            int qsize = taskQueue1->size();

            // we have to make sure that a currently running autoscan task will not
            // launch add tasks for directories that anyway are going to be deleted
            for (i = 0; i < qsize; i++) {
                Ref<GenericTask> t = taskQueue1->get(i);
                invalidateAddTask(t, path);
            }

            qsize = taskQueue2->size();
            for (i = 0; i < qsize; i++) {
                Ref<GenericTask> t = taskQueue2->get(i);
                invalidateAddTask(t, path);
            }

            Ref<GenericTask> t = getCurrentTask();
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
    Ref<GenericTask> task(new CMRescanDirectoryTask(self, objectID, scanID, scanMode, cancellable));
    Ref<AutoscanDirectory> dir = getAutoscanDirectory(scanID, scanMode);
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

Ref<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, ScanMode scanMode)
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

Ref<Array<AutoscanDirectory>> ContentManager::getAutoscanDirectories(ScanMode scanMode)
{
    if (scanMode == ScanMode::Timed) {
        return autoscan_timed->getArrayCopy();
    }

#if HAVE_INOTIFY
    else if (scanMode == ScanMode::INotify) {
        return autoscan_inotify->getArrayCopy();
    }
#endif
    return nullptr;
}

Ref<Array<AutoscanDirectory>> ContentManager::getAutoscanDirectories()
{
    Ref<Array<AutoscanDirectory>> all = autoscan_timed->getArrayCopy();

#if HAVE_INOTIFY
    Ref<Array<AutoscanDirectory>> ino = autoscan_inotify->getArrayCopy();
    if (ino != nullptr)
        for (int i = 0; i < ino->size(); i++)
            all->append(ino->get(i));
#endif
    return all;
}

Ref<AutoscanDirectory> ContentManager::getAutoscanDirectory(std::string location)
{
    // \todo change this when more scanmodes become available
    Ref<AutoscanDirectory> dir = autoscan_timed->get(location);
#if HAVE_INOTIFY
    if (dir == nullptr)
        dir = autoscan_inotify->get(location);
#endif
    return dir;
}

void ContentManager::removeAutoscanDirectory(int scanID, ScanMode scanMode)
{
    if (scanMode == ScanMode::Timed) {
        Ref<AutoscanDirectory> adir = autoscan_timed->get(scanID);
        if (adir == nullptr)
            throw _Exception("can not remove autoscan directory - was not an autoscan");

        autoscan_timed->remove(scanID);
        storage->removeAutoscanDirectory(adir->getStorageID());
        session_manager->containerChangedUI(adir->getObjectID());

        // if 3rd parameter is true: won't fail if scanID doesn't exist
        timer->removeTimerSubscriber(this, adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (scanMode == ScanMode::INotify) {
            Ref<AutoscanDirectory> adir = autoscan_inotify->get(scanID);
            if (adir == nullptr)
                throw _Exception("can not remove autoscan directory - was not an autoscan");
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
    Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(objectID);
    if (adir == nullptr)
        throw _Exception("can not remove autoscan directory - was not an autoscan");

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
    Ref<AutoscanDirectory> adir = autoscan_timed->get(location);
#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        if (adir == nullptr)
            adir = autoscan_inotify->get(location);
    }
#endif
    if (adir == nullptr)
        throw _Exception("can not remove autoscan directory - was not an autoscan");

    removeAutoscanDirectory(adir->getObjectID());
}

void ContentManager::handlePeristentAutoscanRemove(int scanID, ScanMode scanMode)
{
    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
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
    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    int id = ensurePathExistence(adir->getLocation());
    adir->setObjectID(id);
    storage->updateAutoscanDirectory(adir);
}

void ContentManager::setAutoscanDirectory(Ref<AutoscanDirectory> dir)
{
    Ref<AutoscanDirectory> original;

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
            log_debug("objectID: %d\n", dir->getObjectID());
            Ref<CdsObject> obj = storage->loadObject(dir->getObjectID());
            if (obj == nullptr || !IS_CDS_CONTAINER(obj->getObjectType()) || obj->isVirtual())
                throw _Exception("tried to remove an illegal object (id) from the list of the autoscan directories");

            log_debug("location: %s\n", obj->getLocation().c_str());

            if (!string_ok(obj->getLocation()))
                throw _Exception("tried to add an illegal object as autoscan - no location information available!");

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

    Ref<AutoscanDirectory> copy(new AutoscanDirectory());
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

void ContentManager::triggerPlayHook(zmm::Ref<CdsObject> obj)
{
    log_debug("start\n");

    if (config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && !obj->getFlag(OBJECT_FLAG_PLAYED)) {
        std::vector<std::string>  mark_list = config->getStringArrayOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        for (size_t i = 0; i < mark_list.size(); i++) {
            if (startswith(RefCast(obj, CdsItem)->getMimeType(), mark_list[i])) {
                obj->setFlag(OBJECT_FLAG_PLAYED);

                bool supress = config->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
                log_debug("Marking object %s as played\n", obj->getTitle().c_str());
                updateObject(obj, !supress);
                break;
            }
        }
    }

#ifdef HAVE_LASTFMLIB
    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED) && (RefCast(obj, CdsItem)->getMimeType().startsWith("audio")))
        last_fm->startedPlaying(RefCast(obj, CdsItem));
#endif
    log_debug("end\n");
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
    log_debug("running add file task with path %s recursive: %d\n", path.c_str(), recursive);
    content->_addFile(path, rootpath, recursive, hidden, Ref<GenericTask>(this));
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
    Ref<AutoscanDirectory> dir = content->getAutoscanDirectory(scanID, scanMode);
    if (dir == nullptr)
        return;

    content->_rescanDirectory(objectID, dir->getScanID(), dir->getScanMode(), dir->getScanLevel(), Ref<GenericTask>(this));
    dir->decTaskCount();

    if (dir->getTaskCount() == 0) {
        dir->updateLMT();
    }
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> task_processor, std::shared_ptr<Timer> timer,
    Ref<OnlineService> service, Ref<Layout> layout, bool cancellable, bool unscheduled_refresh)
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
        log_debug("Received invalid service!\n");
        return;
    }
    try {
        Ref<GenericTask> t(
            new TPFetchOnlineContentTask(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh)
        );
        task_processor->addTask(t);
    } catch (const Exception& ex) {
        log_error("%s\n", ex.getMessage().c_str());
    }
}
#endif // ONLINE_SERVICES

CMLoadAccountingTask::CMLoadAccountingTask(std::shared_ptr<ContentManager> content)
    : GenericTask(ContentManagerTask)
    , content(content)
{
    this->taskType = LoadAccounting;
}

void CMLoadAccountingTask::run()
{
    content->_loadAccounting();
}

CMAccounting::CMAccounting()
    : Object()
{
    totalFiles = 0;
}

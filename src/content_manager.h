/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    content_manager.h - this file is part of MediaTomb.
    
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

/// \file content_manager.h
#ifndef __CONTENT_MANAGER_H__
#define __CONTENT_MANAGER_H__

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "autoscan.h"
#include "cds_objects.h"
#include "common.h"
#include "util/generic_task.h"
#include "util/timer.h"

#ifdef HAVE_JS
// this is somewhat not nice, the playlist header needs the cm header and
// vice versa
class PlaylistParserScript;
#include "scripting/playlist_parser_script.h"

#endif
#include "layout/layout.h"
#ifdef HAVE_INOTIFY
#include "autoscan_inotify.h"
#endif

#include "transcoding/transcoding.h"

#ifdef ONLINE_SERVICES
#include "onlineservice/online_service.h"
#endif //ONLINE_SERVICES

#include "util/executor.h"

// forward declaration
class ConfigManager;
class Storage;
class UpdateManager;
namespace web { class SessionManager; }
class Runtime;
class LastFm;
class ContentManager;
class TaskProcessor;

class CMAddFileTask : public GenericTask, public std::enable_shared_from_this<CMAddFileTask> {
protected:
    std::shared_ptr<ContentManager> content;
    std::string path;
    std::string rootpath;
    bool recursive;
    bool hidden;

public:
    CMAddFileTask(std::shared_ptr<ContentManager> content,
        std::string path, std::string rootpath, bool recursive = false,
        bool hidden = false, bool cancellable = true);
    std::string getPath();
    std::string getRootPath();
    virtual void run() override;
};

class CMRemoveObjectTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    int objectID;
    bool all;

public:
    CMRemoveObjectTask(std::shared_ptr<ContentManager> content,
        int objectID, bool all);
    virtual void run() override;
};

class CMRescanDirectoryTask : public GenericTask, public std::enable_shared_from_this<CMRescanDirectoryTask> {
protected:
    std::shared_ptr<ContentManager> content;
    int objectID;
    int scanID;
    ScanMode scanMode;

public:
    CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
        int objectID, int scanID, ScanMode scanMode,
        bool cancellable);
    virtual void run() override;
};

#ifdef ONLINE_SERVICES
class CMFetchOnlineContentTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;
    zmm::Ref<OnlineService> service;
    zmm::Ref<Layout> layout;
    bool unscheduled_refresh;

public:
    CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<TaskProcessor> task_processor,
        std::shared_ptr<Timer> timer,
        zmm::Ref<OnlineService> service, zmm::Ref<Layout> layout,
        bool cancellable, bool unscheduled_refresh);
    virtual void run() override;
};
#endif

class ContentManager : public Timer::Subscriber, public std::enable_shared_from_this<ContentManager> {
public:
    ContentManager(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
        std::shared_ptr<UpdateManager> update_manager, std::shared_ptr<web::SessionManager> session_manager,
        std::shared_ptr<Timer> timer, std::shared_ptr<TaskProcessor> task_processor,
        std::shared_ptr<Runtime> scripting_runtime, std::shared_ptr<LastFm> last_fm);
    void init();
    virtual ~ContentManager();
    void shutdown();

    virtual void timerNotify(std::shared_ptr<Timer::Parameter> parameter) override;

    bool isBusy() { return working; }

    /// \brief Returns the task that is currently being executed.
    std::shared_ptr<GenericTask> getCurrentTask();

    /// \brief Returns the list of all enqueued tasks, including the current or nullptr if no tasks are present.
    std::deque<std::shared_ptr<GenericTask>> getTasklist();

    /// \brief Find a task identified by the task ID and invalidate it.
    void invalidateTask(unsigned int taskID, task_owner_t taskOwner = ContentManagerTask);

    /* the functions below return true if the task has been enqueued */

    /// \brief Adds a file or directory to the database.
    /// \param path absolute path to the file
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(std::string path, bool recursive = true, bool async = true,
        bool hidden = false, bool lowPriority = false,
        bool cancellable = true);

    /// \brief Adds a file or directory to the database.
    /// \param path absolute path to the file
    /// \param rootpath absolute path to the container root
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(std::string path, std::string rootpath, bool recursive = true, bool async = true,
        bool hidden = false, bool lowPriority = false,
        bool cancellable = true);

    int ensurePathExistence(std::string path);
    void removeObject(int objectID, bool async = true, bool all = false);
    void rescanDirectory(int objectID, int scanID, ScanMode scanMode,
        std::string descPath = "", bool cancellable = true);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    void updateObject(int objectID, const std::map<std::string,std::string>& parameters);

    std::shared_ptr<CdsObject> createObjectFromFile(std::string path,
        bool magic = true,
        bool allow_fifo = false);

#ifdef ONLINE_SERVICES
    /// \brief Creates a layout based from data that is obtained from an
    /// online service (like YouTube, SopCast, etc.)
    void fetchOnlineContent(service_type_t service, bool lowPriority = true,
        bool cancellable = true,
        bool unscheduled_refresh = false);

    void cleanupOnlineServiceObjects(zmm::Ref<OnlineService> service);

#endif //ONLINE_SERVICES

    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allow_fifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    void addVirtualItem(std::shared_ptr<CdsObject> obj, bool allow_fifo = false);

    /// \brief Adds an object to the database.
    /// \param obj object to add
    ///
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method
    void addObject(std::shared_ptr<CdsObject> obj);

    /// \brief Adds a virtual container chain specified by path.
    /// \param container path separated by '/'. Slashes in container
    /// titles must be escaped.
    /// \param lastClass upnp:class of the last container in the chain, if nullptr
    /// then the default class will be taken
    /// \param lastRefID reference id of the last container in the chain,
    /// INVALID_OBJECT_ID indicates that the id will not be set.
    /// \return ID of the last container in the chain.
    int addContainerChain(std::string chain, std::string lastClass = "",
        int lastRefID = INVALID_OBJECT_ID, const std::map<std::string,std::string>& lastMetadata = std::map<std::string,std::string>());

    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    void addContainer(int parentID, std::string title, std::string upnpClass);

    /// \brief Updates an object in the database.
    /// \param obj the object to update
    void updateObject(std::shared_ptr<CdsObject> obj, bool send_updates = true);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    ///
    /// Note: no actions should be performed on the object given as the parameter.
    /// Only the returned object should be processed. This method does not save
    /// the returned object in the database. To do so updateObject must be called
    std::shared_ptr<CdsObject> convertObject(std::shared_ptr<CdsObject> obj, int objectType);

    /// \brief Gets an AutocsanDirectrory from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int scanID, ScanMode scanMode);

    /// \brief Get an AutoscanDirectory given by location on disk from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(std::string location);
    /// \brief Removes an AutoscanDirectrory (found by scanID) from the watch list.
    void removeAutoscanDirectory(int scanID, ScanMode scanMode);

    /// \brief Removes an AutoscanDirectrory (found by location) from the watch list.
    void removeAutoscanDirectory(std::string location);

    /// \brief Removes an AutoscanDirectory (by objectID) from the watch list.
    void removeAutoscanDirectory(int objectID);

    /// \brief Update autoscan parameters for an existing autoscan directory
    /// or add a new autoscan directory
    void setAutoscanDirectory(std::shared_ptr<AutoscanDirectory> dir);

    /// \brief handles the removal of a persistent autoscan directory
    void handlePeristentAutoscanRemove(int scanID, ScanMode scanMode);

    /// \brief handles the recreation of a persistent autoscan directory
    void handlePersistentAutoscanRecreate(int scanID, ScanMode scanMode);

    /// \brief returns an array of autoscan directories for the given scan mode
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanDirectories(ScanMode scanMode);

    /// \brief returns an array of all autoscan directories
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanDirectories();

    /// \brief instructs ContentManager to reload scripting environment
    void reloadLayout();

    /// \brief register executor
    ///
    /// When an external process is launched we will register the executor
    /// the content manager. This will ensure that we can kill all processes
    /// when we shutdown the server.
    ///
    /// \param exec the Executor object of the process
    void registerExecutor(std::shared_ptr<Executor> exec);

    /// \brief unregister process
    ///
    /// When the the process io handler receives a close on a stream that is
    /// currently being processed by an external process, it will kill it.
    /// The handler will then remove the executor from the list.
    void unregisterExecutor(std::shared_ptr<Executor> exec);

    void triggerPlayHook(std::shared_ptr<CdsObject> obj);

protected:
    void initLayout();
    void destroyLayout();

#ifdef HAVE_JS
    void initJS();
    void destroyJS();
#endif

    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<Storage> storage;
    std::shared_ptr<UpdateManager> update_manager;
    std::shared_ptr<web::SessionManager> session_manager;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Runtime> scripting_runtime;
    std::shared_ptr<LastFm> last_fm;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
    using AutoLockU = std::unique_lock<decltype(mutex)>;

    zmm::Ref<RExp> reMimetype;

    bool ignore_unknown_extensions;
    bool extension_map_case_sensitive;

    std::map<std::string,std::string> extension_mimetype_map;
    std::map<std::string,std::string> mimetype_upnpclass_map;
    std::map<std::string,std::string> mimetype_contenttype_map;

    std::shared_ptr<AutoscanList> autoscan_timed;
#ifdef HAVE_INOTIFY
    std::unique_ptr<AutoscanInotify> inotify;
    std::shared_ptr<AutoscanList> autoscan_inotify;
#endif

    std::vector<std::shared_ptr<Executor>> process_list;

    int addFileInternal(std::string path, std::string rootpath,
        bool recursive = true,
        bool async = true, bool hidden = false,
        bool lowPriority = false,
        unsigned int parentTaskID = 0,
        bool cancellable = true);
    int _addFile(std::string path, std::string rootPath, bool recursive = false, bool hidden = false, std::shared_ptr<CMAddFileTask> task = nullptr);
    //void _addFile2(std::string path, bool recursive=0);
    void _removeObject(int objectID, bool all);

    void _rescanDirectory(int containerID, int scanID, ScanMode scanMode, ScanLevel scanLevel, std::shared_ptr<GenericTask> task = nullptr);
    /* for recursive addition */
    void addRecursive(std::string path, bool hidden, std::shared_ptr<CMAddFileTask> task);
    //void addRecursive2(zmm::Ref<DirCache> dirCache, std::string filename, bool recursive);

    std::string extension2mimetype(std::string extension);
    std::string mimetype2upnpclass(std::string mimeType);

    void invalidateAddTask(std::shared_ptr<GenericTask> t, std::string path);

    zmm::Ref<Layout> layout;

#ifdef ONLINE_SERVICES
    zmm::Ref<OnlineServiceList> online_services;

    void fetchOnlineContentInternal(zmm::Ref<OnlineService> service,
        bool lowPriority = true,
        bool cancellable = true,
        unsigned int parentTaskID = 0,
        bool unscheduled_refresh = false);

#endif //ONLINE_SERVICES

#ifdef HAVE_JS
    std::unique_ptr<PlaylistParserScript> playlist_parser_script;
#endif

    bool layout_enabled;

    void setLastModifiedTime(time_t lm);

    inline void signal() { cond.notify_one(); }
    static void* staticThreadProc(void* arg);
    void threadProc();

    void addTask(std::shared_ptr<GenericTask> task, bool lowPriority = false);

    pthread_t taskThread;
    std::condition_variable_any cond;

    bool working;

    bool shutdownFlag;

    std::deque<std::shared_ptr<GenericTask>> taskQueue1; // priority 1
    std::deque<std::shared_ptr<GenericTask>> taskQueue2; // priority 2
    std::shared_ptr<GenericTask> currentTask;

    unsigned int taskID;

    friend void CMAddFileTask::run();
    friend void CMRemoveObjectTask::run();
    friend void CMRescanDirectoryTask::run();
#ifdef ONLINE_SERVICES
    friend void CMFetchOnlineContentTask::run();
#endif
};

#endif // __CONTENT_MANAGER_H__

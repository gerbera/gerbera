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

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

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

#include "config/directory_tweak.h"
#include "transcoding/transcoding.h"

#ifdef ONLINE_SERVICES
#include "onlineservice/online_service.h"
#endif //ONLINE_SERVICES

#include "util/executor.h"

// forward declaration
class Config;
class Database;
class UpdateManager;
class Mime;
namespace web {
class SessionManager;
} // namespace web
class Runtime;
class LastFm;
class ContentManager;
class TaskProcessor;

class CMAddFileTask : public GenericTask, public std::enable_shared_from_this<CMAddFileTask> {
protected:
    std::shared_ptr<ContentManager> content;
    fs::path path;
    fs::path rootpath;
    AutoScanSetting asSetting;

public:
    CMAddFileTask(std::shared_ptr<ContentManager> content,
        fs::path path, fs::path rootpath, AutoScanSetting& asSetting, bool cancellable = true);
    fs::path getPath();
    fs::path getRootPath();
    void run() override;
};

class CMRemoveObjectTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<AutoscanDirectory> adir;
    int objectID;
    bool all;
    bool rescanResource;

public:
    CMRemoveObjectTask(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> adir,
        int objectID, bool rescanResource, bool all);
    void run() override;
};

class CMRescanDirectoryTask : public GenericTask, public std::enable_shared_from_this<CMRescanDirectoryTask> {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<AutoscanDirectory> adir;
    int containerID;

public:
    CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<AutoscanDirectory> adir, int containerId, bool cancellable);
    void run() override;
};

#ifdef ONLINE_SERVICES
class CMFetchOnlineContentTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<OnlineService> service;
    std::shared_ptr<Layout> layout;
    bool unscheduled_refresh;

public:
    CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<TaskProcessor> task_processor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout,
        bool cancellable, bool unscheduled_refresh);
    void run() override;
};
#endif

class ContentManager : public Timer::Subscriber, public std::enable_shared_from_this<ContentManager> {
public:
    ContentManager(const std::shared_ptr<Config>& config, const std::shared_ptr<Database>& database,
        std::shared_ptr<UpdateManager> update_manager, std::shared_ptr<web::SessionManager> session_manager,
        std::shared_ptr<Timer> timer, std::shared_ptr<TaskProcessor> task_processor,
        std::shared_ptr<Runtime> scripting_runtime, std::shared_ptr<LastFm> last_fm);
    void run();
    ~ContentManager() override;
    void shutdown();

    void timerNotify(std::shared_ptr<Timer::Parameter> parameter) override;

    bool isBusy() const { return working; }

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
    /// \param rescanResource true allows to reload a directory containing a resource
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(const fs::path& path, AutoScanSetting& asSetting,
        bool async = true, bool lowPriority = false, bool cancellable = true);

    /// \brief Adds a file or directory to the database.
    /// \param path absolute path to the file
    /// \param rootpath absolute path to the container root
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param rescanResource true allows to reload a directory containing a resource
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(const fs::path& path, const fs::path& rootpath, AutoScanSetting& asSetting,
        bool async = true, bool lowPriority = false, bool cancellable = true);

    int ensurePathExistence(fs::path path);
    void removeObject(std::shared_ptr<AutoscanDirectory> adir, int objectID, bool rescanResource, bool async = true, bool all = false);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    void updateObject(int objectID, const std::map<std::string, std::string>& parameters);

    // returns nullptr if file does not exist or is ignored due to configuration
    std::shared_ptr<CdsObject> createObjectFromFile(const fs::path& path, bool followSymlinks,
        bool magic = true,
        bool allow_fifo = false);

#ifdef ONLINE_SERVICES
    /// \brief Creates a layout based from data that is obtained from an
    /// online service (like YouTube, SopCast, etc.)
    void fetchOnlineContent(service_type_t service, bool lowPriority = true,
        bool cancellable = true,
        bool unscheduled_refresh = false);

    void cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service);

#endif //ONLINE_SERVICES

    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allow_fifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    void addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allow_fifo = false);

    /// \brief Adds an object to the database.
    /// \param obj object to add
    ///
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method
    void addObject(const std::shared_ptr<CdsObject>& obj);

    /// \brief Adds a virtual container chain specified by path.
    /// \param container path separated by '/'. Slashes in container
    /// titles must be escaped.
    /// \param lastClass upnp:class of the last container in the chain, if nullptr
    /// then the default class will be taken
    /// \param lastRefID reference id of the last container in the chain,
    /// INVALID_OBJECT_ID indicates that the id will not be set.
    /// \return ID of the last container in the chain.
    int addContainerChain(const std::string& chain, const std::string& lastClass = "",
        int lastRefID = INVALID_OBJECT_ID, const std::map<std::string, std::string>& lastMetadata = std::map<std::string, std::string>());

    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    void addContainer(int parentID, std::string title, const std::string& upnpClass);

    /// \brief Updates an object in the database.
    /// \param obj the object to update
    void updateObject(const std::shared_ptr<CdsObject>& obj, bool send_updates = true);

    /// \brief Gets an AutocsanDirectrory from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int scanID, ScanMode scanMode) const;

    /// \brief Gets an AutoscanDirectory (by objectID) from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID);

    /// \brief Get an AutoscanDirectory given by location on disk from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(const fs::path& location) const;

    /// \brief returns an array of all autoscan directories
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanDirectories() const;

    /// \brief Removes an AutoscanDirectrory (found by scanID) from the watch list.
    void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir);

    /// \brief Update autoscan parameters for an existing autoscan directory
    /// or add a new autoscan directory
    void setAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& dir);

    /// \brief handles the removal of a persistent autoscan directory
    void handlePeristentAutoscanRemove(const std::shared_ptr<AutoscanDirectory>& adir);

    /// \brief handles the recreation of a persistent autoscan directory
    void handlePersistentAutoscanRecreate(const std::shared_ptr<AutoscanDirectory>& adir);

    void rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, std::string descPath = "", bool cancellable = true);

    /// \brief instructs ContentManager to reload scripting environment
    void reloadLayout();

    /// \brief register executor
    ///
    /// When an external process is launched we will register the executor
    /// the content manager. This will ensure that we can kill all processes
    /// when we shutdown the server.
    ///
    /// \param exec the Executor object of the process
    void registerExecutor(const std::shared_ptr<Executor>& exec);

    /// \brief unregister process
    ///
    /// When the the process io handler receives a close on a stream that is
    /// currently being processed by an external process, it will kill it.
    /// The handler will then remove the executor from the list.
    void unregisterExecutor(const std::shared_ptr<Executor>& exec);

    void triggerPlayHook(const std::shared_ptr<CdsObject>& obj);

    void initLayout();
    void destroyLayout();

#ifdef HAVE_JS
    void initJS();
    void destroyJS();
#endif

    std::shared_ptr<Config> getConfig() const
    {
        return config;
    }
    std::shared_ptr<Mime> getMime() const
    {
        return mime;
    }
    std::shared_ptr<Database> getDatabase() const
    {
        return database;
    }
    std::shared_ptr<UpdateManager> getUpdateManager() const
    {
        return update_manager;
    }
    std::shared_ptr<web::SessionManager> getSessionManager() const
    {
        return session_manager;
    }

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<UpdateManager> update_manager;
    std::shared_ptr<web::SessionManager> session_manager;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Runtime> scripting_runtime;
    std::shared_ptr<LastFm> last_fm;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
    using AutoLockU = std::unique_lock<decltype(mutex)>;

    bool ignore_unknown_extensions;
    std::map<std::string, std::string> mimetype_contenttype_map;

    std::shared_ptr<AutoscanList> autoscan_timed;
#ifdef HAVE_INOTIFY
    std::unique_ptr<AutoscanInotify> inotify;
    std::shared_ptr<AutoscanList> autoscan_inotify;
#endif

    std::vector<std::shared_ptr<Executor>> process_list;

    int addFileInternal(const fs::path& path, const fs::path& rootpath,
        AutoScanSetting& asSetting,
        bool async = true,
        bool lowPriority = false,
        unsigned int parentTaskID = 0,
        bool cancellable = true);
    int _addFile(const fs::path& path, fs::path rootPath, AutoScanSetting& asSetting,
        const std::shared_ptr<CMAddFileTask>& task = nullptr);

    void _removeObject(std::shared_ptr<AutoscanDirectory> adir, int objectID, bool rescanResource, bool all);

    void _rescanDirectory(std::shared_ptr<AutoscanDirectory>& adir, int containerID, const std::shared_ptr<GenericTask>& task = nullptr);
    /* for recursive addition */
    void addRecursive(std::shared_ptr<AutoscanDirectory>& adir, const fs::path& path, bool followSymlinks, bool hidden, const std::shared_ptr<CMAddFileTask>& task);
    static bool isLink(const fs::path& path, bool allowLinks);
    std::shared_ptr<CdsObject> createSingleItem(const fs::path& path, fs::path& rootPath, bool followSymlinks, bool checkDatabase, bool processExisting, const std::shared_ptr<CMAddFileTask>& task);
    bool updateAttachedResources(std::shared_ptr<AutoscanDirectory> adir, const char* location, const std::string& parentPath, bool all);

    static void invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path);

    template <typename T>
    void updateCdsObject(std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters);

    std::shared_ptr<Layout> layout;

#ifdef ONLINE_SERVICES
    std::unique_ptr<OnlineServiceList> online_services;
#endif //ONLINE_SERVICES

#ifdef HAVE_JS
    std::unique_ptr<PlaylistParserScript> playlist_parser_script;
#endif

    bool layout_enabled;

    void setLastModifiedTime(time_t lm);

    void signal() { cond.notify_one(); }
    static void* staticThreadProc(void* arg);
    void threadProc();

    void addTask(const std::shared_ptr<GenericTask>& task, bool lowPriority = false);

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

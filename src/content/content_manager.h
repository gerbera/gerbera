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

#include <deque>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include "autoscan.h"
#include "cds_objects.h"
#include "common.h"
#include "context.h"
#include "util/generic_task.h"
#include "util/thread_runner.h"
#include "util/timer.h"

#ifdef HAVE_JS
// this is somewhat not nice, the playlist header needs the cm header and
// vice versa
class PlaylistParserScript;
#include "scripting/playlist_parser_script.h"
#else
class ScriptingRuntime;
#endif // HAVE_JS

#include "layout/layout.h"

#include "autoscan_list.h"
#ifdef HAVE_INOTIFY
#include "autoscan_inotify.h"
#endif // HAVE_INOTIFY

#include "config/directory_tweak.h"
#include "transcoding/transcoding.h"

#ifdef ONLINE_SERVICES
#include "onlineservice/online_service.h"
#endif // ONLINE_SERVICES

#include "util/executor.h"

// forward declarations
class AutoScanSetting;
class ContentManager;
class LastFm;
class Server;
class TaskProcessor;

class CMAddFileTask : public GenericTask, public std::enable_shared_from_this<CMAddFileTask> {
protected:
    std::shared_ptr<ContentManager> content;
    fs::directory_entry dirEnt;
    fs::path rootpath;
    AutoScanSetting asSetting;

public:
    CMAddFileTask(std::shared_ptr<ContentManager> content,
        fs::directory_entry dirEnt, fs::path rootpath, AutoScanSetting asSetting, bool cancellable = true);
    fs::path getPath() const;
    fs::path getRootPath() const;
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
        std::shared_ptr<TaskProcessor> taskProcessor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout,
        bool cancellable, bool unscheduledRefresh);
    void run() override;
};
#endif

class UpnpMap {
private:
    std::vector<std::tuple<std::string, std::string, std::string>> filters;

    bool checkValue(const std::string& op, const std::string& expect, const std::string& actual) const;
    bool checkValue(const std::string& op, int expect, int actual) const;

public:
    std::string mimeType;
    std::string upnpClass;

    UpnpMap(std::string mt, std::string cls, std::vector<std::tuple<std::string, std::string, std::string>> f)
        : filters(std::move(f))
        , mimeType(std::move(mt))
        , upnpClass(std::move(cls))
    {
    }

    bool isMatch(const std::shared_ptr<CdsItem>& item, const std::string& mt) const;
};

class ContentManager : public Timer::Subscriber, public std::enable_shared_from_this<ContentManager> {
public:
    ContentManager(const std::shared_ptr<Context>& context,
        const std::shared_ptr<Server>& server, std::shared_ptr<Timer> timer);

    void run();
    void shutdown();

    void timerNotify(const std::shared_ptr<Timer::Parameter>& parameter) override;

    /// \brief Returns the task that is currently being executed.
    std::shared_ptr<GenericTask> getCurrentTask() const;

    /// \brief Returns the list of all enqueued tasks, including the current or nullptr if no tasks are present.
    std::deque<std::shared_ptr<GenericTask>> getTasklist();

    /// \brief Find a task identified by the task ID and invalidate it.
    void invalidateTask(unsigned int taskID, task_owner_t taskOwner = ContentManagerTask);

    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param rescanResource true allows to reload a directory containing a resource
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(const fs::directory_entry& dirEnt, AutoScanSetting& asSetting,
        bool async = true, bool lowPriority = false, bool cancellable = true);

    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param rootpath absolute path to the container root
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param rescanResource true allows to reload a directory containing a resource
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(const fs::directory_entry& dirEnt, const fs::path& rootpath, AutoScanSetting& asSetting,
        bool async = true, bool lowPriority = false, bool cancellable = true);

    int ensurePathExistence(const fs::path& path) const;
    std::vector<int> removeObject(const std::shared_ptr<AutoscanDirectory>& adir, int objectID, bool rescanResource, bool async = true, bool all = false);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    void updateObject(int objectID, const std::map<std::string, std::string>& parameters);

    // returns nullptr if file does not exist or is ignored due to configuration
    std::shared_ptr<CdsObject> createObjectFromFile(const fs::directory_entry& dirEnt, bool followSymlinks, bool allowFifo = false);

#ifdef ONLINE_SERVICES
    /// \brief Creates a layout based from data that is obtained from an
    /// online service (like AppleTrailers etc.)
    void fetchOnlineContent(service_type_t serviceType, bool lowPriority = true,
        bool cancellable = true,
        bool unscheduledRefresh = false);

    void cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service);

#endif // ONLINE_SERVICES

    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allow_fifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    void addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allowFifo = false);

    /// \brief Adds an object to the database.
    /// \param obj object to add
    /// \param firstChild indicate that this obj is the first child in parent container
    ///
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method
    void addObject(const std::shared_ptr<CdsObject>& obj, bool firstChild);

    /// \brief Adds a virtual container chain specified by path.
    /// \return ID of the last container in the chain.
    std::pair<int, bool> addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain);

    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    std::shared_ptr<CdsContainer> addContainer(int parentID, const std::string& title, const std::string& upnpClass);

    /// \brief Updates an object in the database.
    /// \param obj the object to update
    void updateObject(const std::shared_ptr<CdsObject>& obj, bool sendUpdates = true);

    /// \brief Gets an AutocsanDirectrory from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int scanID, AutoscanDirectory::ScanMode scanMode) const;

    /// \brief Gets an AutoscanDirectory (by objectID) from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) const;

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

    void rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, fs::path descPath = {}, bool cancellable = true);

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

    void triggerPlayHook(const std::string& group, const std::shared_ptr<CdsObject>& obj);

    void initLayout();
    void destroyLayout();

    /// \brief parse a file containing metadata for object
    void parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const;

#ifdef HAVE_JS
    void initJS();
    void destroyJS();
#endif

    std::shared_ptr<Context> getContext() const
    {
        return context;
    }

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<UpdateManager> update_manager;
    std::shared_ptr<Web::SessionManager> session_manager;
    std::shared_ptr<Context> context;
    ///\brief cache for containers while creating new layout
    std::map<std::string, std::shared_ptr<CdsContainer>> containerMap;
    std::map<int, std::shared_ptr<CdsContainer>> containersWithFanArt;

    std::shared_ptr<Timer> timer;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<ScriptingRuntime> scripting_runtime;
    std::shared_ptr<LastFm> last_fm;

    std::map<std::string, std::string> mimetypeContenttypeMap;
    std::map<std::string, std::string> mimetypeUpnpclassMap;
    std::vector<UpnpMap> upnpMap {};

    std::shared_ptr<AutoscanList> autoscanList;
#ifdef HAVE_INOTIFY
    std::unique_ptr<AutoscanInotify> inotify;
#endif

    std::vector<std::shared_ptr<Executor>> process_list;

    int addFileInternal(const fs::directory_entry& dirEnt, const fs::path& rootpath,
        AutoScanSetting& asSetting,
        bool async = true,
        bool lowPriority = false,
        unsigned int parentTaskID = 0,
        bool cancellable = true);
    int _addFile(const fs::directory_entry& dirEnt, fs::path rootPath, AutoScanSetting& asSetting,
        const std::shared_ptr<CMAddFileTask>& task = nullptr);

    std::vector<int> _removeObject(const std::shared_ptr<AutoscanDirectory>& adir, int objectID, bool rescanResource, bool all);
    void cleanupTasks(const fs::path& path);

    void scanDir(const std::shared_ptr<AutoscanDirectory>& dir, bool updateUI);
    void _rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int containerID, const std::shared_ptr<GenericTask>& task = nullptr);
    /* for recursive addition */
    void addRecursive(std::shared_ptr<AutoscanDirectory>& adir, const fs::directory_entry& subDir, bool followSymlinks, bool hidden, const std::shared_ptr<CMAddFileTask>& task);
    std::shared_ptr<CdsObject> createSingleItem(const fs::directory_entry& dirEnt, const fs::path& rootPath, bool followSymlinks, bool checkDatabase, bool processExisting, bool firstChild, const std::shared_ptr<AutoscanDirectory>& adir, std::shared_ptr<CMAddFileTask> task);
    bool updateAttachedResources(const std::shared_ptr<AutoscanDirectory>& adir, const std::shared_ptr<CdsObject>& obj, const fs::path& parentPath, bool all);
    void finishScan(const std::shared_ptr<AutoscanDirectory>& adir, const fs::path& location, const std::shared_ptr<CdsContainer>& parent, std::chrono::seconds lmt, const std::shared_ptr<CdsObject>& firstObject = nullptr);
    static void invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path);
    static void initUpnpMap(std::vector<UpnpMap>& target, const std::map<std::string, std::string>& source);
    std::string mimeTypeToUpnpClass(const std::string& mimeType);

    void assignFanArt(const std::shared_ptr<CdsContainer>& container, const std::shared_ptr<CdsObject>& origObj, int count);

    template <typename T>
    void updateCdsObject(const std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters);

    void updateItemData(const std::shared_ptr<CdsItem>& item, const std::string& mimetype);

    std::shared_ptr<Layout> layout;

#ifdef ONLINE_SERVICES
    std::unique_ptr<OnlineServiceList> online_services;
#endif // ONLINE_SERVICES

#ifdef HAVE_JS
    std::unique_ptr<PlaylistParserScript> playlist_parser_script;
    std::unique_ptr<MetafileParserScript> metafileParserScript;
#endif

    std::mutex layoutMutex;

    bool layoutEnabled {};
    void threadProc();

    void addTask(std::shared_ptr<GenericTask> task, bool lowPriority = false);

    std::unique_ptr<ThreadRunner<std::condition_variable_any, std::recursive_mutex>> threadRunner;

    bool working {};
    bool shutdownFlag {};

    std::deque<std::shared_ptr<GenericTask>> taskQueue1; // priority 1
    std::deque<std::shared_ptr<GenericTask>> taskQueue2; // priority 2
    std::shared_ptr<GenericTask> currentTask;

    unsigned int taskID { 1 };

    friend void CMAddFileTask::run();
    friend void CMRemoveObjectTask::run();
    friend void CMRescanDirectoryTask::run();
#ifdef ONLINE_SERVICES
    friend void CMFetchOnlineContentTask::run();
#endif
};

#endif // __CONTENT_MANAGER_H__

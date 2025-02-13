/*MT*

    MediaTomb - http://www.mediatomb.cc/

    content_manager.h - this file is part of MediaTomb.

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

/// \file content_manager.h
#ifndef __CONTENT_MANAGER_H__
#define __CONTENT_MANAGER_H__

#include "content.h"
#include "util/executor.h"
#include "util/thread_runner.h"

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

// forward declarations
#ifdef HAVE_INOTIFY
class AutoscanInotify;
#endif

#ifdef ONLINE_SERVICES
class OnlineServiceList;
enum class OnlineServiceType;
#endif // ONLINE_SERVICES

class AutoscanList;
class CdsItem;
class ConverterManager;
class CMAddFileTask;
class GenericTask;
class ImportService;
class LastFm;
class Mime;
class Server;
class TaskProcessor;
class UpdateManager;
enum class AutoscanScanMode;
namespace Web {
class SessionManager;
}

class ContentManager : public Content, public std::enable_shared_from_this<ContentManager> {
public:
    ContentManager(const std::shared_ptr<Context>& context,
        const std::shared_ptr<Server>& server, std::shared_ptr<Timer> timer);

    void run() override;
    void shutdown() override;

    void timerNotify(const std::shared_ptr<Timer::Parameter>& parameter) override;

    /// \brief Returns the task that is currently being executed.
    std::shared_ptr<GenericTask> getCurrentTask() const override;

    /// \brief Returns the list of all enqueued tasks, including the current or nullptr if no tasks are present.
    std::deque<std::shared_ptr<GenericTask>> getTasklist() override;

    /// \brief Find a task identified by the task ID and invalidate it.
    void invalidateTask(unsigned int taskID, TaskOwner taskOwner) override;

    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param asSetting Settings for import
    /// \param lowPriority for immediate processing or in normal order
    /// \param cancellable can be canceled
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    std::shared_ptr<CdsObject> addFile(const fs::directory_entry& dirEnt, AutoScanSetting& asSetting,
        bool lowPriority = false, bool cancellable = true) override;

    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param rootpath absolute path to the container root
    /// \param asSetting Settings for import
    /// \param lowPriority for immediate processing or in normal order
    /// \param cancellable can be canceled
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    std::shared_ptr<CdsObject> addFile(const fs::directory_entry& dirEnt, const fs::path& rootpath, AutoScanSetting& asSetting,
        bool lowPriority = false, bool cancellable = true) override;

    /// \brief Ensures that a container given by it's location on disk is
    /// present in the database. If it does not exist it will be created, but
    /// it's content will not be added.
    ///
    /// \param path location of the container to handle
    /// \return objectID of the container given by path
    int ensurePathExistence(const fs::path& path) const override;
    std::vector<int> removeObject(const std::shared_ptr<AutoscanDirectory>& adir, const std::shared_ptr<CdsObject>& obj, const fs::path& path, bool rescanResource, bool async = true, bool all = false) override;

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    std::shared_ptr<CdsObject> updateObject(int objectID, const std::map<std::string, std::string>& parameters) override;

    // returns nullptr if file does not exist or is ignored due to configuration
    std::shared_ptr<CdsObject> createObjectFromFile(const std::shared_ptr<AutoscanDirectory>& adir, const fs::directory_entry& dirEnt, bool followSymlinks, bool allowFifo = false) override;

#ifdef ONLINE_SERVICES
    /// \brief Creates a layout based from data that is obtained from an
    /// online service
    void fetchOnlineContent(OnlineServiceType serviceType, bool lowPriority = true,
        bool cancellable = true,
        bool unscheduledRefresh = false);

    void cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service) override;

#endif // ONLINE_SERVICES

    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allowFifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    void addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allowFifo = false) override;

    /// \brief Adds an object to the database.
    /// \param obj object to add
    /// \param firstChild indicate that this obj is the first child in parent container
    ///
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method
    void addObject(const std::shared_ptr<CdsObject>& obj, bool firstChild) override;

    /// \brief Adds a virtual container chain specified by path.
    /// \param chain list of container objects to create
    /// \param refItem object to take artwork from
    /// \return ID of the last container in the chain.
    std::pair<int, bool> addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain, const std::shared_ptr<CdsObject>& refItem) override;

    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    std::shared_ptr<CdsContainer> addContainer(int parentID, const std::string& title, const std::string& upnpClass) override;

    /// \brief Updates an object in the database.
    /// \param obj the object to update
    /// \param sendUpdates send updates to subscribed clients
    void updateObject(const std::shared_ptr<CdsObject>& obj, bool sendUpdates = true) override;

    /// \brief Gets an AutocsanDirectrory from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int scanID, AutoscanScanMode scanMode) const;

    /// \brief Gets an AutoscanDirectory (by objectID) from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) const override;

    /// \brief Get an AutoscanDirectory given by location on disk from the watch list.
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(const fs::path& location) const override;

    /// \brief returns an array of all autoscan directories
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanDirectories() const override;

    std::shared_ptr<AutoscanDirectory> findAutoscanDirectory(fs::path path) const override;
    bool isHiddenFile(const fs::directory_entry& dirEntry, bool isDirectory, const AutoScanSetting& settings) override;

    /// \brief Removes an AutoscanDirectrory (found by scanID) from the watch list.
    void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override;

    /// \brief Update autoscan parameters for an existing autoscan directory
    /// or add a new autoscan directory
    void setAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& dir) override;

    /// \brief handles the removal of a persistent autoscan directory
    void handlePeristentAutoscanRemove(const std::shared_ptr<AutoscanDirectory>& adir) override;

    /// \brief handles the recreation of a persistent autoscan directory
    void handlePersistentAutoscanRecreate(const std::shared_ptr<AutoscanDirectory>& adir) override;

    void rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, fs::path descPath = {}, bool cancellable = true) override;

    /// \brief instructs ContentManager to reload scripting environment
    void reloadLayout();

    /// \brief register executor
    ///
    /// When an external process is launched we will register the executor
    /// the content manager. This will ensure that we can kill all processes
    /// when we shutdown the server.
    ///
    /// \param exec the Executor object of the process
    void registerExecutor(const std::shared_ptr<Executor>& exec) override;

    /// \brief unregister process
    ///
    /// When the the process io handler receives a close on a stream that is
    /// currently being processed by an external process, it will kill it.
    /// The handler will then remove the executor from the list.
    void unregisterExecutor(const std::shared_ptr<Executor>& exec) override;

    void triggerPlayHook(const std::string& group, const std::shared_ptr<CdsObject>& obj) override;

    /// \brief parse a file containing metadata for object
    void parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const override;

    std::shared_ptr<Context> getContext() const override
    {
        return context;
    }
    std::shared_ptr<ScriptingRuntime> getScriptingRuntime() const override
    {
        return scriptingRuntime;
    }

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<UpdateManager> update_manager;
    std::shared_ptr<Web::SessionManager> session_manager;
    std::shared_ptr<ConverterManager> converterManager;
    std::shared_ptr<Context> context;
    std::shared_ptr<ImportService> importService;

    std::shared_ptr<Timer> timer;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<ScriptingRuntime> scriptingRuntime;
    std::shared_ptr<LastFm> last_fm;

    std::shared_ptr<AutoscanList> autoscanList;
#ifdef HAVE_INOTIFY
    std::unique_ptr<AutoscanInotify> inotify;
#endif

    std::vector<std::shared_ptr<Executor>> process_list;

    std::shared_ptr<CdsObject> addFileInternal(
        const fs::directory_entry& dirEnt,
        const fs::path& rootpath,
        AutoScanSetting& asSetting,
        bool lowPriority = false,
        unsigned int parentTaskID = 0,
        bool cancellable = true);
    std::shared_ptr<CdsObject> _addFile(
        const fs::directory_entry& dirEnt,
        fs::path rootPath,
        AutoScanSetting& asSetting,
        const std::shared_ptr<CMAddFileTask>& task = nullptr);

    std::shared_ptr<ImportService> getImportService(const std::shared_ptr<AutoscanDirectory>& adir);
    std::vector<int> _removeObject(
        const std::shared_ptr<AutoscanDirectory>& adir,
        const std::shared_ptr<CdsObject>& obj,
        const fs::path& path,
        bool rescanResource,
        bool all);
    void cleanupTasks(const fs::path& path);

    void scanDir(const std::shared_ptr<AutoscanDirectory>& dir, bool updateUI);
    void _rescanDirectory(
        const std::shared_ptr<AutoscanDirectory>& adir,
        int containerID,
        const std::shared_ptr<GenericTask>& task = nullptr);
    /* for recursive addition */
    void addRecursive(
        std::shared_ptr<AutoscanDirectory>& adir,
        const fs::directory_entry& subDir,
        const std::shared_ptr<CdsContainer>& parentContainer,
        bool followSymlinks,
        bool hidden,
        const std::shared_ptr<CMAddFileTask>& task);
    std::shared_ptr<CdsObject> createSingleItem(
        const fs::directory_entry& dirEnt,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootPath,
        bool followSymlinks,
        bool checkDatabase,
        bool processExisting,
        bool firstChild,
        const std::shared_ptr<AutoscanDirectory>& adir,
        const std::shared_ptr<CMAddFileTask>& task);
    bool updateAttachedResources(
        const std::shared_ptr<AutoscanDirectory>& adir,
        const std::shared_ptr<CdsObject>& obj,
        const fs::path& parentPath,
        bool all);
    static void invalidateAddTask(const std::shared_ptr<GenericTask>& t, const fs::path& path);

    void initLayout();
    void destroyLayout();

    template <typename T>
    std::shared_ptr<CdsObject> updateCdsObject(const std::shared_ptr<T>& item, const std::map<std::string, std::string>& parameters);

#ifdef ONLINE_SERVICES
    std::unique_ptr<OnlineServiceList> online_services;
#endif // ONLINE_SERVICES

    ImportMode importMode = ImportMode::MediaTomb;
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

    friend class CMAddFileTask;
    friend class CMRemoveObjectTask;
    friend class CMRescanDirectoryTask;
#ifdef ONLINE_SERVICES
    friend class CMFetchOnlineContentTask;
#endif
};

#endif // __CONTENT_MANAGER_H__

/*GRB*

    Gerbera - https://gerbera.io/

    content.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// \file content.h
/// \brief Interface for Content handling

#ifndef __CONTENT_H__
#define __CONTENT_H__

#include "util/grb_fs.h"
#include "util/timer.h"

#include <deque>

class AutoscanDirectory;
class AutoScanSetting;
class GenericTask;
class CdsContainer;
class CdsObject;
class Context;
class ScriptingRuntime;
enum class TaskOwner;

#ifdef ONLINE_SERVICES
class OnlineService;
#endif // ONLINE_SERVICES

enum class ImportMode {
    MediaTomb,
    Gerbera,
};

class Content : public Timer::Subscriber {
public:
    virtual ~Content() = default;
    virtual void run() = 0;
    virtual void shutdown() = 0;
    virtual std::shared_ptr<Context> getContext() const = 0;

    virtual std::shared_ptr<ScriptingRuntime> getScriptingRuntime() const = 0;
    /// \brief parse a file containing metadata for object
    virtual void parseMetafile(const std::shared_ptr<CdsObject>& obj, const fs::path& path) const = 0;

    virtual bool isHiddenFile(const fs::directory_entry& dirEntry, bool isDirectory, const AutoScanSetting& settings) = 0;

    /// \brief Ensures that a container given by it's location on disk is
    /// present in the database. If it does not exist it will be created, but
    /// it's content will not be added.
    ///
    /// \param path location of the container to handle
    /// \return objectID of the container given by path
    virtual int ensurePathExistence(const fs::path& path) const = 0;
    virtual void rescanDirectory(const std::shared_ptr<AutoscanDirectory>& adir, int objectId, fs::path descPath = {}, bool cancellable = true) = 0;
    virtual void handlePersistentAutoscanRecreate(const std::shared_ptr<AutoscanDirectory>& adir) = 0;
    virtual void triggerPlayHook(const std::string& group, const std::shared_ptr<CdsObject>& obj) = 0;

    /// \brief register executor
    /// When an external process is launched we will register the executor
    /// the content manager. This will ensure that we can kill all processes
    /// when we shutdown the server.
    /// \param exec the Executor object of the process
    virtual void registerExecutor(const std::shared_ptr<Executor>& exec) = 0;
    /// \brief unregister process
    /// When the the process io handler receives a close on a stream that is
    /// currently being processed by an external process, it will kill it.
    /// The handler will then remove the executor from the list.
    virtual void unregisterExecutor(const std::shared_ptr<Executor>& exec) = 0;

    /// \brief Returns the task that is currently being executed.
    virtual std::shared_ptr<GenericTask> getCurrentTask() const = 0;
    /// \brief Returns the list of all enqueued tasks, including the current or nullptr if no tasks are present.
    virtual std::deque<std::shared_ptr<GenericTask>> getTasklist() = 0;
    /// \brief Find a task identified by the task ID and invalidate it.
    virtual void invalidateTask(unsigned int taskID, TaskOwner taskOwner) = 0;

    /// \brief Get an AutoscanDirectory given by location on disk from the watch list.
    virtual std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(const fs::path& location) const = 0;
    /// \brief Gets an AutoscanDirectory (by objectID) from the watch list.
    virtual std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) const = 0;
    virtual std::shared_ptr<AutoscanDirectory> findAutoscanDirectory(fs::path path) const = 0;
    /// \brief handles the removal of a persistent autoscan directory
    virtual void handlePeristentAutoscanRemove(const std::shared_ptr<AutoscanDirectory>& adir) = 0;
    /// \brief returns an array of all autoscan directories
    virtual std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanDirectories() const = 0;
    /// \brief Update autoscan parameters for an existing autoscan directory
    /// or add a new autoscan directory
    virtual void setAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& dir) = 0;
    /// \brief Removes an AutoscanDirectrory (found by scanID) from the watch list.
    virtual void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) = 0;

    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    virtual std::shared_ptr<CdsContainer> addContainer(int parentID, const std::string& title, const std::string& upnpClass) = 0;
    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allowFifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    virtual void addVirtualItem(const std::shared_ptr<CdsObject>& obj, bool allowFifo = false) = 0;
    /// \brief Adds an object to the database.
    /// \param obj object to add
    /// \param firstChild indicate that this obj is the first child in parent container
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method
    virtual void addObject(const std::shared_ptr<CdsObject>& obj, bool firstChild) = 0;
    /// \brief Updates an object in the database.
    /// \param obj the object to update
    /// \param sendUpdates send updates to subscribed clients
    virtual void updateObject(const std::shared_ptr<CdsObject>& obj, bool sendUpdates = true) = 0;
    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    virtual std::shared_ptr<CdsObject> updateObject(int objectID, const std::map<std::string, std::string>& parameters) = 0;
    virtual std::vector<int> removeObject(const std::shared_ptr<AutoscanDirectory>& adir, const std::shared_ptr<CdsObject>& obj,
        const fs::path& path, bool rescanResource, bool async = true, bool all = false)
        = 0;

#ifdef ONLINE_SERVICES
    virtual void cleanupOnlineServiceObjects(const std::shared_ptr<OnlineService>& service) = 0;

#endif // ONLINE_SERVICES

    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param asSetting Settings for import
    /// \param lowPriority for immediate processing or in normal order
    /// \param cancellable can be canceled
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    virtual std::shared_ptr<CdsObject> addFile(const fs::directory_entry& dirEnt, AutoScanSetting& asSetting,
        bool lowPriority = false, bool cancellable = true)
        = 0;
    /// \brief Adds a file or directory to the database.
    /// \param dirEnt absolute path to the file
    /// \param rootpath absolute path to the container root
    /// \param asSetting Settings for import
    /// \param lowPriority for immediate processing or in normal order
    /// \param cancellable can be canceled
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    virtual std::shared_ptr<CdsObject> addFile(const fs::directory_entry& dirEnt, const fs::path& rootpath, AutoScanSetting& asSetting,
        bool lowPriority = false, bool cancellable = true)
        = 0;
    /// \brief Adds a virtual container chain specified by path.
    /// \param chain list of container objects to create
    /// \param refItem object to take artwork from
    /// \return ID of the last container in the chain.
    virtual std::pair<int, bool> addContainerTree(const std::vector<std::shared_ptr<CdsObject>>& chain, const std::shared_ptr<CdsObject>& refItem) = 0;
    // returns nullptr if file does not exist or is ignored due to configuration
    virtual std::shared_ptr<CdsObject> createObjectFromFile(const std::shared_ptr<AutoscanDirectory>& adir, const fs::directory_entry& dirEnt, bool followSymlinks, bool allowFifo = false) = 0;
};

#endif // __CONTENT_H__

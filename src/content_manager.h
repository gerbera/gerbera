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

#include <memory>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

#include "common.h"
#include "cds_objects.h"
#include "storage.h"
#include "dictionary.h"
#include "autoscan.h"
#include "timer.h"
#include "generic_task.h"

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
    #include "online_service.h"
#endif//ONLINE_SERVICES

    #include "executor.h"

class CMAddFileTask : public GenericTask
{
protected:
    zmm::String path;
    zmm::String rootpath;
    bool recursive;
    bool hidden;
public:
    CMAddFileTask(zmm::String path, zmm::String rootpath, bool recursive=false,
                  bool hidden=false, bool cancellable = true);
    zmm::String getPath();
    zmm::String getRootPath();
    virtual void run() override;
};

class CMRemoveObjectTask : public GenericTask
{
protected:
    int objectID;
    bool all;
public:
    CMRemoveObjectTask(int objectID, bool all);
    virtual void run() override;
};

class CMLoadAccountingTask : public GenericTask
{
public:
    CMLoadAccountingTask();
    virtual void run() override;
};

class CMRescanDirectoryTask : public GenericTask
{
protected: 
    int objectID;
    int scanID;
    ScanMode scanMode;
public:
    CMRescanDirectoryTask(int objectID, int scanID, ScanMode scanMode,
                          bool cancellable);
    virtual void run() override;
};

class CMAccounting : public zmm::Object
{
public:
    CMAccounting();
public:
    int totalFiles;
};

#ifdef ONLINE_SERVICES
class CMFetchOnlineContentTask : public GenericTask
{
protected:
    zmm::Ref<OnlineService> service;
    zmm::Ref<Layout> layout;
    bool unscheduled_refresh;

public:
    CMFetchOnlineContentTask(zmm::Ref<OnlineService> service, 
                             zmm::Ref<Layout> layout,
                             bool cancellable, bool unscheduled_refresh);
    virtual void run() override;
};
#endif

/*
class DirCacheEntry : public zmm::Object
{
public:
    DirCacheEntry();
public:
    int end;
    int id;
};

class DirCache : public zmm::Object
{
protected:
    zmm::Ref<zmm::StringBuffer> buffer;
    int size; // number of entries
    int capacity; // capacity of entries
    zmm::Ref<zmm::Array<DirCacheEntry> > entries;
public:
    DirCache();
    void push(zmm::String name);
    void pop();
    void setPath(zmm::String path);
    void clear();
    zmm::String getPath();
    int createContainers();
};
*/

class ContentManager : public Timer::Subscriber, public Singleton<ContentManager, std::recursive_mutex>
{
public:
    ContentManager();
    void init() override;
    zmm::String getName() override { return _("Content Manager"); }
    virtual ~ContentManager();
    void shutdown() override;

    virtual void timerNotify(zmm::Ref<Timer::Parameter> parameter) override;

    bool isBusy() { return working; }

    zmm::Ref<CMAccounting> getAccounting();

    /// \brief Returns the task that is currently being executed.
    zmm::Ref<GenericTask> getCurrentTask();

    /// \brief Returns the list of all enqueued tasks, including the current or nullptr if no tasks are present.
    zmm::Ref<zmm::Array<GenericTask> > getTasklist();

    /// \brief Find a task identified by the task ID and invalidate it.
    void invalidateTask(unsigned int taskID, task_owner_t taskOwner = ContentManagerTask);

    
    /* the functions below return true if the task has been enqueued */
    
    /* sync/async methods */
    void loadAccounting(bool async=true);

    /// \brief Adds a file or directory to the database.
    /// \param path absolute path to the file
    /// \param recursive recursive add (process subdirecotories)
    /// \param async queue task or perform a blocking call
    /// \param hidden true allows to import hidden files, false ignores them
    /// \param queue for immediate processing or in normal order
    /// \return object ID of the added file - only in blockign mode, when used in async mode this function will return INVALID_OBJECT_ID
    int addFile(zmm::String path, bool recursive=true, bool async=true, 
                bool hidden=false, bool lowPriority=false, 
                bool cancellable=true);

    int ensurePathExistence(zmm::String path);
    void removeObject(int objectID, bool async=true, bool all=false);
    void rescanDirectory(int objectID, int scanID, ScanMode scanMode,
                         zmm::String descPath = nullptr, bool cancellable = true);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    /// \param parameters key value pairs of fields to be updated
    void updateObject(int objectID, zmm::Ref<Dictionary> parameters);

    zmm::Ref<CdsObject> createObjectFromFile(zmm::String path, 
                                             bool magic=true, 
                                             bool allow_fifo=false);

#ifdef ONLINE_SERVICES
    /// \brief Creates a layout based from data that is obtained from an
    /// online service (like YouTube, SopCast, etc.)
    void fetchOnlineContent(service_type_t service, bool lowPriority=true, 
                            bool cancellable=true, 
                            bool unscheduled_refresh = false);

    void cleanupOnlineServiceObjects(zmm::Ref<OnlineService> service);

#endif//ONLINE_SERVICES

    /// \brief Adds a virtual item.
    /// \param obj item to add
    /// \param allow_fifo flag to indicate that it is ok to add a fifo,
    /// otherwise only regular files or directories are allowed.
    ///
    /// This function makes sure that the file is first added to
    /// PC-Directory, however without the scripting execution.
    /// It then adds the user defined virtual item to the database.
    void addVirtualItem(zmm::Ref<CdsObject> obj, bool allow_fifo=false);

    /// \brief Adds an object to the database.
    /// \param obj object to add
    ///
    /// parentID of the object must be set before this method.
    /// The ID of the object provided is ignored and generated by this method    
    void addObject(zmm::Ref<CdsObject> obj);

    /// \brief Adds a virtual container chain specified by path.
    /// \param container path separated by '/'. Slashes in container
    /// titles must be escaped.
    /// \param lastClass upnp:class of the last container in the chain, if nullptr
    /// then the default class will be taken
    /// \param lastRefID reference id of the last container in the chain,
    /// INVALID_OBJECT_ID indicates that the id will not be set. 
    /// \return ID of the last container in the chain.
    int addContainerChain(zmm::String chain, zmm::String lastClass = nullptr,
            int lastRefID = INVALID_OBJECT_ID, zmm::Ref<Dictionary> lastMetadata = nullptr);
    
    /// \brief Adds a virtual container specified by parentID and title
    /// \param parentID the id of the parent.
    /// \param title the title of the container.
    /// \param upnpClass the upnp class of the container.
    void addContainer(int parentID, zmm::String title, zmm::String upnpClass);
    
    /// \brief Updates an object in the database.
    /// \param obj the object to update
    void updateObject(zmm::Ref<CdsObject> obj, bool send_updates = true);

    /// \brief Updates an object in the database using the given parameters.
    /// \param objectID ID of the object to update
    ///
    /// Note: no actions should be performed on the object given as the parameter.
    /// Only the returned object should be processed. This method does not save
    /// the returned object in the database. To do so updateObject must be called
    zmm::Ref<CdsObject> convertObject(zmm::Ref<CdsObject> obj, int objectType);

    /// \brief Gets an AutocsanDirectrory from the watch list.
    zmm::Ref<AutoscanDirectory> getAutoscanDirectory(int scanID, ScanMode scanMode);

    /// \brief Get an AutoscanDirectory given by location on disk from the watch list.
    zmm::Ref<AutoscanDirectory> getAutoscanDirectory(zmm::String location);
    /// \brief Removes an AutoscanDirectrory (found by scanID) from the watch list.
    void removeAutoscanDirectory(int scanID, ScanMode scanMode);

    /// \brief Removes an AutoscanDirectrory (found by location) from the watch list.
    void removeAutoscanDirectory(zmm::String location);
    
    /// \brief Removes an AutoscanDirectory (by objectID) from the watch list.
    void removeAutoscanDirectory(int objectID);
 
    /// \brief Update autoscan parameters for an existing autoscan directory 
    /// or add a new autoscan directory
    void setAutoscanDirectory(zmm::Ref<AutoscanDirectory> dir);

    /// \brief handles the removal of a persistent autoscan directory
    void handlePeristentAutoscanRemove(int scanID, ScanMode scanMode);

    /// \brief handles the recreation of a persistent autoscan directory
    void handlePersistentAutoscanRecreate(int scanID, ScanMode scanMode);

    /// \brief returns an array of autoscan directories for the given scan mode
    zmm::Ref<zmm::Array<AutoscanDirectory> > getAutoscanDirectories(ScanMode scanMode);

    /// \brief returns an array of all autoscan directories 
    zmm::Ref<zmm::Array<AutoscanDirectory> > getAutoscanDirectories();


    /// \brief instructs ContentManager to reload scripting environment
    void reloadLayout();

    /// \brief register executor
    ///
    /// When an external process is launched we will register the executor
    /// the content manager. This will ensure that we can kill all processes
    /// when we shutdown the server.
    /// 
    /// \param exec the Executor object of the process
    void registerExecutor(zmm::Ref<Executor> exec);

    /// \brief unregister process
    /// 
    /// When the the process io handler receives a close on a stream that is
    /// currently being processed by an external process, it will kill it.
    /// The handler will then remove the executor from the list.
    void unregisterExecutor(zmm::Ref<Executor> exec);

#ifdef HAVE_MAGIC
    zmm::String getMimeTypeFromBuffer(const void *buffer, size_t length);
#endif


protected:
    void initLayout();
    void destroyLayout();

#ifdef HAVE_JS
    void initJS();
    void destroyJS();
#endif
    
    zmm::Ref<RExp> reMimetype;

    bool ignore_unknown_extensions;
    bool extension_map_case_sensitive;

    zmm::Ref<Dictionary> extension_mimetype_map;
    zmm::Ref<Dictionary> mimetype_upnpclass_map;
    zmm::Ref<Dictionary> mimetype_contenttype_map;

    zmm::Ref<AutoscanList> autoscan_timed;
#ifdef HAVE_INOTIFY
    AutoscanInotify inotify;
    zmm::Ref<AutoscanList> autoscan_inotify;
#endif
 
    zmm::Ref<zmm::Array<Executor> > process_list;

    void _loadAccounting();

    int addFileInternal(zmm::String path, zmm::String rootpath, 
                        bool recursive=true,
                        bool async=true, bool hidden=false,
                        bool lowPriority=false, 
                        unsigned int parentTaskID = 0,
                        bool cancellable = true);
    int _addFile(zmm::String path, zmm::String rootpath, bool recursive=false, bool hidden=false, zmm::Ref<GenericTask> task=nullptr);
    //void _addFile2(zmm::String path, bool recursive=0);
    void _removeObject(int objectID, bool all);
    
    void _rescanDirectory(int containerID, int scanID, ScanMode scanMode, ScanLevel scanLevel, zmm::Ref<GenericTask> task=nullptr);
    /* for recursive addition */
    void addRecursive(zmm::String path, bool hidden, zmm::Ref<GenericTask> task);
    //void addRecursive2(zmm::Ref<DirCache> dirCache, zmm::String filename, bool recursive);
    
    zmm::String extension2mimetype(zmm::String extension);
    zmm::String mimetype2upnpclass(zmm::String mimeType);

    void invalidateAddTask(zmm::Ref<GenericTask> t, zmm::String path);
    
    zmm::Ref<Layout> layout;

#ifdef ONLINE_SERVICES 
    zmm::Ref<OnlineServiceList> online_services;

    void fetchOnlineContentInternal(zmm::Ref<OnlineService> service,
                                    bool lowPriority=true,
                                    bool cancellable=true,
                                    unsigned int parentTaskID = 0,
                                    bool unscheduled_refresh = false);

#endif //ONLINE_SERVICES 

#ifdef HAVE_JS
    zmm::Ref<PlaylistParserScript> playlist_parser_script;
#endif

    bool layout_enabled;
    
    void setLastModifiedTime(time_t lm);
    
    inline void signal() { cond.notify_one(); }
    static void *staticThreadProc(void *arg);
    void threadProc();
    
    void addTask(zmm::Ref<GenericTask> task, bool lowPriority = false);
    
    zmm::Ref<CMAccounting> acct;
    
    pthread_t taskThread;
    std::condition_variable_any cond;
    
    bool working;
    
    bool shutdownFlag;
    
    zmm::Ref<zmm::ObjectQueue<GenericTask> > taskQueue1; // priority 1
    zmm::Ref<zmm::ObjectQueue<GenericTask> > taskQueue2; // priority 2
    zmm::Ref<GenericTask> currentTask;

    unsigned int taskID;

    friend void CMAddFileTask::run();
    friend void CMRemoveObjectTask::run();
    friend void CMRescanDirectoryTask::run();
#ifdef ONLINE_SERVICES
    friend void CMFetchOnlineContentTask::run();
#endif
    friend void CMLoadAccountingTask::run();
};

#endif // __CONTENT_MANAGER_H__

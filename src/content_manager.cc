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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "tools.h"
#include "rexp.h"
#include "content_manager.h"
#include "config_manager.h"
#include "update_manager.h"
#include "string_converter.h"
#include "metadata_handler.h"
#include "session_manager.h"
#include "timer.h"
#include "layout/fallback_layout.h"
#include "filesystem.h"

#ifdef HAVE_JS
    #include "layout/js_layout.h"
#endif

#ifdef EXTERNAL_TRANSCODING
    #include "process.h"
#endif

#ifdef YOUTUBE
    #include "youtube_service.h"
    #include <time.h>
#endif

#ifdef SOPCAST
    #include "sopcast_service.h"
#endif

#ifdef WEBORAMA
    #include "weborama_service.h"
#endif

#ifdef ATRAILERS
    #include "atrailers_service.h"
#endif

#ifdef ONLINE_SERVICES
    #include "task_processor.h"
#endif

#define DEFAULT_DIR_CACHE_CAPACITY  10
#define CM_INITIAL_QUEUE_SIZE       20

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}

struct magic_set *ms = NULL;
#endif

using namespace zmm;
using namespace mxml;

#define MIMETYPE_REGEXP "^([a-z0-9_-]+/[a-z0-9_-]+)"

SINGLETON_MUTEX(ContentManager, true);

static String get_filename(String path)
{
    if (path.charAt(path.length() - 1) == DIR_SEPARATOR) // cut off trailing slash
        path = path.substring(0, path.length() - 1);
    int pos = path.rindex(DIR_SEPARATOR);
    if (pos < 0)
        return path;
    else
        return path.substring(pos + 1);
}

ContentManager::ContentManager() : TimerSubscriberSingleton<ContentManager>()
{
    int i;
    cond = Ref<Cond>(new Cond(mutex));
    ignore_unknown_extensions = false;
    extension_map_case_sensitive = false;

    taskID = 1;
    working = false;
    shutdownFlag = false;
    layout_enabled = false;
    
    acct = Ref<CMAccounting>(new CMAccounting());
    taskQueue1 = Ref<ObjectQueue<GenericTask> >(new ObjectQueue<GenericTask>(CM_INITIAL_QUEUE_SIZE));
    taskQueue2 = Ref<ObjectQueue<GenericTask> >(new ObjectQueue<GenericTask>(CM_INITIAL_QUEUE_SIZE));    
    
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> tmpEl;  
    
    // loading extension - mimetype map  
    // we can always be sure to get a valid element because everything was prepared by the config manager
    extension_mimetype_map = 
        cm->getDictionaryOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    ignore_unknown_extensions = cm->getBoolOption(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    if (ignore_unknown_extensions && (extension_mimetype_map->size() == 0))
    {
        log_warning("Ignore unknown extensions set, but no mappings specified\n");
        log_warning("Please review your configuration!\n");
        ignore_unknown_extensions = false;
    }
  
    extension_map_case_sensitive = cm->getBoolOption(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    mimetype_upnpclass_map = 
       cm->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
  
    mimetype_contenttype_map = 
      cm->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    Ref<AutoscanList> config_timed_list = 
        cm->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);

    for (i = 0; i < config_timed_list->size(); i++)
    {
        Ref<AutoscanDirectory> dir = config_timed_list->get(i);
        if (dir != nil)
        {
            String path = dir->getLocation();
            if (check_path(path, true))
            {
                dir->setObjectID(ensurePathExistence(path));
            }
        }
    }

    Ref<Storage> storage = Storage::getInstance();
    storage->updateAutoscanPersistentList(TimedScanMode, config_timed_list);
    autoscan_timed = storage->getAutoscanList(TimedScanMode);

#ifdef HAVE_INOTIFY
    if (cm->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        Ref<AutoscanList> config_inotify_list = 
            cm->getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);

        for (i = 0; i < config_inotify_list->size(); i++)
        {
            Ref<AutoscanDirectory> dir = config_inotify_list->get(i);
            if (dir != nil)
            {
                String path = dir->getLocation();
                if (check_path(path, true))
                {
                    dir->setObjectID(ensurePathExistence(path));
                }
            }
        }

        storage->updateAutoscanPersistentList(InotifyScanMode, 
                                              config_inotify_list);
        autoscan_inotify = storage->getAutoscanList(InotifyScanMode);
    }
    else
        // make an empty list so we do not have to do extra checks on shutdown 
        autoscan_inotify = Ref<AutoscanList>(new AutoscanList());
#endif
    /* init filemagic */
#ifdef HAVE_MAGIC
    if (! ignore_unknown_extensions)
    {
        ms = magic_open(MAGIC_MIME);
        if (ms == NULL)
        {
            log_error("magic_open failed\n");
        }
        else
        {
            String magicFile = cm->getOption(CFG_IMPORT_MAGIC_FILE);
            if (! string_ok(magicFile))
                magicFile = nil;
            if (magic_load(ms, (magicFile == nil) ? NULL : magicFile.c_str()) == -1)
            {
                log_warning("magic_load: %s\n", magic_error(ms));
                magic_close(ms);
                ms = NULL;
            }
        }
    }
#endif // HAVE_MAGIC

    String layout_type = 
                       cm->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    if ((layout_type == "builtin") || (layout_type == "js"))
        layout_enabled = true;

#ifdef ONLINE_SERVICES
    online_services = Ref<OnlineServiceList>(new OnlineServiceList());
#ifdef YOUTUBE
    if (cm->getBoolOption(CFG_ONLINE_CONTENT_YOUTUBE_ENABLED))
    {
        try 
        {
            Ref<OnlineService> yt((OnlineService *)new YouTubeService());

            i = cm->getIntOption(CFG_ONLINE_CONTENT_YOUTUBE_REFRESH);
            yt->setRefreshInterval(i);

            i = cm->getIntOption(CFG_ONLINE_CONTENT_YOUTUBE_PURGE_AFTER);
            yt->setItemPurgeInterval(i);

            if (cm->getBoolOption(CFG_ONLINE_CONTENT_YOUTUBE_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<TimerParameter> yt_param(new TimerParameter(TimerParameter::IDOnlineContent, OS_YouTube));
            yt->setTimerParameter(RefCast(yt_param, Object));
            online_services->registerService(yt);
            
            if (i > 0)
            {
                Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), i, yt->getTimerParameter(), true);
            }
        }
        catch (Exception ex)
        {
            log_error("Could not setup YouTube: %s\n", 
                    ex.getMessage().c_str());
        }
    }
    // if YT is disabled in the configuration it is still possible that the DB
    // is populated with YT objects, so we need to allow playing them
    cached_urls = Ref<ReentrantArray<CachedURL> >
        (new ReentrantArray<CachedURL>(MAX_CACHED_URLS));
    urlcache_mutex = Ref<Mutex>(new Mutex(false));

#endif //YOUTUBE

#ifdef SOPCAST
    if (cm->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_ENABLED))
    {
        try
        {
            Ref<OnlineService> sc((OnlineService *)new SopCastService());

            i = cm->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);
            sc->setRefreshInterval(i);

            i = cm->getIntOption(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);
            sc->setItemPurgeInterval(i);

            if (cm->getBoolOption(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<TimerParameter> sc_param(new TimerParameter(TimerParameter::IDOnlineContent, OS_SopCast));
            sc->setTimerParameter(RefCast(sc_param, Object));
            online_services->registerService(sc);
            if (i > 0)
            {
                Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), i, sc->getTimerParameter(), true);
            }
        }
        catch (Exception ex)
        {
            log_error("Could not setup SopCast: %s\n",
                    ex.getMessage().c_str());
        }
    }
#endif //SOPCAST

#ifdef WEBORAMA
    if (cm->getBoolOption(CFG_ONLINE_CONTENT_WEBORAMA_ENABLED))
    {
        try
        {
            Ref<OnlineService> wb((OnlineService *)new WeboramaService());
            i = cm->getIntOption(CFG_ONLINE_CONTENT_WEBORAMA_REFRESH);
            wb->setRefreshInterval(i);

            i = cm->getIntOption(CFG_ONLINE_CONTENT_WEBORAMA_PURGE_AFTER);
            wb->setItemPurgeInterval(i);

            if (cm->getBoolOption(CFG_ONLINE_CONTENT_WEBORAMA_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<TimerParameter> wb_param(new TimerParameter(TimerParameter::IDOnlineContent, OS_Weborama));
            wb->setTimerParameter(RefCast(wb_param, Object));
            online_services->registerService(wb);
            if (i > 0)
            {
                Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), i, wb->getTimerParameter(), true);
            }
        }
        catch (Exception ex)
        {
            log_error("Could not setup Weborama: %s\n",
                    ex.getMessage().c_str());
        }
    }
#endif //WEBORAMA

#ifdef ATRAILERS
    if (cm->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED))
    {
        try
        {
            Ref<OnlineService> at((OnlineService *)new ATrailersService());
            i = cm->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
            at->setRefreshInterval(i);

            i = cm->getIntOption(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
            at->setItemPurgeInterval(i);
            if (cm->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START))
                i = CFG_DEFAULT_UPDATE_AT_START;

            Ref<TimerParameter> at_param(new TimerParameter(TimerParameter::IDOnlineContent, OS_ATrailers));
            at->setTimerParameter(RefCast(at_param, Object));
            online_services->registerService(at);
            if (i > 0)
            {
                Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), i, at->getTimerParameter(), true);
            }
        }
        catch (Exception ex)
        {
            log_error("Could not setup Apple Trailers: %s\n",
                    ex.getMessage().c_str());
        }
    }
#endif//ATRAILERS

#endif //ONLINE_SERVICES
}

ContentManager::~ContentManager()
{
    log_debug("ContentManager destroyed\n");
}

void ContentManager::init()
{   
    int ret;

    reMimetype = Ref<RExp>(new RExp());
    reMimetype->compile(_(MIMETYPE_REGEXP));
    
    /*
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        throw _Exception(_("Could not initialize attribute"));
    }
   
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    */
    
    ret = pthread_create(
        &taskThread,
        NULL, //&attr, // attr
        ContentManager::staticThreadProc,
        this
    );
    if (ret != 0)
    {
        throw _Exception(_("Could not start task thread"));
    }
    
    //pthread_attr_destroy(&attr);
    
    //loadAccounting(false);
    
    autoscan_timed->notifyAll(AS_TIMER_SUBSCRIBER_SINGLETON(this));

#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        inotify = Ref<AutoscanInotify>(new AutoscanInotify());
        /// \todo change this (we need a new autoscan architecture)
        for (int i = 0; i < autoscan_inotify->size(); i++)
        {
            Ref<AutoscanDirectory> dir = autoscan_inotify->get(i);
            if (dir != nil)
                inotify->monitor(dir);
        }
    }
#endif

#ifdef EXTERNAL_TRANSCODING
    process_list = Ref<Array<Executor> >(new Array<Executor>());
#endif
}

#if defined(EXTERNAL_TRANSCODING) || defined(SOPCAST)
void ContentManager::registerExecutor(Ref<Executor> exec)
{
    AUTOLOCK(mutex);
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

    AUTOLOCK(mutex);
    for (int i = 0; i < process_list->size(); i++)
    {
        if (process_list->get(i) == exec)
            process_list->remove(i);
    }
}
#endif

void ContentManager::timerNotify(Ref<Object> parameter)
{
    if (parameter == nil)
        return;
    
    Ref<TimerParameter> tp = RefCast(parameter, TimerParameter);
    if (tp->whoami() == TimerParameter::IDAutoscan)
    {
        Ref<AutoscanDirectory> dir = autoscan_timed->get(tp->getID());
        if (dir == nil)
            return;

        int objectID = dir->getObjectID();
        rescanDirectory(objectID, dir->getScanID(), dir->getScanMode());
    }
#ifdef ONLINE_SERVICES
    else if (tp->whoami() == TimerParameter::IDOnlineContent)
    {
        fetchOnlineContent((service_type_t)(tp->getID()));
    }
#ifdef YOUTUBE
    else if (tp->whoami() == TimerParameter::IDURLCache)
    {
        checkCachedURLs();
    }
#endif
#endif//ONLINE_SERVICES

}

void ContentManager::shutdown()
{
#ifdef HAVE_INOTIFY
    inotify = nil;
#endif
    log_debug("start\n");
    AUTOLOCK(mutex);
    log_debug("updating last_modified data for autoscan in database...\n");
    autoscan_timed->updateLMinDB();

#ifdef HAVE_INOTIFY
    for (int i = 0; i < autoscan_inotify->size(); i++)
    {
        Ref<AutoscanDirectory> dir = autoscan_inotify->get(i);
        if (dir != nil)
        {
            try
            {
                dir->resetLMT();
                dir->setCurrentLMT(check_path_ex(dir->getLocation(), true));
                dir->updateLMT();
            }
            catch (Exception ex)
            {
                continue;
            }
        }
    }
    
    autoscan_inotify->updateLMinDB();
#endif

    shutdownFlag = true;

#ifdef EXTERNAL_TRANSCODING 
    for (int i = 0; i < process_list->size(); i++)
    {
        Ref<Executor> exec = process_list->get(i);
        if (exec != nil)
            exec->kill();
    }
#endif

    log_debug("signalling...\n");
    signal();
    AUTOUNLOCK();
    log_debug("waiting for thread...\n");

    if (taskThread)
        pthread_join(taskThread, NULL);
    taskThread = 0;

#ifdef HAVE_MAGIC
    if (ms)
    {
        magic_close(ms);
        ms = NULL;
    }
#endif
    log_debug("end\n");
}

Ref<CMAccounting> ContentManager::getAccounting()
{
    return acct;
}
Ref<GenericTask> ContentManager::getCurrentTask()
{
    Ref<GenericTask> task;
    AUTOLOCK(mutex);
    task = currentTask;
    return task;
}

Ref<Array<GenericTask> > ContentManager::getTasklist()
{
    int i;

    AUTOLOCK(mutex);
   

    Ref<Array<GenericTask> > taskList = nil;
#ifdef ONLINE_SERVICES
    taskList = TaskProcessor::getInstance()->getTasklist();
#endif
    Ref<GenericTask> t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if ((t == nil) && (taskList == nil))
        return nil;

    if (taskList == nil)
        taskList = Ref<Array<GenericTask> >(new Array<GenericTask>());

    if (t != nil)
        taskList->append(t); 

    int qsize = taskQueue1->size();

    for (i = 0; i < qsize; i++)
    {
        Ref<GenericTask> t = taskQueue1->get(i);
        if (t->isValid())
            taskList->append(t);
    }

    qsize = taskQueue2->size();
    for (i = 0; i < qsize; i++)
    {
        Ref<GenericTask> t = taskQueue2->get(i);
        if (t->isValid())
            taskList = Ref<Array<GenericTask> >(new Array<GenericTask>()); 
    }

    return taskList;
}


void ContentManager::_loadAccounting()
{
    Ref<Storage> storage = Storage::getInstance();
    acct->totalFiles = storage->getTotalFiles();
}

void ContentManager::addVirtualItem(Ref<CdsObject> obj, bool allow_fifo)
{
    obj->validate();
    String path = obj->getLocation();
    check_path_ex(path, false, false);
    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> pcdir = storage->findObjectByPath(path);
    if (pcdir == nil)
    {
        pcdir = createObjectFromFile(path, true, allow_fifo);
        if (pcdir == nil)
        {
            throw _Exception(_("Could not add ") + path);
        }
        if (IS_CDS_ITEM(pcdir->getObjectType()))
        {
            this->addObject(pcdir);
            obj->setRefID(pcdir->getID());
        }
    }

    addObject(obj);

}

int ContentManager::_addFile(String path, String rootpath, bool recursive, bool hidden, Ref<GenericTask> task)
{
    if (hidden == false)
    {
        String filename = get_filename(path);
        if (string_ok(filename) && filename.charAt(0) == '.')
            return INVALID_OBJECT_ID;
    }
    
    // never add the server configuration file
    if (ConfigManager::getInstance()->getConfigFilename() == path)
        return INVALID_OBJECT_ID;

    if (layout_enabled)
        initLayout();

#ifdef HAVE_JS
    initJS();
#endif
    
    Ref<Storage> storage = Storage::getInstance();
    
    Ref<UpdateManager> um = UpdateManager::getInstance();
    //Ref<StringConverter> f2i = StringConverter::f2i();
 
    Ref<CdsObject> obj = storage->findObjectByPath(path);
    if (obj == nil)
    {
        obj = createObjectFromFile(path);
        if (obj == nil) // object ignored
            return INVALID_OBJECT_ID;
        if (IS_CDS_ITEM(obj->getObjectType()))
        {
            addObject(obj);
            if (layout != nil)
            {
                try
                {
                    if (!string_ok(rootpath) && (task != nil))
                        rootpath = RefCast(task, CMAddFileTask)->getRootPath();

                    layout->processCdsObject(obj, rootpath);
                    
                    String mimetype = RefCast(obj, CdsItem)->getMimeType();
                    String content_type = mimetype_contenttype_map->get(mimetype);
#ifdef HAVE_JS
                    if ((playlist_parser_script != nil) &&
                        (content_type == CONTENT_TYPE_PLAYLIST))
                            playlist_parser_script->processPlaylistObject(obj, task);
#ifdef HAVE_LIBDVDNAV
                    if ((dvd_import_script != nil) &&
                        (content_type == CONTENT_TYPE_DVD))
                           dvd_import_script->processDVDObject(obj);
#else
                    if (content_type == CONTENT_TYPE_DVD)
                        log_warning("DVD Image %s will not be parsed: MediaTomb was compiled without libdvdnav  support!\n", obj->getLocation().c_str());
#endif // DVD
#else
                    if (content_type == CONTENT_TYPE_PLAYLIST)
                        log_warning("Playlist %s will not be parsed: MediaTomb was compiled without JS support!\n", obj->getLocation().c_str());
#endif // JS
                }
                catch (Exception e)
                {
                    throw e;
                }
            }
        }
    }
        
    if (recursive && IS_CDS_CONTAINER(obj->getObjectType()))
    {
        addRecursive(path, hidden, task);
    }
    return obj->getID();
}

void ContentManager::_removeObject(int objectID, bool all)
{
    if (objectID == CDS_ID_ROOT)
        throw _Exception(_("cannot remove root container"));
    if (objectID == CDS_ID_FS_ROOT)
        throw _Exception(_("cannot remove PC-Directory container"));
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw _Exception(_("tried to remove illegal object id"));
    
    Ref<Storage> storage = Storage::getInstance();
    
    Ref<Storage::ChangedContainers> changedContainers = storage->removeObject(objectID, all);
    
    if (changedContainers != nil)
    {
        SessionManager::getInstance()->containerChangedUI(changedContainers->ui);
        UpdateManager::getInstance()->containersChanged(changedContainers->upnp);
    }
    
    // reload accounting
    //loadAccounting();
}

int ContentManager::ensurePathExistence(zmm::String path)
{
    int updateID;
    int containerID = Storage::getInstance()->ensurePathExistence(path, &updateID);
    if (updateID != INVALID_OBJECT_ID)
    {
        UpdateManager::getInstance()->containerChanged(updateID);
        SessionManager::getInstance()->containerChangedUI(updateID);
    }
    return containerID;
}

void ContentManager::_rescanDirectory(int containerID, int scanID, scan_mode_t scanMode, scan_level_t scanLevel, Ref<GenericTask> task)
{
    log_debug("start\n");
    int ret;
    struct dirent *dent;
    struct stat statbuf;
    String location;
    String path;
    Ref<CdsObject> obj;

    if (scanID == INVALID_SCAN_ID)
        return; 

    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    if (adir == nil)
        throw _Exception(_("ID valid but nil returned? this should never happen"));

    Ref<Storage> storage = Storage::getInstance();
   
    if (containerID != INVALID_OBJECT_ID)
    {
        try
        {
            obj = storage->loadObject(containerID);
            if (!IS_CDS_CONTAINER(obj->getObjectType()))
            {
                throw _Exception(_("Not a container"));
            }
            if (containerID == CDS_ID_FS_ROOT)
                location = _(FS_ROOT_DIRECTORY);
            else
                location = obj->getLocation();
        }
        catch (Exception e)
        {
            if (adir->persistent())
            {
                containerID = INVALID_OBJECT_ID;
            }
            else
            {
                adir->setTaskCount(-1);
                removeAutoscanDirectory(scanID, scanMode);
                storage->removeAutoscanDirectory(adir->getStorageID());
                return;
            }
        }
    }

    if (containerID == INVALID_OBJECT_ID)
    {
        if (!check_path(adir->getLocation(), true))
        {
            adir->setObjectID(INVALID_OBJECT_ID);
            storage->updateAutoscanDirectory(adir);
            if (adir->persistent())
            {
                return;
            }
            else
            {
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

    if (!string_ok(location))
    {
        log_error("Container with ID %d has no location information\n", containerID);
        return;
        //        continue;
        //throw _Exception(_("Container has no location information!\n"));
    }

    DIR *dir = opendir(location.c_str());
    if (!dir)
    {
        log_warning("Could not open %s: %s\n", location.c_str(), strerror(errno));
        if (adir->persistent())
        {
            removeObject(containerID, false);
            adir->setObjectID(INVALID_OBJECT_ID);
            storage->updateAutoscanDirectory(adir);
            return;
        }
        else
        {
            adir->setTaskCount(-1);
            removeObject(containerID, false);
            removeAutoscanDirectory(scanID, scanMode);
            storage->removeAutoscanDirectory(adir->getStorageID());
            return;
        }
    }

    // request only items if non-recursive scan is wanted
    Ref<DBRHash<int> > list = storage->getObjects(containerID, !adir->getRecursive());

    unsigned int thisTaskID;
    if (task != nil)
    {
        thisTaskID = task->getID();
    }
    else
        thisTaskID = 0;

    while (((dent = readdir(dir)) != NULL) && (!shutdownFlag) && (task == nil || ((task != nil) && task->isValid())))
    {
        char *name = dent->d_name;
        if (name[0] == '.')
        {
            if (name[1] == 0)
            {
                continue;
            }
            else if (name[1] == '.' && name[2] == 0)
            {
                continue;
            }
            else if (!adir->getHidden())
            {
                continue;    
            }
        }

        path = location + DIR_SEPARATOR + name; 
        ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        {
            log_error("Failed to stat %s, %s\n", path.c_str(), mt_strerror(errno).c_str());
            continue;
        }

        // it is possible that someone hits remove while the container is being scanned
        // in this case we will invalidate the autoscan entry
        if (adir->getScanID() == INVALID_SCAN_ID)
        {
            closedir(dir);
            return;
        }

        if (S_ISREG(statbuf.st_mode))
        {
            int objectID = storage->findObjectIDByPath(String(path));
            if (objectID > 0)
            {
                if (list != nil)
                    list->remove(objectID);

                if (scanLevel == FullScanLevel)
                {
                    // check modification time and update file if chagned
                    if (last_modified_current_max < statbuf.st_mtime)
                    {
                        // readd object - we have to do this in order to trigger
                        // layout
                        removeObject(objectID, false);
                        addFileInternal(path, location, false, false, adir->getHidden());
                        // update time variable
                        last_modified_current_max = statbuf.st_mtime;
                    }
                }
                else if (scanLevel == BasicScanLevel)
                    continue;
                else
                    throw _Exception(_("Unsupported scan level!"));

            }
            else
            {
                // add file, not recursive, not async
                // make sure not to add the current config.xml
                if (ConfigManager::getInstance()->getConfigFilename() != path)
                {
                    addFileInternal(path, location, false, false, adir->getHidden());
                    if (last_modified_current_max < statbuf.st_mtime)
                        last_modified_current_max = statbuf.st_mtime;
                }
            }
        }
        else if (S_ISDIR(statbuf.st_mode) && (adir->getRecursive()))
        {
            int objectID = storage->findObjectIDByPath(path + DIR_SEPARATOR);
            if (objectID > 0)
            {
                if (list != nil)
                    list->remove(objectID);
                // add a task to rescan the directory that was found
                rescanDirectory(objectID, scanID, scanMode, path + DIR_SEPARATOR, task->isCancellable());
            }
            else
            {
                // we have to make sure that we will never add a path to the task list
                // if it is going to be removed by a pending remove task.
                // this lock will make sure that remove is not in the process of invalidating
                // the AutocsanDirectories in the autoscan_timed list at the time when we
                // are checking for validity.
                AUTOLOCK(mutex);
                
                // it is possible that someone hits remove while the container is being scanned
                // in this case we will invalidate the autoscan entry
                if (adir->getScanID() == INVALID_SCAN_ID)
                {
                    closedir(dir);
                    return;
                }
                
                // add directory, recursive, async, hidden flag, low priority
                addFileInternal(path, location, true, true, adir->getHidden(), true, thisTaskID, task->isCancellable());
            }
        }
    } // while
    
    closedir(dir);

    if ((shutdownFlag) || ((task != nil) && !task->isValid()))
        return;

    if (list != nil && list->size() > 0)
    {
        Ref<Storage::ChangedContainers> changedContainers = storage->removeObjects(list);
        if (changedContainers != nil)
        {
            SessionManager::getInstance()->containerChangedUI(changedContainers->ui);
            UpdateManager::getInstance()->containersChanged(changedContainers->upnp);
        }
    }


    adir->setCurrentLMT(last_modified_current_max);
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(String path, bool hidden, Ref<GenericTask> task)
{
    if (hidden == false)
    {
        log_debug("Checking path %s\n", path.c_str());
        if (path.charAt(0) == '.')
            return;
    }

    Ref<StringConverter> f2i = StringConverter::f2i();

    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<Storage> storage = Storage::getInstance();
    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        throw _Exception(_("could not list directory ")+
                        path + " : " + strerror(errno));
    }
    int parentID = storage->findObjectIDByPath(path + DIR_SEPARATOR);
    struct dirent *dent;
    // abort loop if either:
    // no valid directory returned, server is about to shutdown, the task is there and was invalidated
    if (task != nil)
    {
        log_debug("IS TASK VALID? [%d], taskoath: [%s]\n", task->isValid(), path.c_str());
    }
    while (((dent = readdir(dir)) != NULL) && (!shutdownFlag) && (task == nil || ((task != nil) && task->isValid())))
    {
        char *name = dent->d_name;
        if (name[0] == '.')
        {
            if (name[1] == 0)
            {
                continue;
            }
            else if (name[1] == '.' && name[2] == 0)
            {
                continue;
            }
            else if (hidden == false)
                continue;
        }
        String newPath = path + DIR_SEPARATOR + name;

        if (ConfigManager::getInstance()->getConfigFilename() == newPath)
            continue;

        try
        {
            Ref<CdsObject> obj = nil;
            if (parentID > 0)
                obj = storage->findObjectByPath(String(newPath));
            if (obj == nil) // create object
            {
                obj = createObjectFromFile(newPath);
                
                if (obj == nil) // object ignored
                {
                    log_warning("file ignored: %s\n", newPath.c_str());
                }
                else
                {
                    //obj->setParentID(parentID);
                    if (IS_CDS_ITEM(obj->getObjectType()))
                    {
                        addObject(obj);
                        parentID = obj->getParentID();
                    }
                }
            }
            if (obj != nil)
            {
//#ifdef HAVE_JS
                if (IS_CDS_ITEM(obj->getObjectType()))
                {
                    if (layout != nil)
                    {
                        try
                        {
                            String rootpath = nil;
                            if (task != nil)
                                rootpath = RefCast(task, CMAddFileTask)->getRootPath();
                            layout->processCdsObject(obj, rootpath);
#ifdef HAVE_JS
                            Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
                            String mimetype = RefCast(obj, CdsItem)->getMimeType();
                            String content_type = mappings->get(mimetype);
                           
                            if ((playlist_parser_script != nil) &&
                                (content_type == CONTENT_TYPE_PLAYLIST))
                                playlist_parser_script->processPlaylistObject(obj, task);
#ifdef HAVE_LIBDVDNAV
                            if ((dvd_import_script != nil) &&
                                (content_type == CONTENT_TYPE_DVD))
                                    dvd_import_script->processDVDObject(obj);
#endif // DVD

#endif // JS
                        }
                        catch (Exception e)
                        {
                            throw e;
                        }
                    }
                    
                    /// \todo Why was this statement here??? - It seems to be unnecessary
                    //obj = createObjectFromFile(newPath);
                }
//#endif
                if (IS_CDS_CONTAINER(obj->getObjectType()))
                {
                    addRecursive(newPath, hidden, task);
                }
            }
        }
        catch(Exception e)
        {
            log_warning("skipping %s : %s\n", newPath.c_str(), e.getMessage().c_str());
        }
    }
    closedir(dir);
}

void ContentManager::updateObject(int objectID, Ref<Dictionary> parameters)
{
    String title = parameters->get(_("title"));
    String upnp_class = parameters->get(_("class"));
    String autoscan = parameters->get(_("autoscan"));
    String mimetype = parameters->get(_("mime-type"));
    String description = parameters->get(_("description"));
    String location = parameters->get(_("location"));
    String protocol = parameters->get(_("protocol"));

    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<SessionManager> sm = SessionManager::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);
    int objectType = obj->getObjectType();

    /// \todo if we have an active item, does it mean we first go through IS_ITEM and then thorugh IS_ACTIVE item? ask Gena
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);
        if (string_ok(location)) clone->setLocation(location);

        Ref<CdsItem> cloned_item = RefCast(clone, CdsItem);
 
        if (string_ok(mimetype) && (string_ok(protocol)))
        {
            cloned_item->setMimeType(mimetype);
            Ref<CdsResource> resource = cloned_item->getResource(0);
            resource->addAttribute(_("protocolInfo"), renderProtocolInfo(mimetype, protocol));
        }
        else if (!string_ok(mimetype) && (string_ok(protocol)))
        {
            Ref<CdsResource> resource = cloned_item->getResource(0);
            resource->addAttribute(_("protocolInfo"), renderProtocolInfo(cloned_item->getMimeType(), protocol));
        }
        else if (string_ok(mimetype) && (!string_ok(protocol)))
        {
            cloned_item->setMimeType(mimetype);
            Ref<CdsResource> resource = cloned_item->getResource(0);
            Ref<Array<StringBase> > parts = split_string(resource->getAttribute(_("protocolInfo")), ':');
            protocol = parts->get(0);
            resource->addAttribute(_("protocolInfo"), renderProtocolInfo(mimetype, protocol));
        }

        if (string_ok(description)) 
        {
            cloned_item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
                                     description);
        }
        else
        {
            cloned_item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
        }


        log_debug("updateObject: checking equality of item %s\n", item->getTitle().c_str());
        if (!item->equals(clone, true))
        {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            um->containerChanged(containerChanged);
            sm->containerChangedUI(containerChanged);
            log_debug("updateObject: calling containerChanged on item %s\n", item->getTitle().c_str());
            um->containerChanged(item->getParentID());
        }
    }
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        String action = parameters->get(_("action"));
        String state = parameters->get(_("state"));

        Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);

        Ref<CdsActiveItem> cloned_item = RefCast(clone, CdsActiveItem);

        // state and description can be an empty strings - if you want to clear it
        if (string_ok(description)) 
        {
            cloned_item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
                                     description);
        }
        else
        {
            cloned_item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
        }

        if (state != nil) cloned_item->setState(state);

        if (string_ok(mimetype)) cloned_item->setMimeType(mimetype);
        if (string_ok(action)) cloned_item->setAction(action);

        if (!item->equals(clone, true))
        {
            cloned_item->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            um->containerChanged(containerChanged);
            sm->containerChangedUI(containerChanged);
            um->containerChanged(item->getParentID());
        }
    }
    else if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        cont->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);

        if (!cont->equals(clone, true))
        {
            clone->validate();
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            um->containerChanged(containerChanged);
            sm->containerChangedUI(containerChanged);
            um->containerChanged(cont->getParentID());
            sm->containerChangedUI(cont->getParentID());
        }
    }

}

void ContentManager::addObject(zmm::Ref<CdsObject> obj)
{
    obj->validate();
    int parent_id;
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<SessionManager> sm = SessionManager::getInstance();
    int containerChanged = INVALID_OBJECT_ID;
    log_debug("Adding: parent ID is %d\n", obj->getParentID());
    if (!IS_CDS_ITEM_EXTERNAL_URL(obj->getObjectType()))
    {
        obj->setLocation(obj->getLocation().reduce(DIR_SEPARATOR));
    }
    storage->addObject(obj, &containerChanged);
    log_debug("After adding: parent ID is %d\n", obj->getParentID());
    
    um->containerChanged(containerChanged);
    sm->containerChangedUI(containerChanged);
    
    parent_id = obj->getParentID();
    if ((parent_id != -1) && (storage->getChildCount(parent_id) == 1))
    {
        Ref<CdsObject> parent; //(new CdsObject());
        parent = storage->loadObject(parent_id);
        log_debug("Will update ID %d\n", parent->getParentID());
        um->containerChanged(parent->getParentID());
    }
    
    um->containerChanged(obj->getParentID());
    if (IS_CDS_CONTAINER(obj->getObjectType()))
        sm->containerChangedUI(obj->getParentID());
    
    if (! obj->isVirtual() && IS_CDS_ITEM(obj->getObjectType()))
        ContentManager::getInstance()->getAccounting()->totalFiles++;
}


void ContentManager::addContainer(int parentID, String title, String upnpClass)
{
    Ref<Storage> storage = Storage::getInstance();
    addContainerChain(storage->buildContainerPath(parentID, escape(title, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR)), upnpClass);
}


int ContentManager::addContainerChain(String chain, String lastClass, int lastRefID)
{
    Ref<Storage> storage = Storage::getInstance();
    int updateID = INVALID_OBJECT_ID;
    int containerID;

    if (!string_ok(chain))
        throw _Exception(_("addContainerChain() called with empty chain parameter"));
    
    log_debug("received chain: %s\n", chain.c_str());
    storage->addContainerChain(chain, lastClass, lastRefID,
                              &containerID, &updateID);
    // if (updateID != INVALID_OBJECT_ID)
    // an invalid updateID is checked by containerChanged()
    UpdateManager::getInstance()->containerChanged(updateID);
    SessionManager::getInstance()->containerChangedUI(updateID);

    return containerID;
}

void ContentManager::updateObject(Ref<CdsObject> obj, bool send_updates)
{
    obj->validate();
    Ref<Storage> storage = Storage::getInstance();
   
    int containerChanged = INVALID_OBJECT_ID;
    storage->updateObject(obj, &containerChanged);

    if (send_updates)
    {
        Ref<UpdateManager> um = UpdateManager::getInstance();
        Ref<SessionManager> sm = SessionManager::getInstance();

        um->containerChanged(containerChanged);
        sm->containerChangedUI(containerChanged);

        um->containerChanged(obj->getParentID());
        if (IS_CDS_CONTAINER(obj->getObjectType()))
            sm->containerChangedUI(obj->getParentID());
    }
}

Ref<CdsObject> ContentManager::convertObject(Ref<CdsObject> oldObj, int newType)
{
    int oldType = oldObj->getObjectType();
    if (oldType == newType)
        return oldObj;
    if (! IS_CDS_ITEM(oldType) || ! IS_CDS_ITEM(newType))
    {
        throw _Exception(_("Cannot convert object type ") + oldType +
                        " to " + newType);
    }

    Ref<CdsObject> newObj = CdsObject::createObject(newType);

    oldObj->copyTo(newObj);

    return newObj;
}

// returns nil if file ignored due to configuration
Ref<CdsObject> ContentManager::createObjectFromFile(String path, bool magic, bool allow_fifo)
{
    String filename = get_filename(path);

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw _Exception(_("Failed to stat ") + path + _(" , ") + mt_strerror(errno));
    }

    Ref<CdsObject> obj;
    if (S_ISREG(statbuf.st_mode) || (allow_fifo && S_ISFIFO(statbuf.st_mode))) // item
    {
        /* retrieve information about item and decide
           if it should be included */
        String mimetype;
        String upnp_class;
        String extension;

        // get file extension
        int dotIndex = filename.rindex('.');
        if (dotIndex > 0)
            extension = filename.substring(dotIndex + 1);

        if (magic)
            mimetype = extension2mimetype(extension);

        if (mimetype == nil && magic)
        {
            if (ignore_unknown_extensions)
                return nil; // item should be ignored
#ifdef HAVE_MAGIC        
            mimetype = get_mime_type(ms, reMimetype, path);
#endif
        }
        if (mimetype != nil)
        {
            upnp_class = mimetype2upnpclass(mimetype);
        }

        if (!string_ok(upnp_class))
        {
            String content_type = mimetype_contenttype_map->get(mimetype);
            if (content_type == CONTENT_TYPE_OGG)
            {
                if (isTheora(path))
                    upnp_class = _(UPNP_DEFAULT_CLASS_VIDEO_ITEM);
                else
                    upnp_class = _(UPNP_DEFAULT_CLASS_MUSIC_TRACK);
            }
        }

        Ref<CdsItem> item(new CdsItem());
        obj = RefCast(item, CdsObject);
        item->setLocation(path);
        item->setMTime(statbuf.st_mtime);
        item->setSizeOnDisk(statbuf.st_size);
        if (mimetype != nil)
            item->setMimeType(mimetype);
        if (upnp_class != nil)
            item->setClass(upnp_class);
        Ref<StringConverter> f2i = StringConverter::f2i();
        obj->setTitle(f2i->convert(filename));
        if (magic)
            MetadataHandler::setMetadata(item);
    }
    else if (S_ISDIR(statbuf.st_mode))
    {
        Ref<CdsContainer> cont(new CdsContainer());
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
    }
    else
    {
        // only regular files and directories are supported
        throw _Exception(_("ContentManager: skipping file ") + path.c_str());
    }
//    Ref<StringConverter> f2i = StringConverter::f2i();
//    obj->setTitle(f2i->convert(filename));
    return obj;
}

String ContentManager::extension2mimetype(String extension)
{
    if (extension_mimetype_map == nil)
        return nil;

    if (!extension_map_case_sensitive)
        extension = extension.toLower();

    return extension_mimetype_map->get(extension);
}

String ContentManager::mimetype2upnpclass(String mimeType)
{
    if (mimetype_upnpclass_map == nil)
        return nil;
    String upnpClass = mimetype_upnpclass_map->get(mimeType);
    if (upnpClass != nil)
        return upnpClass;
    // try to match foo
    Ref<Array<StringBase> > parts = split_string(mimeType, '/');
    if (parts->size() != 2)
        return nil;
    return mimetype_upnpclass_map->get((String)parts->get(0) + "/*");
}

void ContentManager::initLayout()
{

    if (layout == nil)
    {
        AUTOLOCK(mutex);
        if (layout == nil)
            try
            {
                String layout_type = 
                    ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
                if (layout_type == "js")
                {
#ifdef HAVE_JS
                    layout = Ref<Layout>((Layout *)new JSLayout());
#else
                    log_error("Cannot init layout: MediaTomb compiled without js support but js script was requested.");
#endif
                }
                else if (layout_type == "builtin")
                {
                    layout = Ref<Layout>((FallbackLayout *)new FallbackLayout());
                }
            }
        catch (Exception e)
        {
            layout = nil;
            log_error("ContentManager virtual container layout: %s\n", e.getMessage().c_str());
        }
    }
}

#ifdef HAVE_JS
void ContentManager::initJS()
{
    if (playlist_parser_script == nil)
        playlist_parser_script = Ref<PlaylistParserScript>(new PlaylistParserScript(Runtime::getInstance()));

#ifdef HAVE_LIBDVDNAV
    if ((ConfigManager::getInstance()->getOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE) == "js") && (dvd_import_script == nil))
    {
        dvd_import_script = Ref<DVDImportScript>(new DVDImportScript(Runtime::getInstance()));
    }
#endif
}

void ContentManager::destroyJS()
{
    playlist_parser_script = nil;
#ifdef HAVE_LIBDVDNAV
    dvd_import_script = nil;
#endif
}

#endif // HAVE_JS

void ContentManager::destroyLayout()
{
    layout = nil;
}

void ContentManager::reloadLayout()
{
    destroyLayout();
    initLayout();
}

void ContentManager::threadProc()
{
    Ref<GenericTask> task;
    AUTOLOCK(mutex);
    working = true;
    while(! shutdownFlag)
    {
        currentTask = nil;
        if(((task = taskQueue1->dequeue()) == nil) && ((task = taskQueue2->dequeue()) == nil))
        {
            working = false;
            /* if nothing to do, sleep until awakened */
            cond->wait();
            working = true;
            continue;
        }
        else
        {
            currentTask = task;
        }
        AUTOUNLOCK();

//        log_debug(("Async START %s\n", task->getDescription().c_str()));
        try
        {
            if (task->isValid())
                task->run();
        }
        catch (ServerShutdownException se)
        {
            shutdownFlag = true;
        }
        catch (Exception e)
        {
            log_error("Exception caught: %s\n", e.getMessage().c_str());
            e.printStackTrace();
        }
//        log_debug(("ASYNC STOP  %s\n", task->getDescription().c_str()));
        if (! shutdownFlag)
            AUTORELOCK();
    }

    Storage::getInstance()->threadCleanup();
}

void *ContentManager::staticThreadProc(void *arg)
{
    ContentManager *inst = (ContentManager *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}

void ContentManager::addTask(zmm::Ref<GenericTask> task, bool lowPriority)
{
    AUTOLOCK(mutex);

    task->setID(taskID++);

    if (! lowPriority)
        taskQueue1->enqueue(task);
    else
        taskQueue2->enqueue(task);
    signal();
}

/* sync / async methods */
void ContentManager::loadAccounting(bool async)
{
    if (async)
    {
        Ref<GenericTask> task(new CMLoadAccountingTask());
        task->setDescription(_("Initializing statistics"));
        addTask(task);
    }
    else
    {
        _loadAccounting();
    }
}

int ContentManager::addFile(zmm::String path, bool recursive, bool async, 
                            bool hidden, bool lowPriority, bool cancellable)
{
    String rootpath;
    if (check_path(path, true))
        rootpath = path;
    return addFileInternal(path, rootpath, recursive, async, hidden, lowPriority, 0, cancellable);
}

int ContentManager::addFileInternal(String path, String rootpath,
                                    bool recursive, 
                                    bool async, bool hidden, bool lowPriority, 
                                    unsigned int parentTaskID, 
                                    bool cancellable)
{
    if (async)
    {
        Ref<GenericTask> task(new CMAddFileTask(path, rootpath, recursive, hidden, cancellable));
        task->setDescription(_("Adding: ") + path);
        task->setParentID(parentTaskID);
        addTask(task, lowPriority);
        return INVALID_OBJECT_ID;
    }
    else
    {
        return _addFile(path, rootpath, recursive, hidden);
    }
}

#ifdef ONLINE_SERVICES
void ContentManager::fetchOnlineContent(service_type_t service,
                                        bool lowPriority, bool cancellable,
                                        bool unscheduled_refresh)
{
    Ref<OnlineService> os = online_services->getService(service);
    if (os == nil)
    {
        log_debug("No surch service! %d\n", service);
        throw _Exception(_("Service not found!"));
    }
    fetchOnlineContentInternal(os, lowPriority, cancellable, 0,
                               unscheduled_refresh);
}

void ContentManager::fetchOnlineContentInternal(Ref<OnlineService> service, 
                                        bool lowPriority, bool cancellable,
                                        unsigned int parentTaskID, 
                                        bool unscheduled_refresh)
{
    if (layout_enabled)
        initLayout();

    Ref<GenericTask> task(new CMFetchOnlineContentTask(service, layout, cancellable, 
                                                  unscheduled_refresh));
    task->setDescription(_("Updating content from ") + 
                         service->getServiceName());
    task->setParentID(parentTaskID);
    service->incTaskCount();
    addTask(task, lowPriority);    
}

void ContentManager::_fetchOnlineContent(Ref<OnlineService> service, 
                                         unsigned int parentTaskID, 
                                         bool unscheduled_refresh)
{
    throw _Exception(_("Should not be called anymore!"));
    log_debug("Fetching online content!\n");
    if (layout_enabled)
        initLayout();

    if (service->refreshServiceData(layout) && (!shutdownFlag))
    {
        log_debug("Scheduling another task for online service: %s\n",
                  service->getServiceName().c_str());

        if ((service->getRefreshInterval() > 0) || unscheduled_refresh)
            fetchOnlineContentInternal(service, true, true, parentTaskID, 
                                       unscheduled_refresh);
    }
    else
    {
        log_debug("Finished fetch cycle for service: %s\n",
                  service->getServiceName().c_str());

        if (service->getItemPurgeInterval() > 0)
        {
            Ref<Storage> storage = Storage::getInstance();
            Ref<IntArray> ids = storage->getServiceObjectIDs(service->getStoragePrefix());

            struct timespec current, last;
            getTimespecNow(&current);
            last.tv_nsec = 0;
            String temp;

            for (int i = 0; i < ids->size(); i++)
            {
                int object_id = ids->get(i);
                Ref<CdsObject> obj = storage->loadObject(object_id);
                if (obj == nil)
                    continue;

                temp = obj->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE));
                if (!string_ok(temp))
                    continue;

                last.tv_sec = temp.toLong();

                if ((service->getItemPurgeInterval() > 0) && 
                    ((current.tv_sec - last.tv_sec) > service->getItemPurgeInterval()))
                {
                    log_debug("Purging old online service object %s\n", 
                            obj->getTitle().c_str());
                    removeObject(object_id, false);
                }
            } 
        }
    }
}

void ContentManager::cleanupOnlineServiceObjects(zmm::Ref<OnlineService> service)
{
    log_debug("Finished fetch cycle for service: %s\n",
            service->getServiceName().c_str());

    if (service->getItemPurgeInterval() > 0)
    {
        Ref<Storage> storage = Storage::getInstance();
        Ref<IntArray> ids = storage->getServiceObjectIDs(service->getStoragePrefix());

        struct timespec current, last;
        getTimespecNow(&current);
        last.tv_nsec = 0;
        String temp;

        for (int i = 0; i < ids->size(); i++)
        {
            int object_id = ids->get(i);
            Ref<CdsObject> obj = storage->loadObject(object_id);
            if (obj == nil)
                continue;

            temp = obj->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE));
            if (!string_ok(temp))
                continue;

            last.tv_sec = temp.toLong();

            if ((service->getItemPurgeInterval() > 0) &&
                    ((current.tv_sec - last.tv_sec) > service->getItemPurgeInterval()))
            {
                log_debug("Purging old online service object %s\n",
                        obj->getTitle().c_str());
                removeObject(object_id, false);
            }
        }
    }
}

#endif

void ContentManager::invalidateAddTask(Ref<GenericTask> t, String path)
{
    if (t->getType() == AddFile)
    {
        log_debug("comparing, task path: %s, remove path: %s\n", RefCast(t, CMAddFileTask)->getPath().c_str(), path.c_str());
        if ((RefCast(t, CMAddFileTask)->getPath().startsWith(path)))
        {
            log_debug("Invalidating task with path %s\n", RefCast(t, CMAddFileTask)->getPath().c_str());
            t->invalidate();
        }
    }
}

void ContentManager::invalidateTask(unsigned int taskID, task_owner_t taskOwner)
{
    int i;

    if (taskOwner == ContentManagerTask)
    {
        AUTOLOCK(mutex);
        Ref<GenericTask> t = getCurrentTask();
        if (t != nil)
        {
            if ((t->getID() == taskID) || (t->getParentID() == taskID))
            {
                t->invalidate();
            }
        }

        int qsize = taskQueue1->size();

        for (i = 0; i < qsize; i++)
        {
            Ref<GenericTask> t = taskQueue1->get(i);
            if ((t->getID() == taskID) || (t->getParentID() == taskID))
            {
                t->invalidate();
            }
        }

        qsize = taskQueue2->size();
        for (i = 0; i < qsize; i++)
        {
            Ref<GenericTask> t = taskQueue2->get(i);
            if ((t->getID() == taskID) || (t->getParentID() == taskID))
            {
                t->invalidate();
            }
        }
    }
#ifdef ONLINE_SERVICES
    else if (taskOwner == TaskProcessorTask)
        TaskProcessor::getInstance()->invalidateTask(taskID);
#endif
}

void ContentManager::removeObject(int objectID, bool async, bool all)
{
    if (async)
    {
        /*
        // building container path for the description
        Ref<Storage> storage = Storage::getInstance();
        Ref<Array<CdsObject> > objectPath = storage->getObjectPath(objectID);
        Ref<StringBuffer> desc(new StringBuffer(objectPath->size() * 10));
        *desc << "Removing ";
        // skip root container, start from 1
        for (int i = 1; i < objectPath->size(); i++)
            *desc << '/' << objectPath->get(i)->getTitle();
        */
        Ref<GenericTask> task(new CMRemoveObjectTask(objectID, all));
        Ref<Storage> storage = Storage::getInstance();
        String path;
        Ref<CdsObject> obj;

        try
        {
            obj = storage->loadObject(objectID);
            path = obj->getLocation();

            String vpath = obj->getVirtualPath();
            if (string_ok(vpath))
                task->setDescription(_("Removing: ") + obj->getVirtualPath());
        }
        catch (Exception e)
        {
            log_debug("trying to remove an object ID which is no longer in the database! %d\n", objectID);
            return;
        }

        if (IS_CDS_CONTAINER(obj->getObjectType()))
        {
            int i;

            // make sure to remove possible child autoscan directories from the scanlist 
            Ref<AutoscanList> rm_list = autoscan_timed->removeIfSubdir(path);
            for (i = 0; i < rm_list->size(); i++)
            {
                Timer::getInstance()->removeTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), rm_list->get(i)->getTimerParameter(), true);
            }
#ifdef HAVE_INOTIFY
            if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
            {
                rm_list = autoscan_inotify->removeIfSubdir(path);
                for (i = 0; i < rm_list->size(); i++)
                {
                    Ref<AutoscanDirectory> dir = rm_list->get(i);
                    inotify->unmonitor(dir);
                }
            }
#endif

            AUTOLOCK(mutex);
            int qsize = taskQueue1->size();

            // we have to make sure that a currently running autoscan task will not
            // launch add tasks for directories that anyway are going to be deleted
            for (i = 0; i < qsize; i++)
            {
                Ref<GenericTask> t = taskQueue1->get(i);
                invalidateAddTask(t, path);
            }

            qsize = taskQueue2->size();
            for (i = 0; i < qsize; i++)
            {
                Ref<GenericTask> t = taskQueue2->get(i);
                invalidateAddTask(t, path);
            }

            Ref<GenericTask> t = getCurrentTask();
            if (t != nil)
            {
                invalidateAddTask(t, path);
            }
        } 

        addTask(task);
    }
    else
    {
        _removeObject(objectID, all);
    }
}

void ContentManager::rescanDirectory(int objectID, int scanID, scan_mode_t scanMode, String descPath, bool cancellable)
{
    // building container path for the description
    Ref<GenericTask> task(new CMRescanDirectoryTask(objectID, scanID, scanMode, cancellable));
    Ref<AutoscanDirectory> dir = getAutoscanDirectory(scanID, scanMode);
    if (dir == nil)
        return;

    dir->incTaskCount();
    String level;
    if (dir->getScanLevel() == BasicScanLevel)
        level = _("basic");
    else
        level = _("full");

    if (!string_ok(descPath))
        descPath = dir->getLocation();

    task->setDescription(_("Performing ") + level + " scan: " + descPath);
    addTask(task, true); // adding with low priority
}


Ref<AutoscanDirectory> ContentManager::getAutoscanDirectory(int scanID, scan_mode_t scanMode)
{
    if (scanMode == TimedScanMode)
    {
        return autoscan_timed->get(scanID);
    }

#if HAVE_INOTIFY
    else if (scanMode == InotifyScanMode)
    {
        return autoscan_inotify->get(scanID);
    }
#endif
    return nil;
}

Ref<Array<AutoscanDirectory> > ContentManager::getAutoscanDirectories(scan_mode_t scanMode)
{
    if (scanMode == TimedScanMode)
    {
        return autoscan_timed->getArrayCopy();
    }

#if HAVE_INOTIFY
    else if (scanMode == InotifyScanMode)
    {
        return autoscan_inotify->getArrayCopy();
    }
#endif
    return nil;
}

Ref<Array<AutoscanDirectory> > ContentManager::getAutoscanDirectories()
{
    Ref<Array<AutoscanDirectory> > all = autoscan_timed->getArrayCopy();

#if HAVE_INOTIFY
    Ref<Array<AutoscanDirectory> > ino = autoscan_inotify->getArrayCopy();
    if (ino != nil)
        for (int i = 0; i < ino->size(); i ++)
            all->append(ino->get(i));
#endif
    return all;
}

Ref<AutoscanDirectory> ContentManager::getAutoscanDirectory(String location)
{
    // \todo change this when more scanmodes become available
    Ref<AutoscanDirectory> dir = autoscan_timed->get(location);
#if HAVE_INOTIFY
    if (dir == nil)
        dir = autoscan_inotify->get(location);
#endif
    return dir;
}

void ContentManager::removeAutoscanDirectory(int scanID, scan_mode_t scanMode)
{
    if (scanMode == TimedScanMode)
    {
        Ref<Storage> storage = Storage::getInstance();
        Ref<AutoscanDirectory> adir = autoscan_timed->get(scanID);
        if (adir == nil)
            throw _Exception(_("can not remove autoscan directory - was not an autoscan"));

        autoscan_timed->remove(scanID);
        storage->removeAutoscanDirectory(adir->getStorageID());
        SessionManager::getInstance()->containerChangedUI(adir->getObjectID());
        
        // if 3rd parameter is true: won't fail if scanID doesn't exist
        Timer::getInstance()->removeTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), adir->getTimerParameter(), true);

    }
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (scanMode == InotifyScanMode)
        {
            Ref<Storage> storage = Storage::getInstance();
            Ref<AutoscanDirectory> adir = autoscan_inotify->get(scanID);
            if (adir == nil)
                throw _Exception(_("can not remove autoscan directory - was not an autoscan"));
            autoscan_inotify->remove(scanID);
            storage->removeAutoscanDirectory(adir->getStorageID());
            SessionManager::getInstance()->containerChangedUI(adir->getObjectID());
            inotify->unmonitor(adir);
        }
    }
#endif
    
}

void ContentManager::removeAutoscanDirectory(int objectID)
{
    Ref<Storage> storage = Storage::getInstance();
    Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(objectID);
    if (adir == nil)
        throw _Exception(_("can not remove autoscan directory - was not an autoscan"));

    if (adir->getScanMode() == TimedScanMode)
    {
        autoscan_timed->remove(adir->getLocation());
        storage->removeAutoscanDirectoryByObjectID(objectID);
        SessionManager::getInstance()->containerChangedUI(objectID);
        Timer::getInstance()->removeTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), adir->getTimerParameter(), true);
    }
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (adir->getScanMode() == InotifyScanMode)
        {
            autoscan_inotify->remove(adir->getLocation());
            storage->removeAutoscanDirectoryByObjectID(objectID);
            SessionManager::getInstance()->containerChangedUI(objectID);
            inotify->unmonitor(adir);
        }
    }
#endif
}

void ContentManager::removeAutoscanDirectory(String location)
{
    /// \todo change this when more scanmodes become avaiable
    Ref<AutoscanDirectory> adir = autoscan_timed->get(location);
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (adir == nil)
            adir = autoscan_inotify->get(location);
    }
#endif
    if (adir == nil)
        throw _Exception(_("can not remove autoscan directory - was not an autoscan"));
    
    removeAutoscanDirectory(adir->getObjectID());
}

void ContentManager::handlePeristentAutoscanRemove(int scanID, scan_mode_t scanMode)
{
    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    Ref<Storage> st = Storage::getInstance();
    if (adir->persistent())
    {
        adir->setObjectID(INVALID_OBJECT_ID);
        st->updateAutoscanDirectory(adir);
    }
    else
    {
        removeAutoscanDirectory(adir->getScanID(), adir->getScanMode());
        st->removeAutoscanDirectory(adir->getStorageID());
    }
}

void ContentManager::handlePersistentAutoscanRecreate(int scanID, scan_mode_t scanMode)
{
    Ref<AutoscanDirectory> adir = getAutoscanDirectory(scanID, scanMode);
    int id = ensurePathExistence(adir->getLocation());
    adir->setObjectID(id);
    Storage::getInstance()->updateAutoscanDirectory(adir);
}

void ContentManager::setAutoscanDirectory(Ref<AutoscanDirectory> dir)
{
    Ref<Storage> storage = Storage::getInstance();
    Ref<AutoscanDirectory> original;

    // We will have to change this for other scan modes
    original = autoscan_timed->getByObjectID(dir->getObjectID());
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (original == nil)
            original = autoscan_inotify->getByObjectID(dir->getObjectID());
    }
#endif

    if (original != nil)
        dir->setStorageID(original->getStorageID());

    storage->checkOverlappingAutoscans(dir);

    // adding a new autoscan directory
    if (original == nil)
    {
        if (dir->getObjectID() == CDS_ID_FS_ROOT)
            dir->setLocation(_(FS_ROOT_DIRECTORY));
        else
        {
            log_debug("objectID: %d\n", dir->getObjectID());
            Ref<CdsObject> obj = storage->loadObject(dir->getObjectID());
            if (obj == nil
                    || ! IS_CDS_CONTAINER(obj->getObjectType())
                    || obj->isVirtual())
                throw _Exception(_("tried to remove an illegal object (id) from the list of the autoscan directories"));
            
            log_debug("location: %s\n", obj->getLocation().c_str());
            
            if (!string_ok(obj->getLocation()))
                throw _Exception(_("tried to add an illegal object as autoscan - no location information available!"));
    
            dir->setLocation(obj->getLocation());
        }
        dir->resetLMT();
        storage->addAutoscanDirectory(dir);
        if (dir->getScanMode() == TimedScanMode)
        {
            autoscan_timed->add(dir);
            timerNotify(dir->getTimerParameter());
        }
#ifdef HAVE_INOTIFY
        if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
        {
            if (dir->getScanMode() == InotifyScanMode)
            {
                autoscan_inotify->add(dir);
                inotify->monitor(dir);
            }
        }
#endif
        SessionManager::getInstance()->containerChangedUI(dir->getObjectID());
        return;
    }

    if (original->getScanMode() == TimedScanMode)
        Timer::getInstance()->removeTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON(this), original->getTimerParameter(), true);
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (original->getScanMode() == InotifyScanMode)
        {
            inotify->unmonitor(original);
        }
    }
#endif

    Ref<AutoscanDirectory> copy(new AutoscanDirectory());
    original->copyTo(copy);

    // changing from full scan to basic scan need to reset last modification time
    if ((copy->getScanLevel() == FullScanLevel) && (dir->getScanLevel() == BasicScanLevel))
    {
        copy->setScanLevel(BasicScanLevel);
        copy->resetLMT();
    } 
    else if (((copy->getScanLevel() == FullScanLevel) &&
             (dir->getScanLevel() == FullScanLevel)) && 
             (!copy->getRecursive() && dir->getRecursive()))
    {
        copy->resetLMT();
    }

    copy->setScanLevel(dir->getScanLevel());
    copy->setHidden(dir->getHidden());
    copy->setRecursive(dir->getRecursive());
    copy->setInterval(dir->getInterval());

    if (copy->getScanMode() == TimedScanMode)
    {
        autoscan_timed->remove(copy->getScanID());
    }
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (copy->getScanMode() == InotifyScanMode)
        {
            autoscan_inotify->remove(copy->getScanID());
        }
    }
#endif

    copy->setScanMode(dir->getScanMode());

    if (dir->getScanMode() == TimedScanMode)
    {
        autoscan_timed->add(copy);
        timerNotify(copy->getTimerParameter());
    }
#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
    {
        if (dir->getScanMode() == InotifyScanMode)
        {
            autoscan_inotify->add(copy);
            inotify->monitor(copy);
        }
    }
#endif

    storage->updateAutoscanDirectory(copy);
    if (original->getScanMode() != copy->getScanMode())
        SessionManager::getInstance()->containerChangedUI(copy->getObjectID());
}

#ifdef HAVE_MAGIC
zmm::String ContentManager::getMimeTypeFromBuffer(void *buffer, size_t length)
{
    return get_mime_type_from_buffer(ms, reMimetype, buffer, length); 
}
#endif

#ifdef YOUTUBE
void ContentManager::checkCachedURLs()
{
    AUTOLOCK(urlcache_mutex);

    time_t now = time(NULL);

    log_debug("Checking cached URLs..stored: %d\n", cached_urls->size());
    int count = 0;
    int i = 0;

    while (count < cached_urls->size())
    {
        Ref<CachedURL> cached = cached_urls->get(i);
        if (cached != nil)
        {
            // do not increment index because remove unordered places the
            // last array element into the removed slot
            if ((cached->getLastAccessTime() + URL_CACHE_LIFETIME) < now)
            {
                log_debug("URL of object: %d, url: %s exceeds "
                          "lifetime (%lld < %lld), purging...\n", 
                          cached_urls->get(i)->getObjectID(), 
                          cached_urls->get(i)->getURL().c_str(), 
                        (long long)(cached->getLastAccessTime() + 
                            URL_CACHE_LIFETIME), 
                        (long long)now);
                cached_urls->removeUnordered(i);
            }
            else
                i++;
        }
        count++; 
    }

    log_debug("URL Cache check complete, remaining items: %d\n",
            cached_urls->size());

    if (cached_urls->size() > 0)
    {
        Ref<TimerParameter> url_cache_check(new 
                TimerParameter(TimerParameter::IDURLCache, -1));

        Timer::getInstance()->addTimerSubscriber(
                AS_TIMER_SUBSCRIBER_SINGLETON(this), 
                URL_CACHE_CHECK_INTERVAL, 
                RefCast(url_cache_check, Object), true);
    }
}

void ContentManager::cacheURL(zmm::Ref<CachedURL> url)
{
    AUTOLOCK(urlcache_mutex);
    time_t oldest = time(NULL); 
    int oldest_index = -1;
    bool added = false;
    int old_size = cached_urls->size();

    log_debug("Request to cache id %d, URL %s\n", url->getObjectID(),
              url->getURL().c_str());
    for (int i = 0; i < cached_urls->size(); i++)
    {
        Ref<CachedURL> cached = cached_urls->get(i);
        if (cached != nil)
        {
            // get time of the first replacement candidate
            if (cached->getLastAccessTime() < oldest)
            {
                oldest = cached->getLastAccessTime();
                oldest_index = i;
            }

            // this is an update for the same object 
            if (url->getObjectID() == cached->getObjectID())
            {
                log_debug("Updating cache for object %d\n", url->getObjectID());
                cached_urls->set(url, i);
                added = true;
                break;
            }
        }
    }

    if (!added)
    {
        // if our storage capacity is exceeded, we will purge an existing item
        if (cached_urls->size() >= MAX_CACHED_URLS)
        {
            log_debug("Checking if we need to remove something old...");
            if ((oldest_index > -1) && (oldest_index <= MAX_CACHED_URLS))
            {
                log_debug("Removing old url from cache of object %d\n", 
                        cached_urls->get(oldest_index)->getObjectID());
                cached_urls->removeUnordered(oldest_index);
            }
            else
            {
                throw _Exception(_("Index exceeds URL cache size: ") +
                                 String::from(oldest_index));
            }
        }
           
        log_debug("Appeinding url to the cache: %d\n", url->getObjectID());
        cached_urls->append(url);
    }

    if ((cached_urls->size() > 0) && (old_size == 0)) 
    {
        log_debug("URL Cache is not empty, adding invalidation timer!\n");
        Ref<TimerParameter> url_cache_check(new 
                TimerParameter(TimerParameter::IDURLCache, -1));

        Timer::getInstance()->addTimerSubscriber(
                AS_TIMER_SUBSCRIBER_SINGLETON(this), 
                URL_CACHE_CHECK_INTERVAL, 
                RefCast(url_cache_check, Object), true);
    }
}

String ContentManager::getCachedURL(int objectID)
{
    AUTOLOCK(urlcache_mutex);
    log_debug("Asked for an url from cache...\n");
    for (int i = 0; i < cached_urls->size(); i++)
    {
        Ref<CachedURL> cached = cached_urls->get(i);
        if (cached != nil)
        {
            if (cached->getObjectID() == objectID)
            {
                log_debug("Found URL in cache for object ID %d, URL: %s\n",
                        objectID, cached->getURL().c_str());
                return cached->getURL();
            }
        }
    }
    return nil;
}
#endif

CMAddFileTask::CMAddFileTask(String path, String rootpath, bool recursive, bool hidden, bool cancellable) : GenericTask(ContentManagerTask)
{
    this->path = path;
    this->rootpath = rootpath;
    this->recursive = recursive;
    this->hidden = hidden;
    this->taskType = AddFile;
    this->cancellable = cancellable;
}

String CMAddFileTask::getPath()
{
    return path;
}

String CMAddFileTask::getRootPath()
{
    return rootpath;
}
void CMAddFileTask::run()
{
    log_debug("running add file task with path %s recursive: %d\n", path.c_str(), recursive);
    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->_addFile(path, nil, recursive, hidden, Ref<GenericTask> (this));
}

CMRemoveObjectTask::CMRemoveObjectTask(int objectID, bool all) : GenericTask(ContentManagerTask)
{
    this->objectID = objectID;
    this->all = all;
    this->taskType = RemoveObject;
    cancellable = false;
}

void CMRemoveObjectTask::run()
{
    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->_removeObject(objectID, all);
}

CMRescanDirectoryTask::CMRescanDirectoryTask(int objectID, int scanID, scan_mode_t scanMode, bool cancellable) : GenericTask(ContentManagerTask)
{
    this->scanID = scanID;
    this->scanMode = scanMode;
    this->objectID = objectID;
    this->taskType = RescanDirectory;
    this->cancellable = cancellable;
}

void CMRescanDirectoryTask::run()
{
    Ref<ContentManager> cm = ContentManager::getInstance();
    Ref<AutoscanDirectory> dir = cm->getAutoscanDirectory(scanID, scanMode);
    if (dir == nil)
        return;

    cm->_rescanDirectory(objectID, dir->getScanID(), dir->getScanMode(), dir->getScanLevel(), Ref<GenericTask> (this));
    dir->decTaskCount();
    
    if (dir->getTaskCount() == 0)
    {
        dir->updateLMT();
        if (dir->getScanMode() == TimedScanMode)
            Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON_FROM_REF(cm), dir->getInterval(), dir->getTimerParameter(), true);
    }
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(Ref<OnlineService> service,
                                                   Ref<Layout> layout,
                                                   bool cancellable,
                                                   bool unscheduled_refresh) : GenericTask(ContentManagerTask)
{
    this->layout = layout;
    this->service = service;
    this->taskType = FetchOnlineContent;
    this->cancellable = cancellable;
    this->unscheduled_refresh = unscheduled_refresh;
}

void CMFetchOnlineContentTask::run()
{
    if (this->service == nil)
    {
        log_debug("Received invalid service!\n");
        return;
    }
    try
    {
        Ref<GenericTask> t(new TPFetchOnlineContentTask(service, layout, 
                                                   cancellable, 
                                                   unscheduled_refresh));
        TaskProcessor::getInstance()->addTask(t);
    }
    catch (Exception ex)
    {
        log_error("%s\n", ex.getMessage().c_str());
    }
}
#endif//ONLINE_SERVICES

CMLoadAccountingTask::CMLoadAccountingTask() : GenericTask(ContentManagerTask)
{
    this->taskType = LoadAccounting;
}

void CMLoadAccountingTask::run()
{
    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->_loadAccounting();
}

CMAccounting::CMAccounting() : Object()
{
    totalFiles = 0;
}

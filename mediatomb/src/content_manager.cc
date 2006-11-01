/*  content_manager.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

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

Ref<RExp> reMimetype;

static String get_filename(String path)
{
    if (path.charAt(path.length() - 1) == '/') // cut off trailing slash
    {
        path = path.substring(0, path.length() - 1);
    }
    int pos = path.rindex('/');
    if (pos < 0)
        return path;
    else
        return path.substring(pos + 1);
}

ContentManager::ContentManager() : Object()
{
    taskMutex = Ref<Mutex>(new Mutex());
    taskCond = Ref<Cond>(new Cond(taskMutex));
    ignore_unknown_extensions = 0;

    shutdownFlag = false;
   
    try 
    {
        Ref<Storage> storage = Storage::getInstance();
        String lastmod = storage->getInternalSetting(_(SET_LAST_MODIFIED));
        if (lastmod != nil)
        {
#if SIZEOF_TIME_T > 4
            last_modified = lastmod.toLong();
#else
            last_modified = lastmod.toInt();
#endif
            log_debug("Loaded last modified: %d\n", last_modified);
        }
        else
        {
            log_debug("No internal setting for last modified\n");
            last_modified = 0;
        }
    }
    catch (Exception ex)
    {
        log_debug("Exception raised when asking for last modified flag!\n");
        ex.printStackTrace();
        last_modified = 0;
    }
    acct = Ref<CMAccounting>(new CMAccounting());    
    taskQueue1 = Ref<ObjectQueue<CMTask> >(new ObjectQueue<CMTask>(CM_INITIAL_QUEUE_SIZE));
    taskQueue2 = Ref<ObjectQueue<CMTask> >(new ObjectQueue<CMTask>(CM_INITIAL_QUEUE_SIZE));    
    
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> mapEl;  
    
    // loading extension - mimetype map  
    mapEl = cm->getElement(_("/import/mappings/extension-mimetype"));
    if (mapEl == nil)
    {
        log_debug("extension-mimetype mappings not found\n");
    }
    else
    {
        extension_mimetype_map = cm->createDictionaryFromNodeset(mapEl, _("map"), _("from"), _("to"));
    }

    String optIgnoreUnknown = cm->getOption(
        _("/import/mappings/extension-mimetype/attribute::ignore-unknown"));
    if (optIgnoreUnknown != nil && optIgnoreUnknown == "yes")
        ignore_unknown_extensions = 1;

    
    // loading mimetype - upnpclass map
    mapEl = cm->getElement(_("/import/mappings/mimetype-upnpclass"));
    if (mapEl == nil)
    {
        log_debug("mimetype-upnpclass mappings not found\n");
    }
    else
    {
        mimetype_upnpclass_map = cm->createDictionaryFromNodeset(mapEl, _("map"), _("from"), _("to"));
    }    
    
    /* init fielmagic */
#ifdef HAVE_MAGIC
    if (! ignore_unknown_extensions)
    {
        ms = magic_open(MAGIC_MIME);
        if (ms == NULL)
        {
	    log_error("magic_open failed\n");
            return;
        }
        String magicFile = cm->getOption(_("/import/magic-file"));
        if (! string_ok(magicFile))
            magicFile = nil;
        if (magic_load(ms, (magicFile == nil) ? NULL : magicFile.c_str()) == -1)
        {
            log_warning("magic_load: %s\n", magic_error(ms));
            magic_close(ms);
            ms = NULL;
        }
    }
#endif // HAVE_MAGIC
}

ContentManager::~ContentManager()
{
#ifdef HAVE_MAGIC
    if (ms)
        magic_close(ms);
#endif
}

void ContentManager::init()
{   
    int ret;

    reMimetype = Ref<RExp>(new RExp());
    reMimetype->compile(_(MIMETYPE_REGEXP));
    
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        throw _Exception(_("Could not initialize attribute"));
    }
   
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    ret = pthread_create(
        &taskThread,
        &attr, // attr
        ContentManager::staticThreadProc,
        this
    );
    if (ret != 0)
    {
        throw _Exception(_("Could not start task thread"));
    }

    pthread_attr_destroy(&attr);

    loadAccounting();
}

#if 0
void ContentManager::setLastModifiedTime(time_t lm)
{
    pthread_mutex_lock(&last_modified_mutex);
    this->last_modified = lm;
    pthread_mutex_unlock(&last_modified_mutex);
}
#endif

void ContentManager::shutdown()
{
    shutdownFlag = true;
    lock();
    signal();
    unlock();

// detached
//    pthread_join(updateThread, NULL);
}

Ref<ContentManager> ContentManager::instance = nil;
Mutex ContentManager::getInstanceMutex = Mutex();

Ref<ContentManager> ContentManager::getInstance()
{
    if (instance == nil)
    {
        getInstanceMutex.lock();
        if (instance == nil)
        {
            try
            {
                instance = Ref<ContentManager>(new ContentManager());
                instance->init();
            }
            catch (Exception e)
            {
                getInstanceMutex.unlock();
                throw e;
            }
        }
        getInstanceMutex.unlock();
    }
    return instance;
}

Ref<CMAccounting> ContentManager::getAccounting()
{
    return acct;
}
Ref<CMTask> ContentManager::getCurrentTask()
{
    Ref<CMTask> task;
    lock();
    task = currentTask;
    unlock();
    return task;
}

void ContentManager::_loadAccounting()
{
    Ref<Storage> storage = Storage::getInstance();
    acct->totalFiles = storage->getTotalFiles();
}
void ContentManager::_addFile(String path, bool recursive, bool hidden)
{
    if (hidden == false)
    {

        String filename = get_filename(path);
        if (string_ok(filename) && filename.charAt(0) == '.')
            return;
    }

    // never add the server configuration file
    if (ConfigManager::getInstance()->getConfigFilename() == path)
        return;


#ifdef HAVE_JS
    initScripting();
#endif
    
    // _addFile2(path, recursive);
    // return;
    
    /*
    if (path.charAt(path.length() - 1) == '/')
    {
        path = path.substring(0, path.length() - 1);
    }
    */

    Ref<Storage> storage = Storage::getInstance();
    
    Ref<UpdateManager> um = UpdateManager::getInstance();
    //Ref<StringConverter> f2i = StringConverter::f2i();
    
    Ref<CdsObject> obj = storage->findObjectByPath(path);
    if (obj == nil)
    {
        obj = createObjectFromFile(path);
        if (obj == nil) // object ignored
            return;
        if (IS_CDS_ITEM(obj->getObjectType()))
        {
            addObject(obj);
#ifdef HAVE_JS
            scripting->processCdsObject(obj);
#endif
        }
    }
    
    if (recursive && IS_CDS_CONTAINER(obj->getObjectType()))
    {
        addRecursive(path, hidden);
    }
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
    int objectType;
    int parentID = storage->removeObject(objectID, all, &objectType);
    if (IS_CDS_CONTAINER(objectType))
        SessionManager::getInstance()->containerChangedUI(parentID);
    UpdateManager::getInstance()->containerChanged(parentID);
    
    // reload accounting
    loadAccounting();
}

void ContentManager::_rescanDirectory(int containerID, scan_level_t scanLevel)
{
    log_debug("start\n");
    int ret;
    time_t last_modfied_current_max = last_modified;
    struct dirent *dent;
    struct stat statbuf;
    String location;
    String path;

    //    Ref<Array<int> > list(new Array<int>());

    // TEST CODE REMOVE THIS 
    // containerID = 33651; 

    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(containerID);
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    if (!IS_CDS_CONTAINER(obj->getObjectType()))
    {
        throw _Exception(_("Object is not a container: rescan must be triggered on directories\n"));
    }

    log_debug("Rescanning container: %s, id=%d\n", 
            obj->getTitle().c_str(), containerID);

    log_debug("Starting with last modified time: %lld\n", last_modified);

    location = obj->getLocation();
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
        throw _Exception(_("Could not list directory ")+
                location + " : " + strerror(errno));
    }

    Ref<DBRHash<int> > list = storage->getObjects(containerID);
    log_debug("list: %s\n", list->debugGetAll().c_str());

    while (((dent = readdir(dir)) != NULL) && (!shutdownFlag))
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
        }

        path = location + "/" + name; 
        log_debug("Checking name: %s\n", path.c_str());
        ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        {
            log_error("Failed to stat %s\n"), path.c_str();
            continue;
        }

        if (S_ISREG(statbuf.st_mode))
        {
            int objectID = storage->isFileInDatabase(String(path));
            log_debug("%s is a regular file! - objectID: %d\n", path.c_str(), objectID);
            if (objectID > 0)
            {
                list->remove(objectID);
                log_debug("removing %d -> list: %s\n", objectID, list->debugGetAll().c_str());

                if (scanLevel == FullScan)
                {
                    log_debug("last modified: %lld\n", last_modified);
                    log_debug("file last modification time: %lld\n", statbuf.st_mtime);

                    // check modification time and update file if chagned
                    if (last_modified < statbuf.st_mtime)
                    {
                        // readd object - we have to do this in order to trigger
                        // scripting
                        log_debug("removing object %d, %s\n", objectID, name);
                        removeObject(objectID, false);
                        addFile(path, false, false);
                        // update time variable
                        if (statbuf.st_mtime > last_modfied_current_max)
                        {
                            last_modfied_current_max = statbuf.st_mtime;
                        }
                    }
                }
                else if (scanLevel == BasicScan)
                    continue;
                else
                    throw _Exception(_("Unsupported scan level!"));

            }
            else
            {
                // add file, not recursive, not async
                // make sure not to add the current config.xml
                if (cm->getConfigFilename() != path)
                    addFile(path, false, false);
            }
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
            log_debug("%s is a directory!\n", path.c_str());
            int objectID = storage->isFolderInDatabase(path);
            if (objectID > 0)
            {
                list->remove(objectID);
                log_debug("removing %d -> list: %s\n", objectID, list->debugGetAll().c_str());
                log_debug("adding %d to containers stack\n", objectID);
                // add a task to rescan the directory that was found
                rescanDirectory(objectID, scanLevel, true);
            }
            else
            {
                log_debug("Directory is not in database!\n");
                // add directory, recursive, async
                addFile(path, true, true);
            }
        }
    } // while
    log_debug("removing... list: %s\n", list->debugGetAll().c_str());
    if (list->size() > 0)
    {
        storage->removeObjects(list);
        UpdateManager::getInstance()->containerChanged(containerID);
    }

    closedir(dir);

    last_modified = last_modfied_current_max;
    storage->storeInternalSetting(_(SET_LAST_MODIFIED), String::from(last_modified));
}

/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(String path, bool hidden)
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
    int parentID = storage->findObjectIDByPath(path + "/");
    struct dirent *dent;
    while (((dent = readdir(dir)) != NULL) && (!shutdownFlag))
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
        String newPath = path + "/" + name;

        if (ConfigManager::getInstance()->getConfigFilename() == newPath)
            continue;

        try
        {
            Ref<CdsObject> obj = nil;
            if (parentID > 0)
                obj = storage->findObjectByFilename(String(newPath));
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
#ifdef HAVE_JS
        		if (IS_CDS_ITEM(obj->getObjectType()))
	        	{
                    if (scripting != nil)
    		            scripting->processCdsObject(obj);
                    
                    /// \todo Why was this statement here??? - It seems to be unnecessary
                    //obj = createObjectFromFile(newPath);
        		}
#endif
                if (IS_CDS_CONTAINER(obj->getObjectType()))
                {
                    addRecursive(newPath, hidden);
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
            item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
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
            item->removeMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
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
    addContainerChain(storage->buildContainerPath(parentID, title));
}


int ContentManager::addContainerChain(String chain)
{
    Ref<Storage> storage = Storage::getInstance();
    int updateID = INVALID_OBJECT_ID;
    int containerID;
    
    log_debug("received chain: %s\n", chain.c_str());
    storage->addContainerChain(chain, &containerID, &updateID);
    // if (updateID != INVALID_OBJECT_ID)
    // an invalid updateID is checked by containerChanged()
    UpdateManager::getInstance()->containerChanged(updateID);
    SessionManager::getInstance()->containerChangedUI(updateID);

    return containerID;
}

void ContentManager::updateObject(Ref<CdsObject> obj)
{
    obj->validate();
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<SessionManager> sm = SessionManager::getInstance();
    
    int containerChanged = INVALID_OBJECT_ID;
    storage->updateObject(obj, &containerChanged);
    um->containerChanged(containerChanged);
    sm->containerChangedUI(containerChanged);
    
    um->containerChanged(obj->getParentID());
    if (IS_CDS_CONTAINER(obj->getObjectType()))
        sm->containerChangedUI(obj->getParentID());
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
Ref<CdsObject> ContentManager::createObjectFromFile(String path, bool magic)
{
    String filename = get_filename(path);

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw _Exception(_("Failed to stat ") + path);
    }

    Ref<CdsObject> obj;
    if (S_ISREG(statbuf.st_mode)) // item
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

        Ref<CdsItem> item(new CdsItem());
        obj = RefCast(item, CdsObject);
        item->setLocation(path);
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
    return extension_mimetype_map->get(extension);
}
String ContentManager::mimetype2upnpclass(String mimeType)
{
    if (mimetype_upnpclass_map == nil)
        return nil;
    String upnpClass = mimetype_upnpclass_map->get(mimeType);
    if (upnpClass != nil)
        return upnpClass;
    // try to match foo/*
    Ref<Array<StringBase> > parts = split_string(mimeType, '/');
    if (parts->size() != 2)
        return nil;
    return mimetype_upnpclass_map->get((String)parts->get(0) + "/*");
}

#ifdef HAVE_JS
void ContentManager::initScripting()
{
	if (scripting != nil)
		return;
	try
	{
		scripting = Ref<Scripting>(new Scripting());
		scripting->init();
	}
	catch (Exception e)
	{
		scripting = nil;
		log_error("ContentManager SCRIPTING: %s\n", e.getMessage().c_str());
	}

}
void ContentManager::destroyScripting()
{
	scripting = nil;
}
void ContentManager::reloadScripting()
{
	destroyScripting();
	initScripting();
}
#endif // HAVE_JS

/* experimental file adding */
/*
void ContentManager::addFile2(String path, int recursive)
{
#ifdef HAVE_JS
    initScripting();
#endif

    if (path.charAt(path.length() - 1) == '/')
    {
        path = path.substring(0, path.length() - 1);
    }
    Ref<Storage> storage = Storage::getInstance();

    Ref<Array<StringBase> > parts = split_string(path, '/');
    String curParentID = "0";
    String curPath = "";
    Ref<CdsObject> obj = storage->loadObject("0"); // root container

    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<StringConverter> f2i = StringConverter::f2i();

    for (int i = 0; i < parts->size(); i++)
    {
        String part = parts->get(i);
        curPath = curPath + "/" + part;

        obj = storage->findObjectByTitle(f2i->convert(part), curParentID);

        if (obj == nil) // create object
        {
            obj = createObjectFromFile(curPath);
            if (obj == nil) // object ignored
            {
                log_debug("file ignored: %s\n", curPath.c_str());
                return;
            }
            obj->setParentID(curParentID);
            addObject(obj);
            um->containerChanged(curParentID);
        }
        curParentID = obj->getID();
    }
#ifdef HAVE_JS
    if (IS_CDS_ITEM(obj->getObjectType()))
        scripting->processCdsObject(obj);
#endif

    if (recursive && IS_CDS_CONTAINER(obj->getObjectType()))
    {
        addRecursive(path, curParentID);
    }
    um->flushUpdates();
}
*/

/* scans the given directory and adds everything recursively */
/*
void ContentManager::addFile2(Ref<DirStack> dirStack, String parentID)
{
    Ref<StringConverter> f2i = StringConverter::f2i();

    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<Storage> storage = Storage::getInstance();
    DIR *dir = opendir(path.c_str());
    if (! dir)
    {
        throw _Exception(_("could not list directory ")+
                        path + " : " + strerror(errno));
    }
    struct dirent *dent;
    while ((dent = readdir(dir)) != NULL)
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
        }
        String newPath = path + "/" + name;
        try
        {
            Ref<CdsObject> obj = storage->findObjectByTitle(f2i->convert(name), parentID);
            if (obj == nil) // create object
            {
                obj = createObjectFromFile(newPath);
                if (obj == nil) // object ignored
                {
                    log_debug("file ignored: %s\n", newPath.c_str());
                    return;
                }
                obj->setParentID(parentID);
                addObject(obj);
                um->containerChanged(parentID);
            }
#ifdef HAVE_JS
	    if (IS_CDS_ITEM(obj->getObjectType()))
	    {
		scripting->processCdsObject(obj);
	    }
#endif
            if (IS_CDS_CONTAINER(obj->getObjectType()))
            {
                addRecursive(newPath, obj->getID());
            }
        }
        catch(Exception e)
        {
            log_debug("skipping %s : %s\n", newPath.c_str(), e.getMessage().c_str());
        }
    }
    closedir(dir);
}
*/

/* DirStack object */
/*
DirStack::DirStack(String path) : Object()
{
    pathbuf = Ref<StringBuffer>(new StringBuffer(path));
    dirstack = (dirstack_t *)MALLOC(sizeof(dirstack_t) * 20);
}

DirStack::init(String path)
{
    Ref<Array<StringBase> > parts = split_string(path, '/');
    
}
*/

void ContentManager::threadProc()
{
    Ref<CMTask> task;

    while(! shutdownFlag)
    {
        lock();
        currentTask = nil;
        if(((task = taskQueue1->dequeue()) == nil) && ((task = taskQueue2->dequeue()) == nil))
        {
            /* if nothing to do, sleep until awakened */
            taskCond->wait();
            unlock();
            continue;
        }
        else
        {
            currentTask = task;
        }
        unlock();

//        log_debug(("Async START %s\n", task->getDescription().c_str()));
        try
        {
            task->run();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
//        log_debug(("ASYNC STOP  %s\n", task->getDescription().c_str()));
    }
}
void *ContentManager::staticThreadProc(void *arg)
{
    ContentManager *inst = (ContentManager *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}

void ContentManager::addTask(zmm::Ref<CMTask> task, bool lowPriority)
{
    lock();
    if (! lowPriority)
        taskQueue1->enqueue(task);
    else
        taskQueue2->enqueue(task);
    signal();
    unlock();
}

/* sync / async methods */
void ContentManager::loadAccounting(bool async)
{
    if (async)
    {
        Ref<CMTask> task(new CMLoadAccountingTask());
        task->setDescription(_("Initializing statistics"));
        addTask(task);
    }
    else
    {
        _loadAccounting();
    }
}
void ContentManager::addFile(zmm::String path, bool recursive, bool async, bool hidden)
{
    if (async)
    {
        Ref<CMTask> task(new CMAddFileTask(path, recursive, hidden));
        task->setDescription(_("Adding ") + path);
        addTask(task);
    }
    else
    {
        _addFile(path, recursive, hidden);
    }
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
        Ref<CMTask> task(new CMRemoveObjectTask(objectID, all));
        //task->setDescription(desc->toString());
        task->setDescription(_("description missing!!!!!!!!!!!"));
        addTask(task);
    }
    else
    {
        _removeObject(objectID, all);
    }
}
    
void ContentManager::rescanDirectory(int objectID, scan_level_t scanLevel, bool async)
{
    if (async)
    {
        // building container path for the description
        Ref<CMTask> task(new CMRescanDirectoryTask(objectID, scanLevel));
        task->setDescription(_("Autoscan")); /// \todo description should contain the path that we are rescanning
        addTask(task, true); // adding with low priority
    }
    else
    {
        _rescanDirectory(objectID, scanLevel);
    }
}

CMTask::CMTask() : Object()
{
    cm = ContentManager::getInstance().getPtr();
}
void CMTask::setDescription(String description)
{
    this->description = description;
}
String CMTask::getDescription()
{
    return description;
}


CMAddFileTask::CMAddFileTask(String path, bool recursive, bool hidden) : CMTask()
{
    this->path = path;
    this->recursive = recursive;
    this->hidden = hidden;
}
void CMAddFileTask::run()
{
    log_debug("running add file task with path %s recursive: %d\n", path.c_str(), recursive);
    cm->_addFile(path, recursive, hidden);
}

CMRemoveObjectTask::CMRemoveObjectTask(int objectID, bool all) : CMTask()
{
    this->objectID = objectID;
    this->all = all;
}

void CMRemoveObjectTask::run()
{
    cm->_removeObject(objectID, all);
}

CMRescanDirectoryTask::CMRescanDirectoryTask(int objectID, scan_level_t scanLevel) : CMTask()
{
    this->objectID = objectID;
    this->scanLevel = scanLevel;
}
void CMRescanDirectoryTask::run()
{
    cm->_rescanDirectory(objectID, scanLevel);
}

CMLoadAccountingTask::CMLoadAccountingTask() : CMTask()
{}
void CMLoadAccountingTask::run()
{
    cm->_loadAccounting();
}

CMAccounting::CMAccounting() : Object()
{
    totalFiles = 0;
}


/* ************** experimental file adding ************** */
/* dir cache */

/*
DirCacheEntry::DirCacheEntry() : Object()
{}

DirCache::DirCache() : Object()
{
    capacity = DEFAULT_DIR_CACHE_CAPACITY;
    size = 0;
    // assume the average filelength to be 10 chars
    buffer = Ref<StringBuffer>(new StringBuffer(capacity * 10));
    entries = Ref<Array<DirCacheEntry> >(new Array<DirCacheEntry>(capacity));
}
void DirCache::push(String name)
{
    Ref<DirCacheEntry> entry;
    if (size == capacity)
    {
    	entry = Ref<DirCacheEntry>(new DirCacheEntry());
	    entries->append(entry);
    }
    else
        entry = entries->get(size);
    *buffer << "/" << name;
    entry->end = buffer->length();
    size++;        
}
void DirCache::pop()
{
    if (size <= 0)
        return;
    size--;
    if (size == 0)
        buffer->setLength(0);
    else
        buffer->setLength(entries->get(size - 1)->end);
}
void DirCache::clear()
{
    size = 0;
}
void DirCache::setPath(zmm::String path)
{
    int i;
    Ref<Array<StringBase> > parts = split_string(path, '/');
    size = parts->size();
    for (i = 0; i < parts->size(); i++)
    {
        push(String(parts->get(i)));
    }
}
String DirCache::getPath()
{
    return buffer->toString();
}

int DirCache::createContainers()
{
    Ref<Storage> storage = Storage::getInstance();
    Ref<ContentManager> cm = ContentManager::getInstance();
    Ref<StringConverter> f2i = StringConverter::f2i();

    Ref<CdsObject> cont;

    int prev_end;
    for (int i = 0; i < size; i++)
    {
    	Ref<DirCacheEntry> entry = entries->get(i);
        if (entry->id != 0)
            continue;

	    prev_end = (i == 0) ? 1 : entries->get(i - 1)->end + 1;
        String name = String(buffer->c_str() + prev_end, entry->end - prev_end);
        /// \todo: PC Directory id configurable
        int parentID = ((i == 0) ? 1 : entries->get(i - 1)->id);
        String conv_name = f2i->convert(name);
        Ref<CdsObject> obj = storage->findObjectByTitle(conv_name, parentID);
        if (obj != nil)
        {
            entry->id = obj->getID();
            continue;
        }
        String path(buffer->c_str(), entry->end);
        cont = Ref<CdsObject>(new CdsContainer());
        cont->setParentID(parentID);
        cont->setTitle(conv_name);
        cont->setLocation(path);
        cm->addObject(cont);
        entry->id = cont->getID();
    }
    return cont->getID();
}
*/
/*
void ContentManager::_addFile2(String path, bool recursive)
{
#ifdef HAVE_JS
    initScripting();
#endif

    if (path.charAt(path.length() - 1) == '/')
        path = path.substring(0, path.length() - 1);
    
    int slashPos = path.rindex('/');
    if (slashPos < 0)
        throw _Exception(_("only absolute paths are accepted"));

    Ref<DirCache> dirCache(new DirCache());
    // if not in root container initialize dirCache's path
    if (slashPos > 0)
        dirCache->setPath(path.substring(0, slashPos));

    addRecursive2(dirCache, path.substring(slashPos + 1), recursive);
}

void ContentManager::addRecursive2(Ref<DirCache> dirCache, String filename, bool recursive)
{
    String parentID;
    Ref<CdsObject> obj;
    Ref<UpdateManager> um = UpdateManager::getInstance();

    String path = dirCache->getPath() + '/' + filename;

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
        throw _Exception(_("Failed to stat ") + path);

    if (S_ISREG(statbuf.st_mode)) // item
    {
        obj = createObjectFromFile(dirCache->getPath() +'/'+ filename);
        if (obj != nil)
        {
            int parentID = dirCache->createContainers();
            obj->setParentID(parentID);
#ifdef HAVE_JS
    	    if (IS_CDS_ITEM(obj->getObjectType()))
	        {
                if (scripting != nil)
    	            scripting->processCdsObject(obj);
    	    }
#endif
            addObject(obj);
            um->containerChanged(parentID);
        }
        return;
    }
    if (! recursive)
        return;
    if (S_ISDIR(statbuf.st_mode)) // container
    {
        DIR *dir = opendir(path.c_str());
        if (! dir)
        {
            throw _Exception(_("could not list directory ") +
                            path + " : " + strerror(errno));
        }
        struct dirent *dent;

        dirCache->push(filename);
        while ((dent = readdir(dir)) != NULL)
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
            }
            try
            {
                addRecursive2(dirCache, String(name), recursive);
            }
            catch(Exception e)
            {
                log_warning("Skipping %s/%s : %s\n", dirCache->getPath().c_str(), filename.c_str(),
                       e.getMessage().c_str());
            }
        }
        closedir(dir);
        dirCache->pop();
    }
    else
        throw _Exception(_("unsupported file type: ") + path);
}
*/


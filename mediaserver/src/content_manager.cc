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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// for older versions of filemagic
extern "C" {
#include <magic.h>
}

#include "content_manager.h"
#include "config_manager.h"
#include "update_manager.h"
#include "string_converter.h"
#include "metadata_reader.h"
#include "rexp.h"

struct magic_set *ms = NULL;

using namespace zmm;
using namespace mxml;

/*********************** utils ***********************/
#define MIMETYPE_REGEXP "^([a-z0-9_-]+/[a-z0-9_-]+)"
static Ref<RExp> reMimetype;

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

static String get_mime_type(String file)
{
    if (ms == NULL)
        return nil;
    char *mt = (char *)magic_file(ms, file.c_str());
    if (mt == NULL)
    {
        printf("magic_file: %s\n", magic_error(ms));
        return nil;
    }
    
    String mime_type(mt);

    Ref<Matcher> matcher = reMimetype->matcher(mime_type, 2);
    if (matcher->next())
        return matcher->group(1);
    
    printf("filemagic returned invalid mimetype for %s\n%s\n",
           file.c_str(), mt);
    return nil;
 }

/***************************************************************/

static Ref<ContentManager> instance;

ContentManager::ContentManager() : Object()
{  
    ignore_unknown_extensions = 0;
    ms = NULL;

    shutdownFlag = false;

    acct = Ref<CMAccounting>(new CMAccounting());    
    taskQueue = Ref<Array<CMTask> >(new Array<CMTask>());    

    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> mapEl;  
    
    // loading extension - mimetype map  
    mapEl = cm->getElement("/import/mappings/extension-mimetype");
    if (mapEl == nil)
    {
        printf("extension-mimetype mappings not found\n");
    }
    else
    {
        extension_mimetype_map = cm->createDictionaryFromNodeset(mapEl, "map", "from", "to");
    }

    String optIgnoreUnknown = cm->getOption(
        "/import/mappings/extension-mimetype/attribute::ignore-unknown");
    if (optIgnoreUnknown != nil && optIgnoreUnknown == "yes")
        ignore_unknown_extensions = 1;

    
    // loading mimetype - upnpclass map
    mapEl = cm->getElement("/import/mappings/mimetype-upnpclass");
    if (mapEl == nil)
    {
        printf("mimetype-upnpclass mappings not found\n");
    }
    else
    {
        mimetype_upnpclass_map = cm->createDictionaryFromNodeset(mapEl, "map", "from", "to");
    }    
    
    /* init fielmagic */
    if (! ignore_unknown_extensions)
    {
        ms = magic_open(MAGIC_MIME);
        if (ms == NULL)
        {
	        printf("magic_open failed\n");
            return;
        }
        if (magic_load(ms, NULL) == -1)
        {
            printf("magic_load: %s\n", magic_error(ms));
            magic_close(ms);
            ms = NULL;
        }
    }
}

ContentManager::~ContentManager()
{
    if (ms)
        magic_close(ms);
    pthread_mutex_destroy(&taskMutex);
    pthread_cond_destroy(&taskCond);
}

void ContentManager::init()
{   
    int ret;

    reMimetype = Ref<RExp>(new RExp());
    reMimetype->compile(MIMETYPE_REGEXP);
    
    ret = pthread_mutex_init(&taskMutex, NULL);
    ret = pthread_cond_init(&taskCond, NULL);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    ret = pthread_create(
        &taskThread,
        &attr, // attr
        ContentManager::staticThreadProc,
        this
    );
    
    pthread_attr_destroy(&attr);

    loadAccounting();
}

void ContentManager::shutdown()
{
    shutdownFlag = true;
    lock();
    signal();
    unlock();

// detached
//    pthread_join(updateThread, NULL);
}


Ref<ContentManager> ContentManager::getInstance()
{
    if (instance == nil)
    {
        instance = Ref<ContentManager>(new ContentManager());
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
void ContentManager::_addFile(String path, int recursive)
{
	initScripting();

    if (path.charAt(path.length() - 1) == '/')
    {
        path = path.substring(0, path.length() - 1);
    }
    Ref<Storage> storage = Storage::getInstance();

    Ref<Array<StringBase> > parts = split_string(path, '/');
    String curParentID = "1";
    /// \todo make PC-Directory id configurable
    String curPath = "";
    Ref<CdsObject> obj = storage->loadObject(curParentID); // root container

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
                printf("file ignored: %s\n", curPath.c_str());
                return;
            }
            obj->setParentID(curParentID);
            addObject(obj);
        }
        curParentID = obj->getID();
    }

	if (IS_CDS_ITEM(obj->getObjectType()))
    {
		scripting->processCdsObject(obj);
    }

    if (recursive && IS_CDS_CONTAINER(obj->getObjectType()))
    {
        addRecursive(path, curParentID);
    }
    um->flushUpdates();
}
void ContentManager::_removeObject(String objectID)
{
    /// \todo when removing... what about container updates when removing recursively?
    if (objectID == "0")
        throw Exception("cannot remove root container");
    if (objectID == "1")
        throw Exception("cannot remove PC-Directory container");
    /// \todo make PC-Directory ID configurable
    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);
    storage->removeObject(obj);
    Ref<UpdateManager> um = UpdateManager::getInstance();
    um->containerChanged(obj->getParentID());
    
    // reload accounting
    ContentManager::getInstance()->loadAccounting();
    
    um->flushUpdates();
}


/* scans the given directory and adds everything recursively */
void ContentManager::addRecursive(String path, String parentID)
{
    Ref<StringConverter> f2i = StringConverter::f2i();

    Ref<UpdateManager> um = UpdateManager::getInstance();
    Ref<Storage> storage = Storage::getInstance();
    DIR *dir = opendir(path.c_str());
    if (! dir)
    {
        throw Exception(String("could not list directory ")+
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
                    printf("file ignored: %s\n", newPath.c_str());
                    return;
                }
                obj->setParentID(parentID);
                addObject(obj);
            }
			if (IS_CDS_ITEM(obj->getObjectType()))
			{
				scripting->processCdsObject(obj);
			}
            if (IS_CDS_CONTAINER(obj->getObjectType()))
            {
                addRecursive(newPath, obj->getID());
            }
        }
        catch(Exception e)
        {
            printf("skipping %s : %s\n", newPath.c_str(), e.getMessage().c_str());
        }
    }
    closedir(dir);
}

void ContentManager::updateObject(String objectID, Ref<Dictionary> parameters)
{
    String title = parameters->get("title");
    String upnp_class = parameters->get("class");
    String autoscan = parameters->get("autoscan");
    String mimetype = parameters->get("mime-type");
    String description = parameters->get("description");
    String location = parameters->get("location");

    Ref<MetadataReader> meta (new MetadataReader());
    
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);
    int objectType = obj->getObjectType();

    /// \TODO: if we have an active item, does it mean we first go through IS_ITEM and then thorugh IS_ACTIVE item? ask Gena
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);
        if (string_ok(location)) clone->setLocation(location);

        Ref<CdsItem> cloned_item = RefCast(clone, CdsItem);
        
        if (string_ok(description)) 
        {
            cloned_item->setMetadata(meta->getFieldName(M_DESCRIPTION), description);
        }
        else
        {
            item->removeMetadata(meta->getFieldName(M_DESCRIPTION));
        }

        if (string_ok(mimetype)) cloned_item->setMimeType(mimetype);

        if (!item->equals(clone, true))
        {
            cloned_item->validate();
            storage->updateObject(clone);
            um->containerChanged(item->getParentID());
        }
    }
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        String action = parameters->get("action");
        String state = parameters->get("state");

        Ref<CdsActiveItem> item = RefCast(obj, CdsActiveItem);
        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);

        Ref<CdsActiveItem> cloned_item = RefCast(clone, CdsActiveItem);

        // state and description can be an empty strings - if you want to clear it
        if (string_ok(description)) 
        {
            cloned_item->setMetadata(meta->getFieldName(M_DESCRIPTION), description);
        }
        else
        {
            item->removeMetadata(meta->getFieldName(M_DESCRIPTION));
        }

        if (state != nil) cloned_item->setState(state);

        if (string_ok(mimetype)) cloned_item->setMimeType(mimetype);
        if (string_ok(action)) cloned_item->setAction(action);

        if (!item->equals(clone, true))
        {
            cloned_item->validate();
            storage->updateObject(clone);
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
            storage->updateObject(clone);
            um->containerChanged(cont->getParentID());
        }
    }

    um->flushUpdates();
}

void ContentManager::addObject(zmm::Ref<CdsObject> obj)
{
    obj->validate();
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    storage->addObject(obj);
    um->containerChanged(obj->getParentID());
    
    if (! obj->isVirtual() && IS_CDS_ITEM(obj->getObjectType()))
        ContentManager::getInstance()->getAccounting()->totalFiles++;
}

void ContentManager::updateObject(Ref<CdsObject> obj)
{
    obj->validate();
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    storage->updateObject(obj);
}

Ref<CdsObject> ContentManager::convertObject(Ref<CdsObject> oldObj, int newType)
{
    int oldType = oldObj->getObjectType();
    if (oldType == newType)
        return oldObj;
    if (! IS_CDS_ITEM(oldType) || ! IS_CDS_ITEM(newType))
    {
        throw Exception(String("Cannot convert object type ") + oldType +
                        " to " + newType);
    }

    Ref<CdsObject> newObj = CdsObject::createObject(newType);

    oldObj->copyTo(newObj);

    return newObj;
}

Ref<CdsObject> ContentManager::createObjectFromFile(String path, bool magic)
{
    String filename = get_filename(path);

    struct stat statbuf;
    int ret;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw Exception(String("Failed to stat ") + path);
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
        mimetype = extension2mimetype(extension);

        if (mimetype == nil && magic)
        {
            if (ignore_unknown_extensions)
                return nil; // item should be ignored
            mimetype = get_mime_type(path);
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
        Ref<MetadataReader> metadataReader(new MetadataReader());
        metadataReader->setMetadata(item);
    }
    else if (S_ISDIR(statbuf.st_mode))
    {
        Ref<CdsContainer> cont(new CdsContainer());
        obj = RefCast(cont, CdsObject);
        cont->setLocation(path);
        Ref<StringConverter> f2i = StringConverter::f2i();
        obj->setTitle(f2i->convert(filename));
    }
    else
    {
        // only regular files and directories are supported
        throw Exception(String("ContentManager: skipping file ") + path.c_str());
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
		printf("ContentManager SCRIPTING: %s\n", e.getMessage().c_str());
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

/* experimental file adding */
/*
void ContentManager::addFile2(String path, int recursive)
{
	initScripting();

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
                printf("file ignored: %s\n", curPath.c_str());
                return;
            }
            obj->setParentID(curParentID);
            addObject(obj);
            um->containerChanged(curParentID);
        }
        curParentID = obj->getID();
    }

	if (IS_CDS_ITEM(obj->getObjectType()))
		scripting->processCdsObject(obj);

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
        throw Exception(String("could not list directory ")+
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
                    printf("file ignored: %s\n", newPath.c_str());
                    return;
                }
                obj->setParentID(parentID);
                addObject(obj);
                um->containerChanged(parentID);
            }
			if (IS_CDS_ITEM(obj->getObjectType()))
			{
				scripting->processCdsObject(obj);
			}
            if (IS_CDS_CONTAINER(obj->getObjectType()))
            {
                addRecursive(newPath, obj->getID());
            }
        }
        catch(Exception e)
        {
            printf("skipping %s : %s\n", newPath.c_str(), e.getMessage().c_str());
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
    dirstack = (dirstack_t *)malloc(sizeof(dirstack_t) * 20);
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
        if(taskQueue->size() == 0)
        {
            /* if nothing to do, sleep until awakened */        
            pthread_cond_wait(&taskCond, &taskMutex);            
            unlock();
            continue;
        }
        else
        {
            task = taskQueue->get(0);
            taskQueue->remove(0, 1);
            currentTask = task; 
        }
        unlock();

        printf("Running asynchronous task: %s\n",
               task->getDescription().c_str());
        try
        {
            task->run();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        printf("Finished asynchronous task\n");
    }
}
void *ContentManager::staticThreadProc(void *arg)
{
    ContentManager *inst = (ContentManager *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}
void ContentManager::lock()
{
    pthread_mutex_lock(&taskMutex);
}
void ContentManager::unlock()
{
    pthread_mutex_unlock(&taskMutex);
}
void ContentManager::signal()
{
    pthread_cond_signal(&taskCond);
}

int ContentManager::addTask(zmm::Ref<CMTask> task)
{
    int ret = false;
    lock();
    int size = taskQueue->size();
    if (size >= 1)
        ret = true;
    taskQueue->append(task);
    signal();
    unlock();
    return ret;
}

/* sync / async methods */
int ContentManager::loadAccounting(int async)
{
    if (async)
    {
        Ref<CMTask> task(new CMLoadAccountingTask());
        task->setDescription("Loading info");
        return addTask(task);
    }
    else
    {
        _loadAccounting();
        return false;
    }
}
int ContentManager::addFile(zmm::String path, int recursive, int async)
{
    if (async)
    {
        Ref<CMTask> task(new CMAddFileTask(path, recursive));
        task->setDescription(String("Adding ") + path);
        return addTask(task);
    }
    else
    {
        _addFile(path, recursive);
        return false;
    }
}
int ContentManager::removeObject(String objectID, int async)
{
    if (async)
    {
        // building container path for the description
        Ref<Storage> storage = Storage::getInstance();
        Ref<Array<CdsObject> > objectPath = storage->getObjectPath(objectID);
        Ref<StringBuffer> desc(new StringBuffer(objectPath->size() * 10));
        *desc << "Removing ";
        for (int i = 0; i < objectPath->size(); i++)
            *desc << '/' << objectPath->get(i)->getTitle();
        
        Ref<CMTask> task(new CMRemoveObjectTask(objectID));
        task->setDescription(desc->toString());
        return addTask(task);
    }
    else
    {
        _removeObject(objectID);
        return false;
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


CMAddFileTask::CMAddFileTask(String path, int recursive) : CMTask()
{
    this->path = path;
    this->recursive = recursive;
}
void CMAddFileTask::run()
{
    cm->_addFile(path, recursive);
}

CMRemoveObjectTask::CMRemoveObjectTask(String objectID) : CMTask()
{
    this->objectID = objectID;
}
void CMRemoveObjectTask::run()
{
    cm->_removeObject(objectID);
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

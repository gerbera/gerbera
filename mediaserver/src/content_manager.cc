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

struct magic_set *ms = NULL;

using namespace zmm;
using namespace mxml;

/*********************** utils ***********************/

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
    
    // cut off everything after first space character
    int pos = mime_type.index(';');
    int pos2 = mime_type.index(' ');
    if (pos2 < pos)
        pos = pos2;
    if (pos >= 0)
        mime_type = mime_type.substring(0, pos);
 
    return mime_type;    
 }

/***************************************************************/

static Ref<ContentManager> instance;

ContentManager::ContentManager() : Object()
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
   
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> mapEl = cm->getElement("/import/mappings/mimetype-upnpclass");
    if (mapEl == nil)
    {
        printf("mimetype-upnpclass mappings not found\n");
    }
    else
    {
        mimetype_upnpclass_map = cm->createDictionaryFromNodeset(mapEl, "map", "from", "to");
    }
}

ContentManager::~ContentManager()
{
    if (ms)
        magic_close(ms);
}

Ref<ContentManager> ContentManager::getInstance()
{
    if (instance == nil)
    {
        instance = Ref<ContentManager>(new ContentManager());
    }
    return instance;
}

void ContentManager::addFile(String path, int recursive)
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
            obj->setParentID(curParentID);
            storage->addObject(obj);
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

void ContentManager::removeObject(String objectID)
{
    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);
    storage->removeObject(objectID);
    Ref<UpdateManager> um = UpdateManager::getInstance();
    um->containerChanged(obj->getParentID());
    um->flushUpdates();
}

void ContentManager::updateObject(String objectID, Ref<Dictionary> parameters)
{
    String title = parameters->get("title");
    String upnp_class = parameters->get("class");
    String autoscan = parameters->get("autoscan");
    String mimetype = parameters->get("mime-type");
    String description = parameters->get("description");
    String location = parameters->get("location");
 
    Ref<Storage> storage = Storage::getInstance();
    Ref<UpdateManager> um = UpdateManager::getInstance();
    
    Ref<CdsObject> obj = storage->loadObject(objectID);
    int objectType = obj->getObjectType();

    /// \TODO: if we have an active item, does it mean we first go through IS_ITEM and then thorugh IS_ACTIVE item? ask Gena
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        Ref<CdsObject> clone = Storage::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);
        if (string_ok(location)) clone->setLocation(location);

        Ref<CdsItem> cloned_item = RefCast(clone, CdsItem);
        
      
        // description can be an empty string - if you want to clear it
        if (description != nil) cloned_item->setDescription(description);
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
        Ref<CdsObject> clone = Storage::createObject(objectType);
        item->copyTo(clone);

        if (string_ok(title)) clone->setTitle(title);
        if (string_ok(upnp_class)) clone->setClass(upnp_class);

        Ref<CdsActiveItem> cloned_item = RefCast(clone, CdsActiveItem);
       
        // state and description can be an empty strings - if you want to clear it
        if (description != nil) cloned_item->setDescription(description);
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
        Ref<CdsObject> clone = Storage::createObject(objectType);
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
    
    Ref<CdsObject> newObj = Storage::createObject(newType);
    
    oldObj->copyTo(newObj);

    return newObj;
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
                obj->setParentID(parentID);
                storage->addObject(obj);				
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
    if (S_ISREG(statbuf.st_mode))
    {
        Ref<CdsItem> item(new CdsItem());
        obj = RefCast(item, CdsObject);
        item->setLocation(path);
        
        if (magic)
        {
            String mt = get_mime_type(path);
            if (mt != nil)
                item->setMimeType(mt);
            String upnp_class = mimetype2upnpclass(mt);
            if (upnp_class != nil)
                item->setClass(upnp_class);
        }
    }
    else if (S_ISDIR(statbuf.st_mode))
    {
        Ref<CdsContainer> cont(new CdsContainer());
        obj = RefCast(cont, CdsObject);
        cont->setLocation(path);
    }
    else
    {
        // only regular files and directories are supported
        throw Exception(String("ContentManager: skipping file ") + path.c_str());
    }
    Ref<StringConverter> f2i = StringConverter::f2i();    
    obj->setTitle(f2i->convert(filename));
    return obj;
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



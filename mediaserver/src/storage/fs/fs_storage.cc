/*  fs_storage.cc - this file is part of MediaTomb.
                                                                                
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

#include <dirent.h>

#include "common.h"
#include "content_manager.h"
#include "fs_storage.h"

using namespace zmm;

FsStorage::FsStorage() : Storage()
{
}

FsStorage::~FsStorage()
{
}

/*
// should optimize this
static int get_child_count(String path)
{
    DIR *dir;
    struct dirent *dent;

    dir = opendir(path.c_str());
    if (! dir)
    {
        throw Exception(String("could not list directory ")+
                        path + " : " + strerror(errno));
    }

    int count = 0;
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
        count++;
        // TODO: skip non-regular files
    }
    closedir(dir);
    return count;
}
*/

Ref<Array<CdsObject> > FsStorage::browse(Ref<BrowseParam> param)
{
    String path = param->getObjectID();

    if (path != "0") path = hex_decode_string(path);

    if (path.charAt(path.length() - 1) == '/')
    {
        path = path.substring(0, path.length() - 1);
    }
    String upnp_object_id = path;
    if (path == "0")
    {
        path = "/";
    }

    if (path.charAt(0) != '/')
    {
        throw StorageException(String("relative paths not allowed: ") + path);
    }

    Ref<ContentManager> cm = ContentManager::getInstance();
    
    int count = 0;

    Ref<Array<CdsObject> > containers(new Array<CdsObject>());
    Ref<Array<CdsObject> > items(new Array<CdsObject>());
   
    Ref<CdsObject> obj = cm->createObjectFromFile(path, false);

    if (param->getFlag() == BROWSE_DIRECT_CHILDREN &&
       IS_CDS_CONTAINER(obj->getObjectType()))
    {
        DIR *dir;
        struct dirent *dent;

        dir = opendir(path.c_str());
        if (! dir)
        {
            throw Exception(String("could not list directory ")+
                            path + " : " + strerror(errno));
        }

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
            String childPath;
            if (path == "/")
                childPath = path + name;
            else
                childPath = path + "/" + name;
            try
            {
                Ref<CdsObject> childObj = cm->createObjectFromFile(childPath, false);
                if (path == "/")
                    childObj->setParentID("0");
                else
                    childObj->setParentID(hex_encode(path.c_str(), path.length()));

                childObj->setID(hex_encode(childPath.c_str(), childPath.length()));
                int objectType = childObj->getObjectType();
                if (IS_CDS_CONTAINER(objectType))
                {
                    containers->append(childObj);
                }
                else if(IS_CDS_ITEM(objectType))
                {
                    items->append(childObj);
                }
            }
            catch(Exception e)
            {
                printf("FsStorage: skipping %s : %s\n", childPath.c_str(), e.getMessage().c_str());
            }
        }
        closedir(dir);

        param->setTotalMatches(containers->size() + items->size());

        count = param->getRequestedCount();
        if(! count)
            count = 1000000000;
    }
    else
    {
        String id, parent_id;
        if(path == "/")
        {
            id = "0";
            parent_id = "-1";
        }
        else
        {
            id = hex_encode(path.c_str(), path.length());
            int pos = path.rindex('/');
            if (pos <= 0)
                parent_id = "0";
            else
                parent_id = hex_encode(path.substring(0, pos).c_str(), path.length());
        }
        obj->setParentID(parent_id);
        obj->setID(id);
        items->append(obj);
        param->setTotalMatches(1);
        return items;
    }


	quicksort((COMPARABLE *)containers->getObjectArray(), containers->size(),
			  CdsObjectTitleComparator);
	quicksort((COMPARABLE *)items->getObjectArray(), items->size(),
			  CdsObjectTitleComparator);
	
    Ref<Array<CdsObject> > arr(new Array<CdsObject>(10));

    // merging containers and items into single array
    int start = param->getStartingIndex();
    int end = start + count;
    int sum_size = containers->size() + items->size();
    int i;
    for (i = start; i < end; i++)
    {
        Ref<CdsObject> object;
        if (i < containers->size())
            object = containers->get(i);
        else if (i < sum_size)
            object = items->get(i - containers->size());
        else
            break;
        arr->append(object);       
    }

    // update childCount fields
    /*
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_CONTAINER(obj->getObjectType()))
        {
            Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
            cont->setChildCount(get_child_count(cont->getID()));
        }
    }
    */

    return arr;
}

void FsStorage::init(Ref<Dictionary> params)
{
}

void FsStorage::addObject(Ref<CdsObject> obj)
{
    throw StorageException("FsStorage: addObject unsupported");
}

void FsStorage::updateObject(zmm::Ref<CdsObject> obj)
{
    throw StorageException("FsStorage: updateObject unsupported");
}

void FsStorage::eraseObject(zmm::Ref<CdsObject> obj)
{
    throw StorageException("FsStorage: eraseObject unsupported");
}

Ref<Array<StringBase> > FsStorage::getMimeTypes()
{
    Ref<Array<StringBase> > arr(new Array<StringBase>());
    return arr;
}

Ref<CdsObject> FsStorage::findObjectByTitle(String title, String parentID)
{
    Ref<ContentManager> cm = ContentManager::getInstance();
    return cm->createObjectFromFile(parentID + "/" + title, false);
}


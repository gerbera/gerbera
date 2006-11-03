/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    storage.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file storage.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "storage.h"
#include "config_manager.h"

#if ! defined(HAVE_MYSQL) && ! defined(HAVE_SQLITE3)
    #error "need at least one storage (mysql or sqlite3)"
#endif

#include "storage/sqlite3/sqlite3_storage.h"
#include "storage/mysql/mysql_storage.h"

#include "tools.h"

using namespace zmm;

static Ref<Storage> primary_inst;

Storage::Storage() : Object()
{
}

static Ref<Storage> create_primary_inst()
{
    String type;
    Ref<Storage> storage;

    Ref<ConfigManager> config = ConfigManager::getInstance();
    type = config->getOption(_("/server/storage/attribute::driver"));

    do
    {
#ifdef HAVE_SQLITE3
    	if (type == "sqlite3")
        {
            storage = Ref<Storage>(new Sqlite3Storage());
            break;
        }
#endif

#ifdef HAVE_MYSQL
        if (type == "mysql")
        {
            storage = Ref<Storage>(new MysqlStorage());
            break;
        }
#endif
        // other database types...
        throw _Exception(_("Unknown storage type: ") + type);
    }
    while (false);
    
    storage->init();
    return storage;
}

Ref<Mutex> Storage::mutex = Ref<Mutex>(new Mutex());

Ref<Storage> Storage::getInstance()
{
    if(primary_inst == nil)
    {
        mutex->lock();
        if (primary_inst == nil)
        {
            try
            {
                primary_inst = create_primary_inst();
            }
            catch (Exception e)
            {
                mutex->unlock();
                throw e;
            }
        }
        mutex->unlock();
    }
    return primary_inst;
}

/*
// this is a fallback implementation! Derived classes should override 
Ref<CdsObject> Storage::loadObject(int objectID, select_mode_t mode)
{
    Ref<BrowseParam> param(new BrowseParam(objectID, BROWSE_METADATA));
    Ref<Array<CdsObject> > arr;
    arr = browse(param);
    Ref<CdsObject> obj = arr->get(0);
    return obj;
}

void Storage::removeObject(zmm::Ref<CdsObject> obj)
{
    if(IS_CDS_CONTAINER(obj->getObjectType()))
    {
        Ref<BrowseParam> param(new BrowseParam(obj->getID(),
                               BROWSE_DIRECT_CHILDREN));
        Ref<Array<CdsObject> > arr = browse(param);
        for(int i = 0; i < arr->size(); i++)
        {
            removeObject(arr->get(i));
        }
    }
    eraseObject(obj);
}
*/

/*
Ref<Array<CdsObject> > Storage::getObjectPath(int objectID)
{
    Ref<Array<CdsObject> > arr(new Array<CdsObject>());
    getObjectPath(arr, objectID);
    return arr;
}
void Storage::getObjectPath(Ref<Array<CdsObject> > arr, int objectID)
{
    if (objectID == -1)
        return;
    Ref<CdsObject> obj = loadObject(objectID);
    getObjectPath(arr, obj->getParentID());
    arr->append(obj);
}
*/

void Storage::stripAndUnescapeVirtualContainerFromPath(String path, String &first, String &last)
{
    if (path.charAt(0) != VIRTUAL_CONTAINER_SEPARATOR)
    {
        throw _Exception(_("got non-absolute virtual path; needs to start with: ") + VIRTUAL_CONTAINER_SEPARATOR);
    }
    int sep = path.rindex(VIRTUAL_CONTAINER_SEPARATOR);
    if (sep == 0)
    {
        first = _("/");
        last = path.substring(1);
    }
    else
    {
        while (sep > 0)
        {
            char beforeSep = path.charAt(sep - 1);
            if (beforeSep != VIRTUAL_CONTAINER_ESCAPE)
            {
                break;
            }
            sep = path.rindex(sep - 1, VIRTUAL_CONTAINER_SEPARATOR);
        }
        first = path.substring(0, sep);
        last = unescape(path.substring(sep + 1), VIRTUAL_CONTAINER_ESCAPE);
    }
}


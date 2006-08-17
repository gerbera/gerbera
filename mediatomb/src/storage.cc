/*  storage.cc - this file is part of MediaTomb.

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

#include "storage.h"
#include "config_manager.h"

#ifdef HAVE_SQLITE3
#include "storage/sqlite3/sqlite3_storage.h"
#endif

#ifdef HAVE_MYSQL
#include "storage/mysql/mysql_storage.h"
#endif

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

Ref<Storage> Storage::getInstance()
{
     if(primary_inst == nil)
         primary_inst = create_primary_inst();
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

void Storage::removeObject(int objectID)
{
    Ref<CdsObject> obj = loadObject(objectID);
    removeObject(obj);
}

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


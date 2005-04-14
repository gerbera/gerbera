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

#include "storage.h"
#include "config_manager.h"

#include "storage/sqlite3/sqlite3_storage.h"
#include "storage/fs/fs_storage.h"

using namespace zmm;

static Ref<Storage> primary_inst;
static Ref<Storage> filesystem_inst;

Storage::Storage() : Object()
{
}

static Ref<Storage> create_primary_inst()
{
    String type;
    Ref<Storage> storage;
   

    Ref<Dictionary> params(new Dictionary());
    Ref<ConfigManager> config = ConfigManager::getInstance();
    type = config->getOption("/server/storage/attribute::driver");
    if (type == "sqlite3")
    {
        /// \TODO check if this is the way to go.. should the drivers get their options
        /// themselves, or should the options be prepared here?
        /// \TODO who should construct the paths??
        params->put("database-file", config->getOption("/server/home") + DIR_SEPARATOR + config->getOption("/server/storage/database-file"));
        storage = Ref<Storage>(new Sqlite3Storage());
        storage->init(params);
        return storage;        
    }
    // other database types...
    else
    {
        throw Exception(String("Unknown storage type: ") + type);
    }
}

Ref<Storage> Storage::getInstance(storage_type_t type)
{
    switch (type)
    {
        case PRIMARY_STORAGE:
            if(primary_inst == nil)
                primary_inst = create_primary_inst();
            return primary_inst;
        case FILESYSTEM_STORAGE:            
            if(filesystem_inst == nil)
            {
                filesystem_inst = Ref<Storage>(new FsStorage());
                filesystem_inst->init(nil);
            }   
            return filesystem_inst;
        default:
            throw Exception(String("Unknown storage type: ") + (int)type);
    }
}

Ref<CdsObject> Storage::createObject(int objectType)
{
    CdsObject *pobj;

    if(IS_CDS_CONTAINER(objectType))
    {
        pobj = new CdsContainer();
    }
    else if(IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        pobj = new CdsItemExternalURL();
    }
    else if(IS_CDS_ACTIVE_ITEM(objectType))
    {
        pobj = new CdsActiveItem();
    }
    else if(IS_CDS_ITEM(objectType))
    {
        pobj = new CdsItem();
    }
    else
    {
        throw StorageException(String("invalid object type :") + objectType);
    }
    return Ref<CdsObject>(pobj);
}

Ref<CdsObject> Storage::loadObject(String objectID)
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
void Storage::removeObject(String objectID)
{
    Ref<CdsObject> obj = loadObject(objectID);
    removeObject(obj);
}

Ref<Array<CdsContainer> > Storage::getContainerPath(String objectID)
{
    Ref<Array<CdsContainer> > arr(new Array<CdsContainer>());
    getContainerPath(arr, objectID);
    return arr;    
}
void Storage::getContainerPath(Ref<Array<CdsContainer> > arr, String objectID)
{
    Ref<CdsContainer> cont = RefCast(loadObject(objectID), CdsContainer);
    if (cont->getParentID() == "-1")
        return arr->append(cont);
    getContainerPath(arr, cont->getParentID());
    arr->append(cont);
}

BrowseParam::BrowseParam(String objectID, int flag) : Object()
{
    this->objectID = objectID;
    this->flag = flag;
    startIndex = 0;
    requestedCount = 0;
}

int BrowseParam::getFlag()
{
    return flag;
}
String BrowseParam::getObjectID()
{
    return objectID;
}

void BrowseParam::setRange(int startIndex, int requestedCount)
{
    this->startIndex = startIndex;
    this->requestedCount = requestedCount;
}
void BrowseParam::setStartingIndex(int startIndex)
{
    this->startIndex = startIndex;
}
void BrowseParam::setRequestedCount(int requestedCount)
{
    this->requestedCount = requestedCount;
}

int BrowseParam::getStartingIndex(){
    return startIndex;
}
int BrowseParam::getRequestedCount(){
    return requestedCount;
}

int BrowseParam::getTotalMatches(){
    return totalMatches;
}
void BrowseParam::setTotalMatches(int x){
    totalMatches = x;
}


/*  sql_storage.h - this file is part of MediaTomb.

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

#ifndef __SQL_STORAGE_H__
#define __SQL_STORAGE_H__

#include "zmmf/zmmf.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "storage.h"
#include "hash.h"
#include "sync.h"

class SQLResult;

class SQLRow : public zmm::Object
{
public:
    SQLRow(zmm::Ref<SQLResult> sqlResult);
    virtual ~SQLRow();
    virtual zmm::String col(int index) = 0;
protected:
    zmm::Ref<SQLResult> sqlResult;
};

class SQLResult : public zmm::Object
{
public:
    SQLResult();
    virtual ~SQLResult();
    virtual zmm::Ref<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() = 0;
};

class SQLStorage : public Storage
{
public:
    SQLStorage();
    virtual ~SQLStorage();
    
    virtual void init();
    
    /* methods to override in subclasses */
    virtual zmm::String quote(zmm::String str) = 0;
    virtual zmm::Ref<SQLResult> select(zmm::String query) = 0;
    virtual int exec(zmm::String query, bool getLastInsertId = false) = 0;
    
    virtual void addObject(zmm::Ref<CdsObject> object);
    virtual void updateObject(zmm::Ref<CdsObject> object);
    
    virtual zmm::Ref<CdsObject> loadObject(int objectID);
    virtual int getChildCount(int contId, bool containersOnly = false);
    
    virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param);
                                         
    virtual int isFolderInDatabase(zmm::String path);
    virtual int isFileInDatabase(int parentID, zmm::String filename);
   
    virtual zmm::Ref<DBRHash<int> > getObjects(int parentID);

    virtual void removeObjects(zmm::Ref<DBRHash<int> > list);

    virtual void removeObject(int objectID);
    
    /* accounting methods */
    virtual int getTotalFiles();   
    
    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param);
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes();
    
    //virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, int parentID);
    virtual zmm::Ref<CdsObject> findObjectByPath(zmm::String fullpath);
    virtual zmm::Ref<CdsObject> findObjectByFilename(zmm::String filename, int parentID);
    virtual int findObjectIDByPath(zmm::String fullpath);
    virtual void incrementUpdateIDs(int *ids, int size);
    virtual void incrementUIUpdateID(int id);
    virtual void shutdown() = 0;
    
protected:
    /* helper forcreateObjectFromRow() */
    zmm::String getRealLocation(int parentID, zmm::String location);
    
    virtual zmm::Ref<CdsObject> createObjectFromRow(zmm::Ref<SQLRow> row);
    
    /* helpers for selectObjects() */
    zmm::String getSelectQuery();
    
    /* helper for findObjectByPath and findObjectIDByPath */ 
    zmm::Ref<SQLRow> _findObjectByPath(zmm::String fullpath);
    
    /* helper for findObjectByFilename and isFileInDatabase */
    zmm::Ref<SQLRow> _findObjectByFilename(zmm::String filename, int parentID);
    
    /* helper for addObject and updateObject */
    zmm::Ref<Dictionary> _addUpdateObject(zmm::Ref<CdsObject> obj, bool isUpdate);
    
    /*
    zmm::String selectQueryBasic;
    zmm::String selectQueryExtended;
    zmm::String selectQueryFull;
    */
    
    /* helpers for removeObject() */
    
    zmm::Ref<Mutex> mutex;
    
    int *rmIDs;
    zmm::Ref<DBHash<int> > rmIDHash;
    int *rmParents;
    zmm::Ref<DBBHash<int, int> > rmParentHash;
    
    bool dbRemovesDeps;
    
    unsigned int stringHash(zmm::String key);
    
    int ensurePathExistence(zmm::String path);
    zmm::Ref<CdsObject> checkRefID(zmm::Ref<CdsObject> obj);
    int createContainer(int parentID, zmm::String name, zmm::String path);
    
    
    /*
    void rmInit();
    void rmCleanup();
    void rmDeleteIDs();
    void rmUpdateParents();
    void rmFlush();
    void rmObject(zmm::Ref<CdsObject> obj);
    void rmItem(zmm::Ref<CdsObject> obj);
    void rmChildren(zmm::Ref<CdsObject> obj);
    void rmDecChildCount(zmm::Ref<CdsObject> obj);
    */
    
    //zmm::Ref<DSOHash<CdsObject> > objectTitleCache;
    //zmm::Ref<DBOHash<int, CdsObject> > objectIDCache;
    
    //int nextObjectID;
};

#endif // __SQL_STORAGE_H__


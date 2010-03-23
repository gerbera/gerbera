/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sql_storage.h - this file is part of MediaTomb.
    
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

/// \file sql_storage.h

#ifndef __SQL_STORAGE_H__
#define __SQL_STORAGE_H__

#include "zmmf/zmmf.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "storage.h"
#include "hash.h"
#include "sync.h"
#include "storage_cache.h"

#define QTB                 table_quote_begin
#define QTE                 table_quote_end

#define CDS_OBJECT_TABLE            "mt_cds_object"
#define CDS_ACTIVE_ITEM_TABLE       "mt_cds_active_item"
#define INTERNAL_SETTINGS_TABLE     "mt_internal_setting"
#define AUTOSCAN_TABLE              "mt_autoscan"

class SQLResult;

class SQLRow : public zmm::Object
{
public:
    SQLRow(zmm::Ref<SQLResult> sqlResult) { this->sqlResult = sqlResult; }
    //virtual ~SQLRow();
    zmm::String col(int index) { return col_c_str(index); }
    virtual char* col_c_str(int index) = 0;
protected:
    zmm::Ref<SQLResult> sqlResult;
};

class SQLResult : public zmm::Object
{
public:
    //SQLResult();
    //virtual ~SQLResult();
    virtual zmm::Ref<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() = 0;
};

class SQLStorage : protected Storage
{
public:
    /* methods to override in subclasses */
    virtual zmm::String quote(zmm::String str) = 0;
    virtual zmm::String quote(int val) = 0;
    virtual zmm::String quote(unsigned int val) = 0;
    virtual zmm::String quote(long val) = 0;
    virtual zmm::String quote(unsigned long val) = 0;
    virtual zmm::String quote(bool val) = 0;
    virtual zmm::String quote(char val) = 0;
    virtual zmm::Ref<SQLResult> select(const char *query, int length) = 0;
    virtual int exec(const char *query, int length, bool getLastInsertId = false) = 0;
    
    void dbReady();
    
    /* wrapper functions for select and exec */
    zmm::Ref<SQLResult> select(zmm::Ref<zmm::StringBuffer> buf)
        { return select(buf->c_str(), buf->length()); }
    int exec(zmm::Ref<zmm::StringBuffer> buf, bool getLastInsertId = false)
        { return exec(buf->c_str(), buf->length(), getLastInsertId); }
    
    virtual void addObject(zmm::Ref<CdsObject> object, int *changedContainer);
    virtual void updateObject(zmm::Ref<CdsObject> object, int *changedContainer);
    
    virtual zmm::Ref<CdsObject> loadObject(int objectID);
    virtual int getChildCount(int contId, bool containers, bool items, bool hideFsRoot);
    
    //virtual zmm::Ref<zmm::Array<CdsObject> > selectObjects(zmm::Ref<SelectParam> param);
    
    virtual zmm::Ref<DBRHash<int> > getObjects(int parentID, bool withoutContainer);
    
    virtual zmm::Ref<ChangedContainers> removeObject(int objectID, bool all);
    virtual zmm::Ref<ChangedContainers> removeObjects(zmm::Ref<DBRHash<int> > list, bool all = false);
    
    virtual zmm::Ref<CdsObject> loadObjectByServiceID(zmm::String serviceID);
    virtual zmm::Ref<zmm::IntArray> getServiceObjectIDs(char servicePrefix);
    
    /* accounting methods */
    virtual int getTotalFiles();
    
    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param);
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes();
    
    //virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, int parentID);
    virtual zmm::Ref<CdsObject> findObjectByPath(zmm::String fullpath);
    virtual int findObjectIDByPath(zmm::String fullpath);
    virtual zmm::String incrementUpdateIDs(int *ids, int size);
    
    virtual zmm::String buildContainerPath(int parentID, zmm::String title);
    virtual void addContainerChain(zmm::String path, zmm::String lastClass,
            int lastRefID, int *containerID, int *updateID);
    virtual zmm::String getInternalSetting(zmm::String key);
    virtual void storeInternalSetting(zmm::String key, zmm::String value) = 0;
    
    virtual void updateAutoscanPersistentList(scan_mode_t scanmode, zmm::Ref<AutoscanList> list);
    virtual zmm::Ref<AutoscanList> getAutoscanList(scan_mode_t scanmode);
    virtual void addAutoscanDirectory(zmm::Ref<AutoscanDirectory> adir);
    virtual void updateAutoscanDirectory(zmm::Ref<AutoscanDirectory> adir);
    virtual void removeAutoscanDirectoryByObjectID(int objectID);
    virtual void removeAutoscanDirectory(int autoscanID);
    virtual int getAutoscanDirectoryType(int objectId);
    virtual int isAutoscanDirectoryRecursive(int objectId);
    virtual void autoscanUpdateLM(zmm::Ref<AutoscanDirectory> adir);
    virtual zmm::Ref<AutoscanDirectory> getAutoscanDirectory(int objectID);
    virtual int isAutoscanChild(int objectID);
    virtual void checkOverlappingAutoscans(zmm::Ref<AutoscanDirectory> adir);
    
    virtual zmm::Ref<zmm::IntArray> getPathIDs(int objectID);
    
    virtual void shutdown();
    virtual void shutdownDriver() = 0;
    
    virtual int ensurePathExistence(zmm::String path, int *changedContainer);
    
    virtual zmm::String getFsRootName();
    
    virtual void clearFlagInDB(int flag);

protected:
    SQLStorage();
    //virtual ~SQLStorage();
    virtual void init();
    
    char table_quote_begin;
    char table_quote_end;
    
private:
    
    class ChangedContainersStr : public Object
    {
    public:
        ChangedContainersStr()
        {
            upnp = zmm::Ref<zmm::StringBuffer>(new zmm::StringBuffer());
            ui = zmm::Ref<zmm::StringBuffer>(new zmm::StringBuffer());
        }
        zmm::Ref<zmm::StringBuffer> upnp;
        zmm::Ref<zmm::StringBuffer> ui;
    };
    
    zmm::String sql_query;
    
    /* helper for createObjectFromRow() */
    zmm::String getRealLocation(int parentID, zmm::String location);
    
    zmm::Ref<CdsObject> createObjectFromRow(zmm::Ref<SQLRow> row);
    
    /* helper for findObjectByPath and findObjectIDByPath */ 
    zmm::Ref<CdsObject> _findObjectByPath(zmm::String fullpath);
    
    int _ensurePathExistence(zmm::String path, int *changedContainer);
    
    /* helper class and helper function for addObject and updateObject */
    class AddUpdateTable : public Object
    {
    public:
        AddUpdateTable(zmm::String table, zmm::Ref<Dictionary> dict)
        {
            this->table = table;
            this->dict = dict;
        }
        zmm::String getTable() { return table; }
        zmm::Ref<Dictionary> getDict() { return dict; }
    protected:
        zmm::String table;
        zmm::Ref<Dictionary> dict;
    };
    zmm::Ref<zmm::Array<AddUpdateTable> > _addUpdateObject(zmm::Ref<CdsObject> obj, bool isUpdate, int *changedContainer);
    
    /* helper for removeObject(s) */
    void _removeObjects(zmm::Ref<zmm::StringBuffer> objectIDs, int offset);
    zmm::Ref<ChangedContainersStr> _recursiveRemove(zmm::Ref<zmm::StringBuffer> items, zmm::Ref<zmm::StringBuffer> containers, bool all);
    
    virtual zmm::Ref<ChangedContainers> _purgeEmptyContainers(zmm::Ref<ChangedContainersStr> changedContainersStr);
    
    /* helpers for autoscan */
    int _getAutoscanObjectID(int autoscanID);
    void _autoscanChangePersistentFlag(int objectID, bool persistent);
    zmm::Ref<AutoscanDirectory> _fillAutoscanDirectory(zmm::Ref<SQLRow> row);
    int _getAutoscanDirectoryInfo(int objectID, zmm::String field);
    zmm::Ref<zmm::IntArray> _checkOverlappingAutoscans(zmm::Ref<AutoscanDirectory> adir);
    
    /* location hash helpers */
    zmm::String addLocationPrefix(char prefix, zmm::String path);
    zmm::String stripLocationPrefix(char* prefix, zmm::String path);
    zmm::String stripLocationPrefix(zmm::String path);
    
    zmm::Ref<CdsObject> checkRefID(zmm::Ref<CdsObject> obj);
    int createContainer(int parentID, zmm::String name, zmm::String path, bool isVirtual, zmm::String upnpClass, int refID);
    
    zmm::String mapBool(bool val) { return quote((val ? 1 : 0)); }
    bool remapBool(zmm::String field) { return (string_ok(field) && field == "1"); }
    
    void setFsRootName(zmm::String rootName = nil);
    
    zmm::String fsRootName;
    
    int lastID;
    
    int getNextID();
    void loadLastID();
    zmm::Ref<Mutex> nextIDMutex;
    
    zmm::Ref<StorageCache> cache;
    inline bool cacheOn() { return cache != nil; }
    void addObjectToCache(zmm::Ref<CdsObject> object, bool dontLock = false);
    
    inline bool doInsertBuffering() { return insertBufferOn; }
    void addToInsertBuffer(zmm::Ref<zmm::StringBuffer> query);
    void flushInsertBuffer(bool dontLock = false);
    
    /* insert buffer functions to be overriden by implementing classes */
    virtual void _addToInsertBuffer(zmm::Ref<zmm::StringBuffer> query) = 0;
    virtual void _flushInsertBuffer() = 0;
    
    bool insertBufferOn;
    bool insertBufferEmpty;
    int insertBufferStatementCount;
    int insertBufferByteCount;
    zmm::Ref<Mutex> insertBufferMutex;
};

#endif // __SQL_STORAGE_H__

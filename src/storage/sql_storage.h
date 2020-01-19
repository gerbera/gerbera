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

#include "cds_objects.h"
#include "storage.h"

#include <unordered_set>
#include <mutex>
#include <sstream>

#define QTB                 table_quote_begin
#define QTE                 table_quote_end

#define CDS_OBJECT_TABLE            "mt_cds_object"
#define CDS_ACTIVE_ITEM_TABLE       "mt_cds_active_item"
#define INTERNAL_SETTINGS_TABLE     "mt_internal_setting"
#define AUTOSCAN_TABLE              "mt_autoscan"
#define METADATA_TABLE              "mt_metadata"

class SQLResult;
class SQLEmitter;

class SQLRow
{
public:
    SQLRow(std::shared_ptr<SQLResult> sqlResult) { this->sqlResult = sqlResult; }
    //virtual ~SQLRow();
    std::string col(int index)
    {
        char* c = col_c_str(index);
        if (c == 0) return "";
        return std::string(c);
    }
    virtual char* col_c_str(int index) = 0;
protected:
    std::shared_ptr<SQLResult> sqlResult;
};

class SQLResult : public std::enable_shared_from_this<SQLResult>
{
public:
    //SQLResult();
    //virtual ~SQLResult();
    virtual std::unique_ptr<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() = 0;
};

class SQLStorage : public Storage
{
public:
    /* methods to override in subclasses */
    virtual std::string quote(std::string str) = 0;
    virtual std::string quote(const char* str) = 0;
    virtual std::string quote(int val) = 0;
    virtual std::string quote(unsigned int val) = 0;
    virtual std::string quote(long val) = 0;
    virtual std::string quote(unsigned long val) = 0;
    virtual std::string quote(bool val) = 0;
    virtual std::string quote(char val) = 0;
    virtual std::string quote(long long val) = 0;
    virtual std::shared_ptr<SQLResult> select(const char *query, int length) = 0;
    virtual int exec(const char *query, int length, bool getLastInsertId = false) = 0;
    
    void dbReady();
    
    /* wrapper functions for select and exec */
    std::shared_ptr<SQLResult> select(const std::string &buf)
        { return select(buf.c_str(), buf.length()); }
    std::shared_ptr<SQLResult> select(const std::ostringstream &buf) {
        auto s = buf.str();
        return select(s.c_str(), s.length());
    }
    int exec(const std::ostringstream &buf, bool getLastInsertId = false) {
        auto s = buf.str();
        return exec(s.c_str(), s.length(), getLastInsertId);
    }
    
    virtual void addObject(std::shared_ptr<CdsObject> object, int *changedContainer) override;
    virtual void updateObject(std::shared_ptr<CdsObject> object, int *changedContainer) override;
    
    virtual std::shared_ptr<CdsObject> loadObject(int objectID) override;
    virtual int getChildCount(int contId, bool containers, bool items, bool hideFsRoot) override;
    
    virtual std::unique_ptr<std::unordered_set<int>> getObjects(int parentID, bool withoutContainer) override;
    
    virtual std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) override;
    virtual std::unique_ptr<ChangedContainers> removeObjects(const std::unique_ptr<std::unordered_set<int>>& list, bool all = false) override;
    
    virtual std::shared_ptr<CdsObject> loadObjectByServiceID(std::string serviceID) override;
    virtual std::unique_ptr<std::vector<int>> getServiceObjectIDs(char servicePrefix) override;

    virtual std::string findFolderImage(int id, std::string trackArtBase) override;
    
    /* accounting methods */
    virtual int getTotalFiles() override;
    
    virtual std::vector<std::shared_ptr<CdsObject>> browse(const std::unique_ptr<BrowseParam>& param) override;
    virtual std::vector<std::shared_ptr<CdsObject>> search(const std::unique_ptr<SearchParam>& param, int* numMatches) override;
    
    virtual std::vector<std::string> getMimeTypes() override;
    
    //virtual std::shared_ptr<CdsObject> findObjectByTitle(std::string title, int parentID);
    virtual std::shared_ptr<CdsObject> findObjectByPath(std::string fullpath) override;
    virtual int findObjectIDByPath(std::string fullpath) override;
    virtual std::string incrementUpdateIDs(const std::unique_ptr<std::unordered_set<int>>& ids) override;

    virtual std::string buildContainerPath(int parentID, std::string title) override;
    virtual void addContainerChain(std::string path, std::string lastClass, int lastRefID, int *containerID, int *updateID, const std::map<std::string,std::string>& lastMetadata) override;
    virtual std::string getInternalSetting(std::string key) override;
    virtual void storeInternalSetting(std::string key, std::string value) override = 0;
    
    virtual void updateAutoscanPersistentList(ScanMode scanmode, std::shared_ptr<AutoscanList> list) override;
    virtual std::shared_ptr<AutoscanList> getAutoscanList(ScanMode scanmode) override;
    virtual void addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override;
    virtual void updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override;
    virtual void removeAutoscanDirectoryByObjectID(int objectID) override;
    virtual void removeAutoscanDirectory(int autoscanID) override;
    virtual int getAutoscanDirectoryType(int objectId) override;
    virtual int isAutoscanDirectoryRecursive(int objectId) override;
    virtual void autoscanUpdateLM(std::shared_ptr<AutoscanDirectory> adir) override;
    virtual std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) override;
    virtual int isAutoscanChild(int objectID) override;
    virtual void checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir) override;
    
    virtual std::unique_ptr<std::vector<int>> getPathIDs(int objectID) override;
    
    virtual void shutdown();
    virtual void shutdownDriver() = 0;
    
    virtual int ensurePathExistence(std::string path, int *changedContainer) override;
    
    virtual std::string getFsRootName() override;
    
    virtual void clearFlagInDB(int flag) override;

protected:
    SQLStorage(std::shared_ptr<ConfigManager> config);
    //virtual ~SQLStorage();
    virtual void init();

    void doMetadataMigration() override;
    void migrateMetadata(std::shared_ptr<CdsObject> object);
    
    char table_quote_begin;
    char table_quote_end;
    
private:
    std::string sql_query;
    
    /* helper for createObjectFromRow() */
    std::string getRealLocation(int parentID, std::string location);
    
    std::shared_ptr<CdsObject> createObjectFromRow(const std::unique_ptr<SQLRow>& row);
    std::shared_ptr<CdsObject> createObjectFromSearchRow(const std::unique_ptr<SQLRow>& row);
    std::map<std::string,std::string> retrieveMetadataForObject(int objectId);
    
    /* helper for findObjectByPath and findObjectIDByPath */ 
    std::shared_ptr<CdsObject> _findObjectByPath(std::string fullpath);
    
    int _ensurePathExistence(std::string path, int *changedContainer);
    
    /* helper class and helper function for addObject and updateObject */
    class AddUpdateTable
    {
    public:
        AddUpdateTable(std::string table, std::map<std::string,std::string> dict, std::string operation)
        {
            this->table = table;
            this->dict = dict;
            this->operation = operation;
        }
        std::string getTable() { return table; }
        std::map<std::string,std::string> getDict() { return dict; }
        std::string getOperation() { return operation; }
    protected:
        std::string table;
        std::map<std::string,std::string> dict;
        std::string operation;
    };
    std::vector<std::shared_ptr<AddUpdateTable>> _addUpdateObject(std::shared_ptr<CdsObject> obj, bool isUpdate, int *changedContainer);

    void generateMetadataDBOperations(std::shared_ptr<CdsObject> obj, bool isUpdate,
        std::vector<std::shared_ptr<AddUpdateTable>>& operations);

    std::unique_ptr<std::ostringstream> sqlForInsert(std::shared_ptr<CdsObject> obj, std::shared_ptr<AddUpdateTable> addUpdateTable);
    std::unique_ptr<std::ostringstream> sqlForUpdate(std::shared_ptr<CdsObject> obj, std::shared_ptr<AddUpdateTable> addUpdateTable);
    std::unique_ptr<std::ostringstream> sqlForDelete(std::shared_ptr<CdsObject> obj, std::shared_ptr<AddUpdateTable> addUpdateTable);
    
    /* helper for removeObject(s) */
    void _removeObjects(const std::vector<int32_t> &objectIDs);

    std::string toCSV(const std::vector<int>& input);

    std::unique_ptr<ChangedContainers> _recursiveRemove(
        const std::vector<int32_t> &items,
        const std::vector<int32_t> &containers, bool all);
    
    virtual std::unique_ptr<ChangedContainers> _purgeEmptyContainers(std::unique_ptr<ChangedContainers>& maybeEmpty);
    
    /* helpers for autoscan */
    int _getAutoscanObjectID(int autoscanID);
    void _autoscanChangePersistentFlag(int objectID, bool persistent);
    std::shared_ptr<AutoscanDirectory> _fillAutoscanDirectory(const std::unique_ptr<SQLRow>& row);
    int _getAutoscanDirectoryInfo(int objectID, std::string field);
    std::unique_ptr<std::vector<int>> _checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir);
    
    /* location hash helpers */
    std::string addLocationPrefix(char prefix, std::string path);
    std::string stripLocationPrefix(char* prefix, std::string path);
    std::string stripLocationPrefix(std::string path);
    
    std::shared_ptr<CdsObject> checkRefID(std::shared_ptr<CdsObject> obj);
    int createContainer(int parentID, std::string name, std::string path, bool isVirtual, std::string upnpClass, int refID, const std::map<std::string,std::string>& lastMetadata);

    std::string mapBool(bool val) { return quote((val ? 1 : 0)); }
    bool remapBool(std::string field) { return (string_ok(field) && field == "1"); }
    
    void setFsRootName(std::string rootName = "");
    
    std::string fsRootName;
    
    int lastID;
    
    int getNextID();
    void loadLastID();

    int lastMetadataID;

    int getNextMetadataID();
    void loadLastMetadataID();

    std::shared_ptr<SQLEmitter> sqlEmitter;

    std::mutex nextIDMutex;
    using AutoLock = std::lock_guard<std::mutex>;
};

#endif // __SQL_STORAGE_H__

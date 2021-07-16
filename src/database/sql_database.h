/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sql_database.h - this file is part of MediaTomb.
    
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

/// \file sql_database.h

#ifndef __SQL_STORAGE_H__
#define __SQL_STORAGE_H__

#include <array>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "config/config.h"
#include "database.h"

// forward declarations
class CdsContainer;
class CdsResource;
class SQLResult;
class SQLEmitter;

#define DBVERSION 13

#define QTB table_quote_begin
#define QTE table_quote_end

#define CDS_OBJECT_TABLE "mt_cds_object"
#define INTERNAL_SETTINGS_TABLE "mt_internal_setting"
#define AUTOSCAN_TABLE "mt_autoscan"
#define METADATA_TABLE "mt_metadata"
#define RESOURCE_TABLE "grb_cds_resource"
#define CONFIG_VALUE_TABLE "grb_config_value"

class SQLRow {
public:
    virtual ~SQLRow() = default;
    std::string col(int index) const
    {
        char* c = col_c_str(index);
        if (!c)
            return "";
        return std::string(c);
    }
    virtual char* col_c_str(int index) const = 0;
};

class SQLResult {
public:
    //SQLResult();
    virtual ~SQLResult() = default;
    virtual std::unique_ptr<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() const = 0;
};

class SQLDatabase : public Database {
public:
    /* methods to override in subclasses */
    virtual std::string quote(std::string_view str) const = 0;
    virtual std::string quote(std::string str) const = 0;
    virtual std::string quote(const char* str) const = 0;
    virtual std::string quote(int val) const = 0;
    virtual std::string quote(unsigned int val) const = 0;
    virtual std::string quote(long val) const = 0;
    virtual std::string quote(unsigned long val) const = 0;
    virtual std::string quote(bool val) const = 0;
    virtual std::string quote(char val) const = 0;
    virtual std::string quote(long long val) const = 0;

    virtual void beginTransaction(const std::string_view& tName) = 0;
    virtual void rollback(const std::string_view& tName) = 0;
    virtual void commit(const std::string_view& tName) = 0;

    virtual std::shared_ptr<SQLResult> select(const char* query, size_t length) = 0;
    virtual int exec(const char* query, size_t length, bool getLastInsertId = false) = 0;

    /* wrapper functions for select and exec */
    std::shared_ptr<SQLResult> select(const std::string& buf)
    {
        return select(buf.c_str(), buf.length());
    }
    std::shared_ptr<SQLResult> select(const std::ostringstream& buf)
    {
        auto s = buf.str();
        return select(s.c_str(), s.length());
    }
    int exec(const std::string& query, bool getLastInsertId = false)
    {
        return exec(query.c_str(), query.length(), getLastInsertId);
    }

    void addObject(std::shared_ptr<CdsObject> object, int* changedContainer) override;
    void updateObject(std::shared_ptr<CdsObject> object, int* changedContainer) override;

    std::shared_ptr<CdsObject> loadObject(int objectID) override;
    int getChildCount(int contId, bool containers, bool items, bool hideFsRoot) override;

    std::unique_ptr<std::unordered_set<int>> getObjects(int parentID, bool withoutContainer) override;

    std::unique_ptr<ChangedContainers> removeObject(int objectID, bool all) override;
    std::unique_ptr<ChangedContainers> removeObjects(const std::unique_ptr<std::unordered_set<int>>& list, bool all = false) override;

    std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID) override;
    std::unique_ptr<std::vector<int>> getServiceObjectIDs(char servicePrefix) override;

    /* accounting methods */
    int getTotalFiles(bool isVirtual = false, const std::string& mimeType = "", const std::string& upnpClass = "") override;

    std::vector<std::shared_ptr<CdsObject>> browse(const std::unique_ptr<BrowseParam>& param) override;
    std::vector<std::shared_ptr<CdsObject>> search(const std::unique_ptr<SearchParam>& param, int* numMatches) override;

    std::vector<std::string> getMimeTypes() override;

    //virtual std::shared_ptr<CdsObject> findObjectByTitle(std::string title, int parentID);
    std::shared_ptr<CdsObject> findObjectByPath(fs::path fullpath, bool wasRegularFile = false) override;
    int findObjectIDByPath(fs::path fullpath, bool wasRegularFile = false) override;
    std::string incrementUpdateIDs(const std::unique_ptr<std::unordered_set<int>>& ids) override;

    fs::path buildContainerPath(int parentID, const std::string& title) override;
    void addContainerChain(std::string path, const std::string& lastClass, int lastRefID, int* containerID, std::vector<int>& updateID, const std::map<std::string, std::string>& lastMetadata) override;
    std::string getInternalSetting(const std::string& key) override;
    void storeInternalSetting(const std::string& key, const std::string& value) override = 0;

    std::shared_ptr<AutoscanList> getAutoscanList(ScanMode scanmode) override;
    void updateAutoscanList(ScanMode scanmode, std::shared_ptr<AutoscanList> list) override;

    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) override;
    void addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override;
    void updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override;
    void removeAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) override;
    void checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir) override;

    /* config methods */
    std::vector<ConfigValue> getConfigValues() override;
    void removeConfigValue(const std::string& item) override;
    void updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status = "unchanged") override;

    std::unique_ptr<std::vector<int>> getPathIDs(int objectID) override;

    void shutdown() override;
    virtual void shutdownDriver() = 0;

    int ensurePathExistence(fs::path path, int* changedContainer) override;

    std::string getFsRootName() override;
    static std::string getSortCapabilities();
    static std::string getSearchCapabilities();

    void clearFlagInDB(int flag) override;
    unsigned int getHash(size_t index) { return index < DBVERSION ? hashies[index] : 0; }

protected:
    explicit SQLDatabase(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime);
    void init() override;

    /// \brief migrate metadata from mt_cds_objects to mt_metadata before removing the column (DBVERSION 12)
    bool doMetadataMigration();
    void migrateMetadata(int objectId, const std::string& metadataStr);

    /// \brief Add a column to resource table for each defined resource attribute
    void prepareResourceTable(std::string_view addColumnCmd);

    /// \brief migrate resources from mt_cds_objects to grb_resource before removing the column (DBVERSION 13)
    bool doResourceMigration();
    void migrateResources(int objectId, const std::string& resourcesStr);

    std::shared_ptr<Mime> mime;

    char table_quote_begin { '\0' };
    char table_quote_end { '\0' };
    bool use_transaction;
    bool inTransaction {};
    std::array<unsigned int, DBVERSION> hashies;

    std::recursive_mutex sqlMutex;
    using SqlAutoLock = std::lock_guard<decltype(sqlMutex)>;
    std::map<int, std::shared_ptr<CdsContainer>> dynamicContainers;

    void upgradeDatabase(std::string& dbVersion, const std::array<unsigned int, DBVERSION>& hashies, config_option_t upgradeOption, std::string_view updateVersionCommand, std::string_view addResourceColumnCmd);
    virtual void _exec(const char* query, int length = -1) = 0;

private:
    std::string sql_browse_query;
    std::string sql_search_columns;
    std::string sql_search_query;
    std::string sql_meta_query;
    std::string sql_resource_query;
    std::string addResourceColumnCmd;

    std::shared_ptr<CdsObject> createObjectFromRow(const std::unique_ptr<SQLRow>& row);
    std::shared_ptr<CdsObject> createObjectFromSearchRow(const std::unique_ptr<SQLRow>& row);
    std::map<std::string, std::string> retrieveMetadataForObject(int objectId);
    std::vector<std::shared_ptr<CdsResource>> retrieveResourcesForObject(int objectId);

    enum class Operation {
        Insert,
        Update,
        Delete
    };

    /* helper class and helper function for addObject and updateObject */
    class AddUpdateTable {
    public:
        AddUpdateTable(std::string tableName, std::map<std::string, std::string> dict, Operation operation)
            : tableName(std::move(tableName))
            , dict(std::move(dict))
            , operation(operation)
        {
        }
        [[nodiscard]] std::string getTableName() const { return tableName; }
        [[nodiscard]] std::map<std::string, std::string> getDict() const { return dict; }
        [[nodiscard]] Operation getOperation() const { return operation; }

    protected:
        std::string tableName;
        std::map<std::string, std::string> dict;
        Operation operation;
    };
    std::vector<std::shared_ptr<AddUpdateTable>> _addUpdateObject(const std::shared_ptr<CdsObject>& obj, Operation op, int* changedContainer);

    void generateMetadataDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op,
        std::vector<std::shared_ptr<AddUpdateTable>>& operations);

    void generateResourceDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op,
        std::vector<std::shared_ptr<AddUpdateTable>>& operations);

    std::unique_ptr<std::ostringstream> sqlForInsert(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const;
    std::unique_ptr<std::ostringstream> sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const;
    std::unique_ptr<std::ostringstream> sqlForDelete(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const;

    /* helper for removeObject(s) */
    void _removeObjects(const std::vector<int32_t>& objectIDs);

    static std::string toCSV(const std::vector<int>& input);

    std::unique_ptr<ChangedContainers> _recursiveRemove(
        const std::vector<int32_t>& items,
        const std::vector<int32_t>& containers, bool all);

    virtual std::unique_ptr<ChangedContainers> _purgeEmptyContainers(const std::unique_ptr<ChangedContainers>& maybeEmpty);

    /* helpers for autoscan */
    void _removeAutoscanDirectory(int autoscanID);
    int _getAutoscanObjectID(int autoscanID);
    void _autoscanChangePersistentFlag(int objectID, bool persistent);
    static std::shared_ptr<AutoscanDirectory> _fillAutoscanDirectory(const std::unique_ptr<SQLRow>& row);
    std::unique_ptr<std::vector<int>> _checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir);

    /* location helper: filesystem path or virtual path to db location*/
    static std::string addLocationPrefix(char prefix, const fs::path& path);
    /* location helpers: db location to filesystem path */
    static fs::path stripLocationPrefix(std::string_view dbLocation, char* prefix = nullptr);

    std::shared_ptr<CdsObject> checkRefID(const std::shared_ptr<CdsObject>& obj);
    int createContainer(int parentID, std::string name, const std::string& virtualPath, bool isVirtual, const std::string& upnpClass, int refID, const std::map<std::string, std::string>& itemMetadata);

    std::string mapBool(bool val) const { return quote((val ? 1 : 0)); }
    static bool remapBool(const std::string& field) { return field == "1"; }

    void setFsRootName(const std::string& rootName = "");

    std::string fsRootName;
    std::shared_ptr<SQLEmitter> sqlEmitter;

    using AutoLock = std::lock_guard<std::mutex>;
};

#endif // __SQL_STORAGE_H__

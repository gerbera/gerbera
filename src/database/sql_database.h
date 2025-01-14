/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sql_database.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "config/config.h"
#include "config/config_val.h"
#include "database.h"
#include "sql_format.h"

#include <array>
#include <mutex>
#include <unordered_set>
#include <utility>

// forward declarations
class CdsContainer;
class CdsResource;
class SQLResult;
class SQLEmitter;

#define DBVERSION 23

#define CDS_OBJECT_TABLE "mt_cds_object"
#define INTERNAL_SETTINGS_TABLE "mt_internal_setting"
#define AUTOSCAN_TABLE "mt_autoscan"
#define METADATA_TABLE "mt_metadata"
#define RESOURCE_TABLE "grb_cds_resource"
#define CONFIG_VALUE_TABLE "grb_config_value"
#define CLIENTS_TABLE "grb_client"
#define PLAYSTATUS_TABLE "grb_playstatus"

class SQLRow {
public:
    virtual ~SQLRow() = default;
    /// \brief Returns true if the column index contains the value NULL
    bool isNullOrEmpty(int index) const
    {
        const char* c = col_c_str(index);
        return c == nullptr || *c == '\0';
    }
    /// \brief Return the value of column index as a string value
    std::string col(int index) const
    {
        const char* c = col_c_str(index);
        if (!c)
            return {};
        return { c };
    }
    /// \brief Return the value of column index as an integer value
    int col_int(int index, int null_value) const
    {
        const char* c = col_c_str(index);
        if (!c || *c == '\0')
            return null_value;
        return std::atoi(c);
    }
    /// \brief Return the value of column index as an integer value
    long long col_long(int index, long long null_value) const
    {
        const char* c = col_c_str(index);
        if (!c || *c == '\0')
            return null_value;
        return std::atoll(c);
    }
    virtual char* col_c_str(int index) const = 0;
};

class SQLResult {
public:
    virtual ~SQLResult() = default;
    virtual std::unique_ptr<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() const = 0;
};

class SQLDatabase : public Database {
public:
    /* methods to override in subclasses */
    virtual std::string quote(const std::string& str) const = 0;
    // required to handle #defines
    std::string quote(const char* str) const { return quote(std::string(str)); }
    /* wrapper functions for different types */
    std::string quote(char val) const { return quote(fmt::to_string(val)); }
    static std::string quote(int val) { return fmt::to_string(val); }
    static std::string quote(unsigned int val) { return fmt::to_string(val); }
    static std::string quote(long val) { return fmt::to_string(val); }
    static std::string quote(unsigned long val) { return fmt::to_string(val); }
    static std::string quote(bool val) { return val ? "1" : "0"; }
    static std::string quote(long long val) { return fmt::to_string(val); }

    // hooks for transactions
    virtual void beginTransaction(std::string_view tName) { }
    virtual void rollback(std::string_view tName) { }
    virtual void commit(std::string_view tName) { }

    virtual void del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids) = 0;
    virtual void exec(std::string_view tableName, const std::string& query, int objId) = 0;
    virtual int exec(const std::string& query, bool getLastInsertId = false) = 0;
    virtual void execOnly(const std::string& query) = 0;
    virtual std::shared_ptr<SQLResult> select(const std::string& query) = 0;

    void addObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer) override;
    void updateObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer) override;

    std::shared_ptr<CdsObject> loadObject(int objectID) override;
    std::shared_ptr<CdsObject> loadObject(const std::string& group, int objectID) override;
    int getChildCount(int contId, bool containers, bool items, bool hideFsRoot) override;
    std::map<int, int> getChildCounts(const std::vector<int>& contId, bool containers, bool items, bool hideFsRoot) override;

    std::size_t getObjects(int parentID, bool withoutContainer, std::unordered_set<int>& ret, bool full) override;
    std::vector<int> getRefObjects(int objectId) override;
    std::unordered_set<int> getUnreferencedObjects() override;

    std::unique_ptr<ChangedContainers> removeObject(int objectID, const fs::path& path, bool all) override;
    std::unique_ptr<ChangedContainers> removeObjects(const std::unordered_set<int>& list, bool all = false) override;

    std::shared_ptr<CdsObject> loadObjectByServiceID(const std::string& serviceID, const std::string& group) override;
    std::vector<int> getServiceObjectIDs(char servicePrefix) override;

    /* accounting methods */
    long long getFileStats(const StatsParam& stats) override;
    std::map<std::string, long long> getGroupStats(const StatsParam& stats) override;

    std::vector<std::shared_ptr<CdsObject>> browse(BrowseParam& param) override;
    std::vector<std::shared_ptr<CdsObject>> search(SearchParam& param) override;

    std::vector<std::string> getMimeTypes() override;
    std::map<std::string, std::shared_ptr<CdsContainer>> getShortcuts() override;

    std::vector<std::shared_ptr<CdsObject>> findObjectByContentClass(const std::string& contentClass, const std::string& group) override;
    std::shared_ptr<CdsObject> findObjectByPath(const fs::path& fullpath, const std::string& group, DbFileType fileType = DbFileType::Auto) override;
    int findObjectIDByPath(const fs::path& fullpath, DbFileType fileType = DbFileType::Auto) override;
    std::string incrementUpdateIDs(const std::unordered_set<int>& ids) override;

    fs::path buildContainerPath(int parentID, const std::string& title) override;
    bool addContainer(int parentContainerId, std::string virtualPath, const std::shared_ptr<CdsContainer>& cont, int* containerID) override;
    std::string getInternalSetting(const std::string& key) override;
    void storeInternalSetting(const std::string& key, const std::string& value) override = 0;

    std::shared_ptr<AutoscanList> getAutoscanList(AutoscanScanMode scanmode) override;
    void updateAutoscanList(AutoscanScanMode scanmode, const std::shared_ptr<AutoscanList>& list) override;

    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory(int objectID) override;
    void addAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override;
    void updateAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override;
    void removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir) override;
    void checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir) override;

    /* config methods */
    std::vector<ConfigValue> getConfigValues() override;
    void removeConfigValue(const std::string& item) override;
    void updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status = "unchanged") override;

    /* clients methods */
    std::vector<ClientObservation> getClients() override;
    void saveClients(const std::vector<ClientObservation>& cache) override;
    std::shared_ptr<ClientStatusDetail> getPlayStatus(const std::string& group, int objectId) override;
    std::vector<std::shared_ptr<ClientStatusDetail>> getPlayStatusList(int objectId) override;
    void savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail) override;
    std::vector<std::map<std::string, std::string>> getClientGroupStats() override;

    std::vector<int> getPathIDs(int objectID) override;

    void shutdown() override;
    virtual void shutdownDriver() = 0;

    int ensurePathExistence(const fs::path& path, int* changedContainer) override;

    static std::string getSortCapabilities();
    static std::string getSearchCapabilities();

    unsigned int getHash(std::size_t index) const { return index < DBVERSION ? hashies.at(index) : 0; }

    int insert(std::string_view tableName, const std::vector<SQLIdentifier>& fields, const std::vector<std::string>& values, bool getLastInsertId = false, bool warnOnly = false);
    void insertMultipleRows(std::string_view tableName, const std::vector<SQLIdentifier>& fields, const std::vector<std::vector<std::string>>& valuesets);
    template <typename T>
    void updateRow(std::string_view tableName, const std::vector<ColumnUpdate>& values, std::string_view key, const T& value);
    void deleteAll(std::string_view tableName);
    template <typename T>
    void deleteRow(const std::string& tableName, const std::string& key, const T& value);
    void deleteRows(std::string_view tableName, const std::string& key, const std::vector<int>& values);

protected:
    explicit SQLDatabase(const std::shared_ptr<Config>& config, std::shared_ptr<Mime> mime, std::shared_ptr<ConverterManager> converterManager);
    void init() override;
    void initDynContainers(const std::shared_ptr<CdsObject>& sParent = {});

    /// \brief migrate metadata from mt_cds_objects to mt_metadata before removing the column (DBVERSION 12)
    bool doMetadataMigration();
    void migrateMetadata(int objectId, const std::string& metadataStr);

    /// \brief Add a column to resource table for each defined resource attribute
    void prepareResourceTable(std::string_view addColumnCmd);

    /// \brief migrate resources from mt_cds_objects to grb_resource before removing the column (DBVERSION 13)
    bool doResourceMigration();
    void migrateResources(int objectId, const std::string& resourcesStr);

    /// \brief returns a fmt-printable identifier name
    SQLIdentifier identifier(const std::string& name) const { return { name, table_quote_begin, table_quote_end }; }

    std::shared_ptr<Mime> mime;
    std::shared_ptr<ConverterManager> converterManager;

    char table_quote_begin { '\0' };
    char table_quote_end { '\0' };
    std::array<unsigned int, DBVERSION> hashies;

    mutable std::recursive_mutex sqlMutex;
    using SqlAutoLock = std::scoped_lock<decltype(sqlMutex)>;
    std::map<int, std::shared_ptr<CdsContainer>> dynamicContainers;

    void upgradeDatabase(unsigned int dbVersion, const std::array<unsigned int, DBVERSION>& hashies, ConfigVal upgradeOption, std::string_view updateVersionCommand, std::string_view addResourceColumnCmd);
    virtual void _exec(const std::string& query) = 0;

private:
    std::string sql_browse_columns;
    std::string sql_browse_query;
    std::string sql_search_columns;
    std::string sql_search_container_query_format;
    std::string sql_search_query;
    std::string sql_meta_query;
    std::string sql_autoscan_query;
    std::string sql_resource_query;
    std::string addResourceColumnCmd;
    /// \brief List of column names to be used in insert and update to ensure correct order of columns
    // only columns listed here are added to the insert and update statements
    std::map<std::string, std::vector<std::string>> tableColumnOrder;

    /// \brief Configuration content for dynamic folders
    std::shared_ptr<DynamicContentList> dynamicContentList;
    bool dynamicContentEnabled;

    std::shared_ptr<CdsObject> createObjectFromRow(const std::string& group, const std::unique_ptr<SQLRow>& row);
    std::shared_ptr<CdsObject> createObjectFromSearchRow(const std::string& group, const std::unique_ptr<SQLRow>& row);
    std::vector<std::pair<std::string, std::string>> retrieveMetaDataForObject(int objectId);
    std::vector<std::shared_ptr<CdsResource>> retrieveResourcesForObject(int objectId);

    enum class Operation {
        Insert,
        Update,
        Delete
    };

    /* helper class and helper function for addObject and updateObject */
    class AddUpdateTable {
    public:
        AddUpdateTable(std::string&& tableName, std::map<std::string, std::string>&& dict, Operation operation) noexcept
            : tableName(std::move(tableName))
            , dict(std::move(dict))
            , operation(operation)
        {
        }
        ~AddUpdateTable() = default;
        [[nodiscard]] const std::string& getTableName() const noexcept { return tableName; }
        [[nodiscard]] const std::map<std::string, std::string>& getDict() const noexcept { return dict; }
        [[nodiscard]] Operation getOperation() const noexcept { return operation; }

    protected:
        std::string tableName;
        std::map<std::string, std::string> dict;
        Operation operation;
    };
    std::vector<AddUpdateTable> _addUpdateObject(const std::shared_ptr<CdsObject>& obj, Operation op, int* changedContainer);

    void generateMetaDataDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op, std::vector<AddUpdateTable>& operations) const;
    void generateResourceDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op, std::vector<AddUpdateTable>& operations);

    std::string sqlForInsert(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const;
    std::string sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const;
    std::string sqlForDelete(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const;

    /* helper for removeObject(s) */
    void _removeObjects(const std::vector<std::int32_t>& objectIDs);

    ChangedContainers _recursiveRemove(
        const std::vector<std::int32_t>& items,
        const std::vector<std::int32_t>& containers, bool all);

    virtual std::unique_ptr<ChangedContainers> _purgeEmptyContainers(ChangedContainers maybeEmpty);

    /* helpers for autoscan */
    void _removeAutoscanDirectory(int autoscanID);
    int _getAutoscanObjectID(int autoscanID);
    void _autoscanChangePersistentFlag(int objectID, bool persistent);
    static std::shared_ptr<AutoscanDirectory> _fillAutoscanDirectory(const std::unique_ptr<SQLRow>& row);
    std::vector<int> _checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir);

    /* location helper: filesystem path or virtual path to db location*/
    static std::string addLocationPrefix(char prefix, const fs::path& path, const std::string_view& suffix = "");
    /* location helpers: db location to filesystem path */
    static std::pair<fs::path, char> stripLocationPrefix(std::string_view dbLocation);

    std::shared_ptr<CdsObject> checkRefID(const std::shared_ptr<CdsObject>& obj);
    int createContainer(int parentID, const std::string& name, const std::string& virtualPath, int flags, bool isVirtual, const std::string& upnpClass, int refID,
        const std::vector<std::pair<std::string, std::string>>& itemMetadata, const std::vector<std::shared_ptr<CdsResource>>& itemResources = {});

    static bool remapBool(const std::string& field) { return field == "1"; }
    static bool remapBool(int field) { return field == 1; }

    std::shared_ptr<SQLEmitter> sqlEmitter;

    using AutoLock = std::scoped_lock<std::mutex>;
};

template <typename T>
void SQLDatabase::updateRow(std::string_view tableName, const std::vector<ColumnUpdate>& values, std::string_view key, const T& value)
{
    exec(fmt::format("UPDATE {} SET {} WHERE {} = {}", identifier(std::string(tableName)), fmt::join(values, ", "), identifier(std::string(key)), quote(value)));
}

template <typename T>
void SQLDatabase::deleteRow(const std::string& tableName, const std::string& key, const T& value)
{
    exec(fmt::format("DELETE FROM {} WHERE {} = {}", identifier(tableName), identifier(key), quote(value)));
}

class SqlWithTransactions {
protected:
    explicit SqlWithTransactions(const std::shared_ptr<Config>& config)
        : use_transaction(config->getBoolOption(ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS))
    {
    }

    bool use_transaction;
    bool inTransaction {};
};

#endif // __SQL_STORAGE_H__

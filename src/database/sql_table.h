/*GRB*
  Gerbera - https://gerbera.io/

  sql_table.h - this file is part of Gerbera.

  Copyright (C) 2025 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

/// \file sql_table.h

#ifndef __SQL_TABLE_H__
#define __SQL_TABLE_H__

#include "search_handler.h"

#include "exceptions.h"

#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class AutoscanDirectory;
class CdsObject;
class ClientObservation;
class ConfigValue;
enum class ResourceAttribute;

#define AUTOSCAN_TABLE "mt_autoscan"
#define CLIENTS_TABLE "grb_client"
#define CDS_OBJECT_TABLE "mt_cds_object"
#define CONFIG_VALUE_TABLE "grb_config_value"
#define METADATA_TABLE "mt_metadata"
#define RESOURCE_TABLE "grb_cds_resource"

#define SQL_NULL "NULL"

/// \brief type of database operation
enum class Operation {
    Insert,
    InsertMulti,
    Update,
    Delete,
};

/// \brief metadata column ids
enum class MetadataCol {
    ItemId = 0,
    PropertyName,
    PropertyValue,
};

/// \brief resource column ids
enum class ResourceCol {
    ItemId = 0,
    ResId,
    HandlerType,
    Purpose,
    Options,
    Parameters,
    Attributes, // index of first attribute
};

/// \brief browse column ids
/// enum for createObjectFromRow's mode parameter
enum class BrowseCol {
    Id = 0,
    RefId,
    ParentId,
    ObjectType,
    UpnpClass,
    DcTitle,
    SortKey,
    Location,
    LocationHash,
    Auxdata,
    UpdateId,
    MimeType,
    Flags,
    PartNumber,
    TrackNumber,
    ServiceId,
    LastModified,
    LastUpdated,
    RefUpnpClass,
    RefLocation,
    RefAuxdata,
    RefMimeType,
    RefServiceId,
    AsPersistent
};

/// \brief autoscan column ids
enum class AutoscanColumn {
    Id = 0,
    ObjId,
    ScanMode,
    Recursive,
    MediaType,
    CtAudio,
    CtImage,
    CtVideo,
    Hidden,
    FollowSymlinks,
    Interval,
    LastModified,
    Persistent,
    Location,
    RetryCount,
    DirTypes,
    ForceRescan,
    ObjLocation,
    PathIds,
    Touched,
};

/// \brief configvalue column ids
enum class ConfigColumn {
    Key,
    Item,
    ItemValue,
    Status,
};

/// \brief client column ids
enum class ClientColumn {
    Addr,
    Port,
    AddrFamily,
    UserAgent,
    Last,
    Age,
};

/// \brief base helper class for insert, update and delete operations
template <class Item>
class AddUpdateTable {
public:
    AddUpdateTable(
        std::string&& tableName,
        Operation operation) noexcept
        : tableName(std::move(tableName))
        , operation(operation)
    {
    }
    virtual ~AddUpdateTable() = default;
    /// \brief does the insert statement return an object id to retrieve
    virtual bool hasInsertResult() { return false; }
    /// \brief get the table name
    [[nodiscard]] const std::string& getTableName() const noexcept { return tableName; }
    /// \brief get the required table operation
    [[nodiscard]] Operation getOperation() const noexcept { return operation; }
    /// \brief Generate INSERT statement from row data and object
    virtual std::string sqlForInsert(
        const std::shared_ptr<Item>& obj) const
        = 0;
    /// \brief Generate INSERT statement from row data and object
    virtual std::string sqlForMultiInsert(
        const std::shared_ptr<Item>& obj) const
        = 0;
    /// \brief Generate UPDATE statement from row data and object
    virtual std::string sqlForUpdate(
        const std::shared_ptr<Item>& obj) const
        = 0;
    /// \brief Generate DELETE statement from row data and object
    virtual std::string sqlForDelete(
        const std::shared_ptr<Item>& obj) const
        = 0;

protected:
    std::string tableName;
    Operation operation;
};

/// \brief base template class for insert, update and delete operations
template <class Tab, class Item>
class TableAdaptor : public AddUpdateTable<Item> {
public:
    TableAdaptor(
        std::string&& tableName,
        std::map<Tab, std::string>&& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<Tab>> columnMapper) noexcept
        : AddUpdateTable<Item>(std::move(tableName), operation)
        , rowData(std::move(dict))
        , columnMapper(std::move(columnMapper))
    {
    }
    TableAdaptor(
        std::string&& tableName,
        std::vector<std::map<Tab, std::string>>&& dict,
        std::shared_ptr<EnumColumnMapper<Tab>> columnMapper) noexcept
        : AddUpdateTable<Item>(std::move(tableName), Operation::InsertMulti)
        , rowMultiData(std::move(dict))
        , columnMapper(std::move(columnMapper))
    {
    }
    std::string sqlForInsert(
        const std::shared_ptr<Item>& obj) const override;
    std::string sqlForMultiInsert(
        const std::shared_ptr<Item>& obj) const override;
    std::string sqlForUpdate(
        const std::shared_ptr<Item>& obj) const override;
    std::string sqlForDelete(
        const std::shared_ptr<Item>& obj) const override;

protected:
    /// \brief content to store in request
    std::map<Tab, std::string> rowData;
    /// \brief content to store in request
    std::vector<std::map<Tab, std::string>> rowMultiData;
    /// \brief mapper to convert enum member to column name
    std::shared_ptr<EnumColumnMapper<Tab>> columnMapper;

    /// \brief check whether row data is valid for insert and update operations
    virtual bool isValid() const { return true; }

    /// \brief List of column names to be used in insert and update to ensure correct order of columns
    /// only columns listed here are added to the insert and update statements
    virtual const std::vector<Tab>& getTableColumnOrder() const = 0;
    /// \brief Generate WHERE clause for statements from row data and object
    virtual std::vector<std::string> getWhere(
        const std::shared_ptr<Item>& obj) const
        = 0;
};

/// \brief Adaptor for operations on mt_cds_objects Table
class Object2Table : public TableAdaptor<BrowseCol, CdsObject> {
public:
    Object2Table(
        std::map<BrowseCol, std::string>&& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<BrowseCol>> columnMapper) noexcept
        : TableAdaptor(CDS_OBJECT_TABLE, std::move(dict), operation, std::move(columnMapper))
    {
    }
    bool hasInsertResult() override { return true; }

protected:
    const std::vector<BrowseCol>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<CdsObject>& obj) const override;

private:
    static const std::vector<BrowseCol> tableColumnOrder;
};

/// \brief Adaptor for operations on mt_metadata Table
class Metadata2Table : public TableAdaptor<MetadataCol, CdsObject> {
public:
    Metadata2Table(
        std::map<MetadataCol, std::string>&& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<MetadataCol>> columnMapper) noexcept
        : TableAdaptor(METADATA_TABLE, std::move(dict), operation, std::move(columnMapper))
    {
    }
    Metadata2Table(
        std::vector<std::map<MetadataCol, std::string>>&& dict,
        std::shared_ptr<EnumColumnMapper<MetadataCol>> columnMapper) noexcept
        : TableAdaptor(METADATA_TABLE, std::move(dict), std::move(columnMapper))
    {
    }

protected:
    bool isValid() const override { return rowData.size() == 2 || (operation == Operation::Delete && rowData.empty()); }
    const std::vector<MetadataCol>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<CdsObject>& obj) const override;

private:
    static const std::vector<MetadataCol> tableColumnOrder;
};

/// \brief Adaptor for operations on grb_cds_resource Table
class Resource2Table : public TableAdaptor<ResourceCol, CdsObject> {
public:
    Resource2Table(
        std::map<ResourceCol, std::string>&& dict,
        std::map<ResourceAttribute, std::string>&& resDict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<ResourceCol>> columnMapper) noexcept
        : TableAdaptor(RESOURCE_TABLE, std::move(dict), operation, std::move(columnMapper))
        , resDict(std::move(resDict))
    {
    }
    std::string sqlForInsert(
        const std::shared_ptr<CdsObject>& obj) const override;
    std::string sqlForUpdate(
        const std::shared_ptr<CdsObject>& obj) const override;

protected:
    const std::vector<ResourceCol>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<CdsObject>& obj) const override;

private:
    static const std::vector<ResourceCol> tableColumnOrder;
    std::map<ResourceAttribute, std::string> resDict;
};

/// \brief Adaptor for operations on mt_autoscan Table
class Autoscan2Table : public TableAdaptor<AutoscanColumn, AutoscanDirectory> {
public:
    Autoscan2Table(
        std::map<AutoscanColumn, std::string>& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<AutoscanColumn>> columnMapper) noexcept
        : TableAdaptor(AUTOSCAN_TABLE, std::move(dict), operation, std::move(columnMapper))
    {
    }

protected:
    const std::vector<AutoscanColumn>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<AutoscanDirectory>& obj) const override;

private:
    static const std::vector<AutoscanColumn> tableColumnOrder;
};

/// \brief Adaptor for operations on grb_config_value Table
class Config2Table : public TableAdaptor<ConfigColumn, ConfigValue> {
public:
    Config2Table(
        std::map<ConfigColumn, std::string>&& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<ConfigColumn>> columnMapper) noexcept
        : TableAdaptor(CONFIG_VALUE_TABLE, std::move(dict), operation, std::move(columnMapper))
    {
    }

protected:
    const std::vector<ConfigColumn>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<ConfigValue>& obj) const override;

private:
    static const std::vector<ConfigColumn> tableColumnOrder;
};

/// \brief Adaptor for operations on grb_client Table
class Client2Table : public TableAdaptor<ClientColumn, ClientObservation> {
public:
    Client2Table(
        std::map<ClientColumn, std::string>&& dict,
        Operation operation,
        std::shared_ptr<EnumColumnMapper<ClientColumn>> columnMapper) noexcept
        : TableAdaptor(CLIENTS_TABLE, std::move(dict), operation, std::move(columnMapper))
    {
    }
    Client2Table(
        std::vector<std::map<ClientColumn, std::string>>&& dict,
        std::shared_ptr<EnumColumnMapper<ClientColumn>> columnMapper) noexcept
        : TableAdaptor(CLIENTS_TABLE, std::move(dict), std::move(columnMapper))
    {
    }

protected:
    const std::vector<ClientColumn>& getTableColumnOrder() const override
    {
        return tableColumnOrder;
    }
    std::vector<std::string> getWhere(
        const std::shared_ptr<ClientObservation>& obj) const override;

private:
    static const std::vector<ClientColumn> tableColumnOrder;
};

template <class Tab, class Item>
std::string TableAdaptor<Tab, Item>::sqlForInsert(
    const std::shared_ptr<Item>& obj) const
{
    std::vector<std::string> fields;
    std::vector<std::string> values;
    fields.reserve(rowData.size());
    values.reserve(rowData.size());

    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(columnMapper->mapQuoted(field, true));
            values.push_back(rowData.at(field));
        }
    }

    return this->operation == Operation::Insert //
        ? fmt::format("INSERT INTO {} ({}) VALUES ({})", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(values, ", "))
        : "";
}

template <class Tab, class Item>
std::string TableAdaptor<Tab, Item>::sqlForMultiInsert(
    const std::shared_ptr<Item>& obj) const
{
    std::vector<std::string> fields;
    std::vector<Tab> activeFields;
    std::vector<std::string> tuples;
    fields.reserve(getTableColumnOrder().size());
    tuples.reserve(rowMultiData.size());

    // get columns in right order
    for (auto&& field : getTableColumnOrder()) {
        for (auto&& row : rowMultiData) {
            if (row.find(field) != row.end()) {
                fields.push_back(columnMapper->mapQuoted(field, true));
                activeFields.push_back(field);
            }
        }
    }
    // get values for columns
    for (auto&& row : rowMultiData) {
        std::vector<std::string> values(activeFields.size());
        std::size_t index = 0;
        for (auto&& field : activeFields) {
            if (row.find(field) != row.end()) {
                values[index] = row.at(field);
            } else {
                values[index] = SQL_NULL;
            }
            index++;
        }
        tuples.push_back(fmt::format("({})", fmt::join(values, ",")));
    }

    return this->operation == Operation::InsertMulti //
        ? fmt::format("INSERT INTO {} ({}) VALUES {}", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(tuples, ", "))
        : "";
}

template <class Tab, class Item>
std::string TableAdaptor<Tab, Item>::sqlForUpdate(
    const std::shared_ptr<Item>& obj) const
{
    if (!isValid())
        throw DatabaseException("sqlForUpdate called with invalid arguments", LINE_MESSAGE);

    std::vector<std::string> fields;
    fields.reserve(rowData.size());
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(fmt::format("{} = {}", columnMapper->mapQuoted(field, true), rowData.at(field)));
        }
    }

    return fmt::format("UPDATE {} SET {} WHERE {}", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(getWhere(obj), " AND "));
}

template <class Tab, class Item>
std::string TableAdaptor<Tab, Item>::sqlForDelete(
    const std::shared_ptr<Item>& obj) const
{
    if (!isValid())
        throw DatabaseException("sqlForDelete called with invalid arguments", LINE_MESSAGE);

    return fmt::format("{}", fmt::join(getWhere(obj), " AND "));
}
#endif

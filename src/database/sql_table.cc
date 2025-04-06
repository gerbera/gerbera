/*GRB*
  Gerbera - https://gerbera.io/

  sql_table.cc - this file is part of Gerbera.

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

/// \file sql_table.cc

#include "sql_table.h" // API

#include "cds/cds_objects.h"
#include "sql_database.h"

const std::vector<BrowseCol> Object2Table::tableColumnOrder = {
    BrowseCol::RefId,
    BrowseCol::ParentId,
    BrowseCol::ObjectType,
    BrowseCol::UpnpClass,
    BrowseCol::DcTitle,
    BrowseCol::SortKey,
    BrowseCol::Location,
    BrowseCol::LocationHash,
    BrowseCol::Auxdata,
    BrowseCol::UpdateId,
    BrowseCol::MimeType,
    BrowseCol::Flags,
    BrowseCol::PartNumber,
    BrowseCol::TrackNumber,
    BrowseCol::ServiceId,
    BrowseCol::LastModified,
    BrowseCol::LastUpdated,
};

const std::vector<MetadataCol> Metadata2Table::tableColumnOrder = {
    MetadataCol::ItemId,
    MetadataCol::PropertyName,
    MetadataCol::PropertyValue,
};

const std::vector<ResourceCol> Resource2Table::tableColumnOrder = {
    ResourceCol::ItemId,
    ResourceCol::ResId,
    ResourceCol::ResId,
    ResourceCol::HandlerType,
    ResourceCol::Purpose,
    ResourceCol::Options,
    ResourceCol::Parameters,
};

template <>
std::string TableAdaptor<BrowseCol>::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    if (obj->getID() != INVALID_OBJECT_ID) {
        throw DatabaseException("Attempted to insert new object with ID!", LINE_MESSAGE);
    }

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

    return fmt::format("INSERT INTO {} ({}) VALUES ({})", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(values, ", "));
}

template <>
std::string TableAdaptor<MetadataCol>::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    std::vector<std::string> fields;
    std::vector<std::string> values;
    fields.reserve(rowData.size() + 1);
    values.reserve(rowData.size() + 1);

    fields.push_back(columnMapper->mapQuoted(MetadataCol::ItemId, true));
    values.push_back(fmt::to_string(obj->getID()));
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(columnMapper->mapQuoted(field, true));
            values.push_back(rowData.at(field));
        }
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(values, ", "));
}

std::string Resource2Table::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    std::vector<std::string> fields;
    std::vector<std::string> values;
    fields.reserve(rowData.size() + resDict.size() + 1);
    values.reserve(rowData.size() + resDict.size() + 1);

    fields.push_back(columnMapper->mapQuoted(ResourceCol::ItemId, true));
    values.push_back(fmt::to_string(obj->getID()));
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(columnMapper->mapQuoted(field, true));
            values.push_back(rowData.at(field));
        }
    }
    for (auto&& [key, val] : resDict) {
        fields.push_back(EnumMapper::getAttributeName(key));
        values.push_back(val);
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(values, ", "));
}

std::string Resource2Table::sqlForUpdate(
    const std::shared_ptr<CdsObject>& obj) const
{
    if (!isValid())
        throw DatabaseException("sqlForUpdate called with invalid arguments", LINE_MESSAGE);

    std::vector<std::string> fields;
    fields.reserve(rowData.size() + resDict.size());
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(fmt::format("{} = {}", columnMapper->mapQuoted(field, true), rowData.at(field)));
        }
    }
    for (auto&& [key, val] : resDict) {
        fields.push_back(fmt::format("{} = {}", EnumMapper::getAttributeName(key), val));
    }

    return fmt::format("UPDATE {} SET {} WHERE {}", columnMapper->getTableName(), fmt::join(fields, ", "), fmt::join(getWhere(obj), " AND "));
}

std::vector<std::string> Object2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    return { fmt::format("{} = {}", columnMapper->mapQuoted(BrowseCol::Id), obj->getID()) };
}

std::vector<std::string> Metadata2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    if (!rowData.empty()) {
        return {
            fmt::format("{} = {}", columnMapper->mapQuoted(MetadataCol::ItemId, true), obj->getID()),
            fmt::format("{} = {}", columnMapper->mapQuoted(MetadataCol::PropertyName, true), rowData.begin()->second),
        };
    }
    return { fmt::format("{} = {}", columnMapper->mapQuoted(MetadataCol::ItemId, true), obj->getID()) };
}

std::vector<std::string> Resource2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    return {
        fmt::format("{} = {}", columnMapper->mapQuoted(ResourceCol::ItemId, true), obj->getID()),
        fmt::format("{} = {}", columnMapper->mapQuoted(ResourceCol::ResId, true), rowData.at(ResourceCol::ResId)),
    };
}

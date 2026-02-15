/*GRB*
  Gerbera - https://gerbera.io/

  sql_table.cc - this file is part of Gerbera.

  Copyright (C) 2025-2026 Gerbera Contributors

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

/// @file database/sql_table.cc
#define GRB_LOG_FAC GrbLogFacility::sqldatabase

#include "sql_table.h" // API

#include "cds/cds_objects.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "upnp/clients.h"
#include "util/grb_net.h"

const std::vector<BrowseColumn> Object2Table::tableColumnOrder = {
    BrowseColumn::Id,
    BrowseColumn::RefId,
    BrowseColumn::ParentId,
    BrowseColumn::ObjectType,
    BrowseColumn::UpnpClass,
    BrowseColumn::DcTitle,
    BrowseColumn::SortKey,
    BrowseColumn::Location,
    BrowseColumn::LocationHash,
    BrowseColumn::Auxdata,
    BrowseColumn::UpdateId,
    BrowseColumn::MimeType,
    BrowseColumn::Flags,
    BrowseColumn::PartNumber,
    BrowseColumn::TrackNumber,
    BrowseColumn::Source,
    BrowseColumn::EntryType,
    BrowseColumn::ServiceId,
    BrowseColumn::LastModified,
    BrowseColumn::LastUpdated,
};

const std::vector<MetadataColumn> Metadata2Table::tableColumnOrder = {
    MetadataColumn::ItemId,
    MetadataColumn::PropertyName,
    MetadataColumn::PropertyValue,
};

const std::vector<ResourceColumn> Resource2Table::tableColumnOrder = {
    ResourceColumn::ItemId,
    ResourceColumn::ResId,
    ResourceColumn::HandlerType,
    ResourceColumn::Purpose,
    ResourceColumn::Options,
    ResourceColumn::Parameters,
};

const std::vector<AutoscanColumn> Autoscan2Table::tableColumnOrder = {
    AutoscanColumn::Id,
    AutoscanColumn::ObjId,
    AutoscanColumn::ScanMode,
    AutoscanColumn::Recursive,
    AutoscanColumn::MediaType,
    AutoscanColumn::CtAudio,
    AutoscanColumn::CtImage,
    AutoscanColumn::CtVideo,
    AutoscanColumn::Hidden,
    AutoscanColumn::FollowSymlinks,
    AutoscanColumn::Interval,
    AutoscanColumn::LastModified,
    AutoscanColumn::Persistent,
    AutoscanColumn::Location,
    AutoscanColumn::PathIds,
    AutoscanColumn::Touched,
    AutoscanColumn::RetryCount,
    AutoscanColumn::DirTypes,
    AutoscanColumn::ForceRescan,
    AutoscanColumn::ItemId,
    AutoscanColumn::ObjLocation,
    AutoscanColumn::EntryType,
};

const std::vector<ConfigColumn> Config2Table::tableColumnOrder = {
    ConfigColumn::Key,
    ConfigColumn::Item,
    ConfigColumn::ItemValue,
    ConfigColumn::Status,
};

const std::vector<ClientColumn> Client2Table::tableColumnOrder = {
    ClientColumn::Addr,
    ClientColumn::Port,
    ClientColumn::AddrFamily,
    ClientColumn::UserAgent,
    ClientColumn::Last,
    ClientColumn::Age,
};

const std::vector<PlaystatusColumn> Playstatus2Table::tableColumnOrder = {
    PlaystatusColumn::Group,
    PlaystatusColumn::ItemId,
    PlaystatusColumn::PlayCount,
    PlaystatusColumn::LastPlayed,
    PlaystatusColumn::LastPlayedPosition,
    PlaystatusColumn::BookMarkPosition,
};

/// @brief Generate INSERT statement from row data and object for object tables
template <>
std::string TableAdaptor<BrowseColumn, CdsObject>::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    if (obj && obj->getID() != INVALID_OBJECT_ID) {
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

    return fmt::format("INSERT INTO {} ({}) VALUES ({})",
        columnMapper->getTableName(),
        fmt::join(fields, ", "),
        fmt::join(values, ", "));
}

/// @brief Generate INSERT statement from row data and object for metadata tables
template <>
std::string TableAdaptor<MetadataColumn, CdsObject>::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    std::vector<std::string> fields;
    std::vector<std::string> values;
    fields.reserve(rowData.size() + (obj ? 1 : 0));
    values.reserve(rowData.size() + (obj ? 1 : 0));

    if (obj) {
        fields.push_back(columnMapper->mapQuoted(MetadataColumn::ItemId, true));
        values.push_back(fmt::to_string(obj->getID()));
    }
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(columnMapper->mapQuoted(field, true));
            values.push_back(rowData.at(field));
        }
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})",
        columnMapper->getTableName(),
        fmt::join(fields, ", "),
        fmt::join(values, ", "));
}

std::string Resource2Table::sqlForInsert(
    const std::shared_ptr<CdsObject>& obj) const
{
    std::vector<std::string> fields;
    std::vector<std::string> values;
    fields.reserve(rowData.size() + resDict.size() + (obj ? 1 : 0));
    values.reserve(rowData.size() + resDict.size() + (obj ? 1 : 0));

    if (obj) {
        fields.push_back(columnMapper->mapQuoted(ResourceColumn::ItemId, true));
        values.push_back(fmt::to_string(obj->getID()));
    }
    for (auto&& field : getTableColumnOrder()) {
        if (rowData.find(field) != rowData.end()) {
            fields.push_back(columnMapper->mapQuoted(field, true));
            values.push_back(rowData.at(field));
        }
    }
    for (auto&& [key, val] : resDict) {
        fields.push_back(columnMapper->quote(EnumMapper::getAttributeName(key)));
        values.push_back(val);
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})",
        columnMapper->getTableName(),
        fmt::join(fields, ", "),
        fmt::join(values, ", "));
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
        fields.push_back(fmt::format("{} = {}", columnMapper->quote(EnumMapper::getAttributeName(key)), val));
    }

    return fmt::format("UPDATE {} SET {} WHERE {}",
        columnMapper->getTableName(),
        fmt::join(fields, ", "),
        fmt::join(getWhere(obj), " AND "));
}

std::vector<std::string> Object2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    return obj
        ? std::vector<std::string> { columnMapper->getClause(BrowseColumn::Id, obj->getID(), true) }
        : std::vector<std::string> { columnMapper->getClause(BrowseColumn::Id, rowData.at(BrowseColumn::Id), true) };
}

std::vector<std::string> Metadata2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    if (!rowData.empty()) {
        return {
            columnMapper->getClause(MetadataColumn::ItemId, obj->getID(), true),
            columnMapper->getClause(MetadataColumn::PropertyName, rowData.begin()->second, true),
        };
    }
    return { columnMapper->getClause(MetadataColumn::ItemId, obj->getID(), true) };
}

std::vector<std::string> Resource2Table::getWhere(
    const std::shared_ptr<CdsObject>& obj) const
{
    return {
        columnMapper->getClause(ResourceColumn::ItemId, obj ? fmt::to_string(obj->getID()) : rowData.at(ResourceColumn::ItemId), true),
        columnMapper->getClause(ResourceColumn::ResId, rowData.at(ResourceColumn::ResId), true),
    };
}

std::vector<std::string> Autoscan2Table::getWhere(
    const std::shared_ptr<AutoscanDirectory>& obj) const
{
    return {
        columnMapper->getClause(AutoscanColumn::Id,
            obj ? fmt::to_string(obj->getDatabaseID()) : rowData.at(AutoscanColumn::Id), true),
    };
}

std::vector<std::string> Config2Table::getWhere(
    const std::shared_ptr<ConfigValue>& obj) const
{
    return {
        columnMapper->getClause(ConfigColumn::Item, obj ? obj->item : rowData.at(ConfigColumn::Item), true),
    };
}

std::vector<std::string> Client2Table::getWhere(
    const std::shared_ptr<ClientObservation>& obj) const
{
    return {
        columnMapper->getClause(ClientColumn::Addr, rowData.at(ClientColumn::Addr), true),
        columnMapper->getClause(ClientColumn::Port, obj && obj->addr ? fmt::to_string(obj->addr->getPort()) : rowData.at(ClientColumn::Port), true),
    };
}

std::vector<std::string> Playstatus2Table::getWhere(
    const std::shared_ptr<ClientStatusDetail>& obj) const
{
    return {
        columnMapper->getClause(PlaystatusColumn::Group, rowData.at(PlaystatusColumn::Group), true),
        columnMapper->getClause(PlaystatusColumn::ItemId, obj ? fmt::to_string(obj->getItemId()) : rowData.at(PlaystatusColumn::ItemId), true),
    };
}

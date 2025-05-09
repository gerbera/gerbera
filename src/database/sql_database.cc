/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sql_database.cc - this file is part of MediaTomb.

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

/// \file sql_database.cc
#define GRB_LOG_FAC GrbLogFacility::sqldatabase

#include "sql_database.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "config/result/dynamic_content.h"
#include "content/autoscan_list.h"
#include "db_param.h"
#include "exceptions.h"
#include "metadata/metadata_enums.h"
#include "search_handler.h"
#include "sql_table.h"
#include "upnp/clients.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <algorithm>
#include <fmt/chrono.h>
#include <vector>

#define MAX_REMOVE_SIZE 1000
#define MAX_REMOVE_RECURSION 500

#define RESOURCE_SEP '|'

#define AUS_ALIAS "as"
#define CFG_ALIAS "co"
#define CLT_ALIAS "cl"
#define ITM_ALIAS "f"
#define MTA_ALIAS "m"
#define PLY_ALIAS "pl"
#define REF_ALIAS "rf"
#define RES_ALIAS "re"
#define SRC_ALIAS "c"

// database
#define LOC_DIR_PREFIX 'D'
#define LOC_FILE_PREFIX 'F'
#define LOC_VIRT_PREFIX 'V'
#define LOC_ILLEGAL_PREFIX 'X'

/// \brief search column ids
enum class SearchColumn {
    Id = 0,
    RefId,
    ParentId,
    ObjectType,
    UpnpClass,
    DcTitle,
    SortKey,
    MimeType,
    Flags,
    PartNumber,
    TrackNumber,
    Location,
    LastModified,
    LastUpdated,
};

/// \brief autoscan column ids
enum class ASColumn {
    Id = 0,
    ObjId,
    Persistent,
};

/// \brief Map browse column ids to column names
// map ensures entries are in correct order, each value of BrowseCol must be present
static const std::map<BrowseColumn, SearchProperty> browseColMap {
    { BrowseColumn::Id, { ITM_ALIAS, "id" } },
    { BrowseColumn::RefId, { ITM_ALIAS, "ref_id" } },
    { BrowseColumn::ParentId, { ITM_ALIAS, "parent_id" } },
    { BrowseColumn::ObjectType, { ITM_ALIAS, "object_type" } },
    { BrowseColumn::UpnpClass, { ITM_ALIAS, "upnp_class" } },
    { BrowseColumn::DcTitle, { ITM_ALIAS, "dc_title", FieldType::String, 255 } },
    { BrowseColumn::SortKey, { ITM_ALIAS, "sort_key", FieldType::String, 255 } },
    { BrowseColumn::Location, { ITM_ALIAS, "location" } },
    { BrowseColumn::LocationHash, { ITM_ALIAS, "location_hash" } },
    { BrowseColumn::Auxdata, { ITM_ALIAS, "auxdata" } },
    { BrowseColumn::UpdateId, { ITM_ALIAS, "update_id" } },
    { BrowseColumn::MimeType, { ITM_ALIAS, "mime_type", FieldType::String, 40 } },
    { BrowseColumn::Flags, { ITM_ALIAS, "flags" } },
    { BrowseColumn::PartNumber, { ITM_ALIAS, "part_number", FieldType::Integer } },
    { BrowseColumn::TrackNumber, { ITM_ALIAS, "track_number", FieldType::Integer } },
    { BrowseColumn::ServiceId, { ITM_ALIAS, "service_id" } },
    { BrowseColumn::LastModified, { ITM_ALIAS, "last_modified", FieldType::Date } },
    { BrowseColumn::LastUpdated, { ITM_ALIAS, "last_updated", FieldType::Date } },
    { BrowseColumn::RefUpnpClass, { REF_ALIAS, "upnp_class" } },
    { BrowseColumn::RefLocation, { REF_ALIAS, "location" } },
    { BrowseColumn::RefAuxdata, { REF_ALIAS, "auxdata" } },
    { BrowseColumn::RefMimeType, { REF_ALIAS, "mime_type" } },
    { BrowseColumn::RefServiceId, { REF_ALIAS, "service_id" } },
    { BrowseColumn::AsPersistent, { AUS_ALIAS, "persistent", FieldType::Bool } },
};

/// \brief Map search column ids to column names
// map ensures entries are in correct order, each value of SearchCol must be present
static const std::map<SearchColumn, SearchProperty> searchColMap {
    { SearchColumn::Id, { SRC_ALIAS, "id" } },
    { SearchColumn::RefId, { SRC_ALIAS, "ref_id" } },
    { SearchColumn::ParentId, { SRC_ALIAS, "parent_id" } },
    { SearchColumn::ObjectType, { SRC_ALIAS, "object_type" } },
    { SearchColumn::UpnpClass, { SRC_ALIAS, "upnp_class" } },
    { SearchColumn::DcTitle, { SRC_ALIAS, "dc_title", FieldType::String, 255 } },
    { SearchColumn::SortKey, { SRC_ALIAS, "sort_key", FieldType::String, 255 } },
    { SearchColumn::MimeType, { SRC_ALIAS, "mime_type", FieldType::String, 40 } },
    { SearchColumn::Flags, { SRC_ALIAS, "flags", FieldType::Integer } },
    { SearchColumn::PartNumber, { SRC_ALIAS, "part_number", FieldType::Integer } },
    { SearchColumn::TrackNumber, { SRC_ALIAS, "track_number", FieldType::Integer } },
    { SearchColumn::Location, { SRC_ALIAS, "location" } },
    { SearchColumn::LastModified, { SRC_ALIAS, "last_modified", FieldType::Date } },
    { SearchColumn::LastUpdated, { SRC_ALIAS, "last_updated", FieldType::Date } },
};

/// \brief Map meta column ids to column names
// map ensures entries are in correct order, each value of MetadataCol must be present
static const std::map<MetadataColumn, SearchProperty> metaColMap {
    { MetadataColumn::ItemId, { MTA_ALIAS, "item_id" } },
    { MetadataColumn::PropertyName, { MTA_ALIAS, "property_name" } },
    { MetadataColumn::PropertyValue, { MTA_ALIAS, "property_value", FieldType::String, 255 } },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanCol must be present
static const std::map<ASColumn, SearchProperty> asColMap {
    { ASColumn::Id, { AUS_ALIAS, "id" } },
    { ASColumn::ObjId, { AUS_ALIAS, "obj_id" } },
    { ASColumn::Persistent, { AUS_ALIAS, "persistent", FieldType::Integer } },
};

/// \brief Map resource column ids to column names
// map ensures entries are in correct order, each value of ResourceCol must be present
static const std::map<ResourceColumn, SearchProperty> resColMap {
    { ResourceColumn::ItemId, { RES_ALIAS, "item_id" } },
    { ResourceColumn::ResId, { RES_ALIAS, "res_id" } },
    { ResourceColumn::HandlerType, { RES_ALIAS, "handlerType" } },
    { ResourceColumn::Purpose, { RES_ALIAS, "purpose" } },
    { ResourceColumn::Options, { RES_ALIAS, "options" } },
    { ResourceColumn::Parameters, { RES_ALIAS, "parameters" } },
};

/// \brief Map resource search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, ResourceColumn>> resTagMap {
    { UPNP_SEARCH_ID, ResourceColumn::ItemId },
    { "res@id", ResourceColumn::ResId },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanColumn must be present
static const std::map<AutoscanColumn, SearchProperty> autoscanColMap {
    { AutoscanColumn::Id, { AUS_ALIAS, "id" } },
    { AutoscanColumn::ObjId, { AUS_ALIAS, "obj_id" } },
    { AutoscanColumn::ScanMode, { AUS_ALIAS, "scan_mode" } },
    { AutoscanColumn::Recursive, { AUS_ALIAS, "recursive", FieldType::Bool } },
    { AutoscanColumn::MediaType, { AUS_ALIAS, "media_type" } },
    { AutoscanColumn::CtAudio, { AUS_ALIAS, "ct_audio" } },
    { AutoscanColumn::CtImage, { AUS_ALIAS, "ct_image" } },
    { AutoscanColumn::CtVideo, { AUS_ALIAS, "ct_video" } },
    { AutoscanColumn::Hidden, { AUS_ALIAS, "hidden", FieldType::Integer } },
    { AutoscanColumn::FollowSymlinks, { AUS_ALIAS, "follow_symlinks", FieldType::Integer } },
    { AutoscanColumn::Interval, { AUS_ALIAS, "interval", FieldType::Integer } },
    { AutoscanColumn::LastModified, { AUS_ALIAS, "last_modified", FieldType::Date } },
    { AutoscanColumn::Persistent, { AUS_ALIAS, "persistent", FieldType::Bool } },
    { AutoscanColumn::Location, { AUS_ALIAS, "location" } },
    { AutoscanColumn::RetryCount, { AUS_ALIAS, "retry_count", FieldType::Integer } },
    { AutoscanColumn::DirTypes, { AUS_ALIAS, "dir_types", FieldType::Bool } },
    { AutoscanColumn::ForceRescan, { AUS_ALIAS, "force_rescan", FieldType::Bool } },
    { AutoscanColumn::PathIds, { AUS_ALIAS, "path_ids", FieldType::String } },
    { AutoscanColumn::Touched, { AUS_ALIAS, "touched", FieldType::Bool } },
    { AutoscanColumn::ObjLocation, { ITM_ALIAS, "location" } },
    { AutoscanColumn::ItemId, { ITM_ALIAS, "id" } },
};

/// \brief Map browse sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static std::vector<std::pair<std::string, BrowseColumn>> browseSortMap;

/// \brief Map search sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static std::vector<std::pair<std::string, SearchColumn>> searchSortMap;

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, MetadataColumn>> metaTagMap {
    { UPNP_SEARCH_ID, MetadataColumn::ItemId },
    { META_NAME, MetadataColumn::PropertyName },
    { META_VALUE, MetadataColumn::PropertyValue },
};

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, ASColumn>> asTagMap {
    { "id", ASColumn::Id },
    { "obj_id", ASColumn::ObjId },
    { "persistent", ASColumn::Persistent },
};

/// \brief Autoscan search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, AutoscanColumn>> autoscanTagMap {
    { "id", AutoscanColumn::Id },
    { "obj_id", AutoscanColumn::ObjId },
    { "scan_mode", AutoscanColumn::ScanMode },
    { "recursive", AutoscanColumn::Recursive },
    { "hidden", AutoscanColumn::Hidden },
    { "follow_symlinks", AutoscanColumn::FollowSymlinks },
    { "interval", AutoscanColumn::Interval },
    { "last_modified", AutoscanColumn::LastModified },
    { "persistent", AutoscanColumn::Persistent },
    { "location", AutoscanColumn::Location },
    { "retry_count", AutoscanColumn::RetryCount },
    { "dir_types", AutoscanColumn::DirTypes },
    { "force_rescan", AutoscanColumn::ForceRescan },
    { "obj_location", AutoscanColumn::ObjLocation },
};

/// \brief Map playstatus column ids to column names
// map ensures entries are in correct order, each value of PlaystatusCol must be present
static const std::map<PlaystatusColumn, SearchProperty> playstatusColMap {
    { PlaystatusColumn::Group, { PLY_ALIAS, "group" } },
    { PlaystatusColumn::ItemId, { PLY_ALIAS, "item_id" } },
    { PlaystatusColumn::PlayCount, { PLY_ALIAS, "playCount", FieldType::Integer } },
    { PlaystatusColumn::LastPlayed, { PLY_ALIAS, "lastPlayed", FieldType::Date } },
    { PlaystatusColumn::LastPlayedPosition, { PLY_ALIAS, "lastPlayedPosition", FieldType::Integer } },
    { PlaystatusColumn::BookMarkPosition, { PLY_ALIAS, "bookMarkPos", FieldType::Integer } },
};

/// \brief Playstatus search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, PlaystatusColumn>> playstatusTagMap {
    { UPNP_SEARCH_PLAY_GROUP, PlaystatusColumn::Group },
    { UPNP_SEARCH_ID, PlaystatusColumn::ItemId },
    { UPNP_SEARCH_PLAY_COUNT, PlaystatusColumn::PlayCount },
    { UPNP_SEARCH_LAST_PLAYED, PlaystatusColumn::LastPlayed },
    { "lastPlayedPosition", PlaystatusColumn::LastPlayedPosition },
    { "bookMarkPos", PlaystatusColumn::BookMarkPosition },
};

/// \brief Map config column ids to column names
static const std::map<ConfigColumn, SearchProperty> configColumnMap {
    { ConfigColumn::Item, { CFG_ALIAS, "item" } },
    { ConfigColumn::Key, { CFG_ALIAS, "key" } },
    { ConfigColumn::ItemValue, { CFG_ALIAS, "item_value" } },
    { ConfigColumn::Status, { CFG_ALIAS, "status" } },
};

/// \brief Config search keys to column ids
static const std::vector<std::pair<std::string, ConfigColumn>> configTagMap {
    { "item", ConfigColumn::Item },
    { "key", ConfigColumn::Key },
    { "item_value", ConfigColumn::ItemValue },
    { "status", ConfigColumn::Status },
};

/// \brief Map client column ids to column names
static const std::map<ClientColumn, SearchProperty> clientColumnMap {
    { ClientColumn::Addr, { CLT_ALIAS, "addr" } },
    { ClientColumn::Port, { CLT_ALIAS, "port" } },
    { ClientColumn::AddrFamily, { CLT_ALIAS, "addrFamily" } },
    { ClientColumn::UserAgent, { CLT_ALIAS, "userAgent" } },
    { ClientColumn::Last, { CLT_ALIAS, "last" } },
    { ClientColumn::Age, { CLT_ALIAS, "age" } },
};

/// \brief Config search keys to column ids
static const std::vector<std::pair<std::string, ClientColumn>> clientTagMap {
    { "addr", ClientColumn::Addr },
    { "port", ClientColumn::Port },
    { "addrFamily", ClientColumn::AddrFamily },
    { "userAgent", ClientColumn::UserAgent },
    { "last", ClientColumn::Last },
    { "age", ClientColumn::Age },
};

// Format string for a recursive query of a parent container
static constexpr auto sql_search_container_query_raw = R"(
-- Find all children of parent_id
WITH {0} AS (SELECT * FROM {2} WHERE {4} = {{}}
UNION
SELECT {3}.* FROM {2} JOIN {0} AS {1} ON {3}.{4} = {1}.{5}
),
-- Find all physical items and de-reference any virtual item (i.e. follow ref-id)
items AS (SELECT * from {0} AS {1} WHERE {6} IS NULL
UNION
SELECT {3}.* FROM {0} AS {1} JOIN {2} ON {1}.{6} = {3}.{5}
)
-- Select desired cols from items
SELECT {{}} FROM items AS {3})";

#define getCol(rw, idx) (rw)->col(to_underlying((idx)))
#define getColInt(rw, idx, def) (rw)->col_int(to_underlying((idx)), (def))
#define setCol(dict, key, val, map) (dict).emplace((key), quote((val), (map).at((key)).length))

static std::shared_ptr<EnumColumnMapper<BrowseColumn>> browseColumnMapper;
static std::shared_ptr<EnumColumnMapper<SearchColumn>> searchColumnMapper;
static std::shared_ptr<EnumColumnMapper<MetadataColumn>> metaColumnMapper;
static std::shared_ptr<EnumColumnMapper<ASColumn>> asColumnMapper;
static std::shared_ptr<EnumColumnMapper<AutoscanColumn>> autoscanColumnMapper;
static std::shared_ptr<EnumColumnMapper<int>> resourceColumnMapper;
static std::shared_ptr<EnumColumnMapper<ResourceColumn>> resColumnMapper;
static std::shared_ptr<EnumColumnMapper<PlaystatusColumn>> playstatusColumnMapper;
static std::shared_ptr<EnumColumnMapper<ConfigColumn>> configColumnMapper;
static std::shared_ptr<EnumColumnMapper<ClientColumn>> clientColumnMapper;

SQLDatabase::SQLDatabase(
    const std::shared_ptr<Config>& config,
    std::shared_ptr<Mime> mime,
    std::shared_ptr<ConverterManager> converterManager)
    : Database(config)
    , mime(std::move(mime))
    , converterManager(std::move(converterManager))
    , dynamicContentList(this->config->getDynamicContentListOption(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST))
    , dynamicContentEnabled(this->config->getBoolOption(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST_ENABLED))
    , sortKeyEnabled(this->config->getBoolOption(ConfigVal::SERVER_STORAGE_SORT_KEY_ENABLED))
{
}

void SQLDatabase::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw DatabaseException("quote vars need to be overridden", LINE_MESSAGE);

    browseSortMap = {
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_PARTNUMBER), BrowseColumn::PartNumber },
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER), BrowseColumn::PartNumber },
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER), BrowseColumn::TrackNumber },
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE), BrowseColumn::DcTitle },
        { UPNP_SORT_KEY, BrowseColumn::SortKey },
        { UPNP_SEARCH_CLASS, BrowseColumn::UpnpClass },
        { UPNP_SEARCH_PATH, BrowseColumn::Location },
        { UPNP_SEARCH_REFID, BrowseColumn::RefId },
        { UPNP_SEARCH_PARENTID, BrowseColumn::ParentId },
        { UPNP_SEARCH_ID, BrowseColumn::Id },
        { UPNP_SEARCH_LAST_UPDATED, BrowseColumn::LastUpdated },
        { UPNP_SEARCH_LAST_MODIFIED, BrowseColumn::LastModified },
    };
    searchSortMap = {
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER), SearchColumn::PartNumber },
        { MetaEnumMapper::getMetaFieldName(MetadataFields::M_TRACKNUMBER), SearchColumn::TrackNumber },
        { UPNP_SORT_KEY, SearchColumn::SortKey },
        { UPNP_SEARCH_CLASS, SearchColumn::UpnpClass },
        { UPNP_SEARCH_PATH, SearchColumn::Location },
        { UPNP_SEARCH_REFID, SearchColumn::RefId },
        { UPNP_SEARCH_PARENTID, SearchColumn::ParentId },
        { UPNP_SEARCH_ID, SearchColumn::Id },
        { UPNP_SEARCH_LAST_UPDATED, SearchColumn::LastUpdated },
        { UPNP_SEARCH_LAST_MODIFIED, SearchColumn::LastModified },
    };
    /// \brief Map resource search keys to column ids
    // entries are handled sequentially,
    // duplicate entries are added to statement in same order if key is present in SortCriteria
    std::vector<std::pair<std::string, int>> resourceTagMap;
    for (auto&& [key, val] : resTagMap) {
        resourceTagMap.emplace_back(key, to_underlying(val));
    }

    /// \brief Map resource column ids to column names
    // map ensures entries are in correct order, each value of ResourceCol must be present
    std::map<int, SearchProperty> resourceColMap;
    for (auto&& [key, val] : resColMap) {
        resourceColMap[to_underlying(key)] = val;
    }

    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = EnumMapper::getAttributeName(resAttrId);
        resourceTagMap.emplace_back(fmt::format("res@{}", attrName), to_underlying(ResourceColumn::Attributes) + to_underlying(resAttrId));
        resourceColMap.emplace(to_underlying(ResourceColumn::Attributes) + to_underlying(resAttrId), SearchProperty { RES_ALIAS, attrName });
    }

    browseColumnMapper = std::make_shared<EnumColumnMapper<BrowseColumn>>(table_quote_begin, table_quote_end, ITM_ALIAS, CDS_OBJECT_TABLE, browseSortMap, browseColMap);
    auto searchTagMap = searchSortMap;
    if (config->getBoolOption(ConfigVal::UPNP_SEARCH_FILENAME)) {
        if (sortKeyEnabled)
            searchTagMap.emplace_back(MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE), SearchColumn::SortKey);
        else
            searchTagMap.emplace_back(MetaEnumMapper::getMetaFieldName(MetadataFields::M_TITLE), SearchColumn::DcTitle);
    }
    searchColumnMapper = std::make_shared<EnumColumnMapper<SearchColumn>>(table_quote_begin, table_quote_end, SRC_ALIAS, CDS_OBJECT_TABLE, searchTagMap, searchColMap);
    metaColumnMapper = std::make_shared<EnumColumnMapper<MetadataColumn>>(table_quote_begin, table_quote_end, MTA_ALIAS, METADATA_TABLE, metaTagMap, metaColMap);
    resourceColumnMapper = std::make_shared<EnumColumnMapper<int>>(table_quote_begin, table_quote_end, RES_ALIAS, RESOURCE_TABLE, resourceTagMap, resourceColMap);
    resColumnMapper = std::make_shared<EnumColumnMapper<ResourceColumn>>(table_quote_begin, table_quote_end, RES_ALIAS, RESOURCE_TABLE, resTagMap, resColMap);
    playstatusColumnMapper = std::make_shared<EnumColumnMapper<PlaystatusColumn>>(table_quote_begin, table_quote_end, PLY_ALIAS, PLAYSTATUS_TABLE, playstatusTagMap, playstatusColMap);
    asColumnMapper = std::make_shared<EnumColumnMapper<ASColumn>>(table_quote_begin, table_quote_end, AUS_ALIAS, AUTOSCAN_TABLE, asTagMap, asColMap);
    autoscanColumnMapper = std::make_shared<EnumColumnMapper<AutoscanColumn>>(table_quote_begin, table_quote_end, AUS_ALIAS, AUTOSCAN_TABLE, autoscanTagMap, autoscanColMap);
    configColumnMapper = std::make_shared<EnumColumnMapper<ConfigColumn>>(table_quote_begin, table_quote_end, CFG_ALIAS, CONFIG_VALUE_TABLE, configTagMap, configColumnMap);
    clientColumnMapper = std::make_shared<EnumColumnMapper<ClientColumn>>(table_quote_begin, table_quote_end, CFG_ALIAS, CLIENTS_TABLE, clientTagMap, clientColumnMap);

    // Statement for UPnP browse
    {
        std::vector<std::string> buf;
        buf.reserve(browseColMap.size());
        for (auto&& [key, col] : browseColMap) {
            buf.push_back(fmt::format("{}.{}", identifier(col.alias), identifier(col.field)));
        }
        auto join1 = fmt::format("LEFT JOIN {0} {1} ON {2} = {1}.{3}",
            browseColumnMapper->getTableName(), identifier(REF_ALIAS),
            browseColumnMapper->mapQuoted(BrowseColumn::RefId),
            identifier(browseColMap.at(BrowseColumn::Id).field));
        auto join2 = fmt::format("LEFT JOIN {} ON {} = {}",
            asColumnMapper->tableQuoted(),
            asColumnMapper->mapQuoted(ASColumn::ObjId), browseColumnMapper->mapQuoted(BrowseColumn::Id));
        auto join3 = fmt::format("LEFT JOIN {} ON {} = {}",
            playstatusColumnMapper->tableQuoted(),
            playstatusColumnMapper->mapQuoted(PlaystatusColumn::ItemId), browseColumnMapper->mapQuoted(BrowseColumn::Id));
        this->sql_browse_columns = fmt::format("{}", fmt::join(buf, ", "));
        this->sql_browse_query = fmt::format("{} {} {} ", browseColumnMapper->tableQuoted(), join1, join2);
    }
    // Statement for UPnP search
    {
        std::vector<std::string> colBuf;
        colBuf.reserve(searchColMap.size());
        for (auto&& [key, col] : searchColMap) {
            colBuf.push_back(fmt::format("{}.{}", identifier(col.alias), identifier(col.field)));
        }
        this->sql_search_columns = fmt::format("{}", fmt::join(colBuf, ", "));

        auto join1 = fmt::format("LEFT JOIN {} ON {} = {}",
            metaColumnMapper->tableQuoted(),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), metaColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        auto join2 = fmt::format("LEFT JOIN {} ON {} = {}",
            resourceColumnMapper->tableQuoted(),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        auto join3 = fmt::format("LEFT JOIN {} ON {} = {}",
            playstatusColumnMapper->tableQuoted(),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), playstatusColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        this->sql_search_query = fmt::format("{} {} {} {}", searchColumnMapper->tableQuoted(), join1, join2, join3);

        // Build container query format string
        auto sql_container_query = fmt::format(sql_search_container_query_raw,
            identifier("containers"), // defines an alias in in query_raw
            identifier("cont"), // defines an alias in in query_raw
            searchColumnMapper->tableQuoted(), searchColumnMapper->getAlias(),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_PARENTID, true),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_ID, true),
            searchColumnMapper->mapQuoted(UPNP_SEARCH_REFID, true));
        this->sql_search_container_query_format = fmt::format("{} {} {} {}", sql_container_query, join1, join2, join3);
    }
    // Statement for metadata
    {
        std::vector<SQLIdentifier> buf;
        buf.reserve(metaColMap.size());
        for (auto&& [key, col] : metaColMap) {
            buf.push_back(identifier(col.field)); // currently no alias
        }
        this->sql_meta_query = fmt::format("SELECT {} ", fmt::join(buf, ", "));
    }
    // Statement for autoscan
    {
        std::vector<std::string> buf;
        buf.reserve(autoscanColMap.size());
        for (auto&& [key, col] : autoscanColMap) {
            buf.push_back(fmt::format("{}.{}", identifier(col.alias), identifier(col.field)));
        }
        auto join = fmt::format("LEFT JOIN {} ON {} = {}", browseColumnMapper->tableQuoted(), autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId), browseColumnMapper->mapQuoted(BrowseColumn::Id));
        this->sql_autoscan_query = fmt::format("SELECT {} FROM {} {}", fmt::join(buf, ", "), autoscanColumnMapper->tableQuoted(), join);
    }
    // Statement for resource
    {
        std::vector<SQLIdentifier> buf;
        buf.reserve(resourceColMap.size());
        for (auto&& [key, col] : resourceColMap) {
            buf.push_back(identifier(col.field)); // currently no alias
        }
        this->sql_resource_query = fmt::format("SELECT {} ", fmt::join(buf, ", "));
    }

    sqlEmitter = std::make_shared<DefaultSQLEmitter>(searchColumnMapper, metaColumnMapper, resourceColumnMapper, playstatusColumnMapper);
}

void SQLDatabase::upgradeDatabase(
    unsigned int dbVersion,
    const std::array<unsigned int, DBVERSION>& hashies,
    ConfigVal upgradeOption,
    std::string_view updateVersionCommand,
    std::string_view addResourceColumnCmd)
{
    /* --- load database upgrades from config file --- */
    const fs::path& upgradeFile = config->getOption(upgradeOption);
    log_debug("db_version: {}", dbVersion);
    log_debug("Loading SQL Upgrades from: {}", upgradeFile.c_str());
    std::vector<std::vector<std::pair<std::string, std::string>>> dbUpdates;
    pugi::xml_document xmlDoc;
    pugi::xml_parse_result result = xmlDoc.load_file(upgradeFile.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    auto root = xmlDoc.document_element();
    if (std::string_view(root.name()) != "upgrade")
        throw std::runtime_error("Error in upgrade file: <upgrade> tag not found");

    std::size_t version = 1;
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        const pugi::xml_node& versionNode = versionElement.node();
        std::vector<std::pair<std::string, std::string>> versionCmds;
        const auto myHash = stringHash(UpnpXMLBuilder::printXml(versionNode));
        if (version < DBVERSION && myHash == hashies.at(version)) {
            for (auto&& scriptNode : versionNode.children("script")) {
                std::string migration = trimString(scriptNode.attribute("migration").as_string());
                versionCmds.emplace_back(std::move(migration), trimString(scriptNode.text().as_string()));
            }
        } else {
            log_error("Wrong hash for version {}: {} != {}", version + 1, myHash, hashies.at(version));
            throw DatabaseException(fmt::format("Wrong hash for version {}", version + 1), LINE_MESSAGE);
        }
        dbUpdates.push_back(std::move(versionCmds));
        version++;
    }

    if (version != DBVERSION)
        throw DatabaseException(fmt::format("The database upgrade file {} seems to be from another Gerbera version. Expected {}, actual {}", upgradeFile.c_str(), DBVERSION, dbVersion), LINE_MESSAGE);

    version = 1;
    static const std::map<std::string, bool (SQLDatabase::*)()> migActions {
        { "metadata", &SQLDatabase::doMetadataMigration },
        { "resources", &SQLDatabase::doResourceMigration },
    };
    this->addResourceColumnCmd = addResourceColumnCmd;

    /* --- run database upgrades --- */
    for (auto&& upgrade : dbUpdates) {
        if (dbVersion == version) {
            log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
            for (auto&& [migrationCmd, upgradeCmd] : upgrade) {
                bool actionResult = true;
                if (!migrationCmd.empty() && migActions.find(migrationCmd) != migActions.end())
                    actionResult = (*this.*(migActions.at(migrationCmd)))();
                if (actionResult && !upgradeCmd.empty())
                    _exec(upgradeCmd);
            }
            _exec(fmt::format(updateVersionCommand, version + 1, version));
            dbVersion = version + 1;
            log_info("Database upgrade to version {} successful.", dbVersion);
        }
        version++;
    }

    if (dbVersion != DBVERSION)
        throw DatabaseException(fmt::format("The database seems to be from another Gerbera version. Expected {}, actual {}", DBVERSION, dbVersion), LINE_MESSAGE);

    prepareResourceTable(addResourceColumnCmd);
}

void SQLDatabase::shutdown()
{
    shutdownDriver();
}

std::string SQLDatabase::getSortCapabilities()
{
    std::vector<std::string> sortKeys;
    for (auto&& [key, col] : browseSortMap) {
        if (std::find(sortKeys.begin(), sortKeys.end(), key) == sortKeys.end()) {
            sortKeys.push_back(key);
        }
    }
    sortKeys.reserve(to_underlying(MetadataFields::M_MAX) + to_underlying(ResourceAttribute::MAX));
    for (auto&& [field, meta] : MetaEnumMapper::mt_keys) {
        if (std::find(sortKeys.begin(), sortKeys.end(), meta) == sortKeys.end()) {
            sortKeys.emplace_back(meta);
        }
    }
    return fmt::format("{}", fmt::join(sortKeys, ","));
}

std::string SQLDatabase::quote(const std::string& value, std::size_t len) const
{
    if (len > 0 && value.length() > len) {
        return quote(limitString(len, value));
    }
    return quote(value);
}

std::string SQLDatabase::getSearchCapabilities()
{
    auto searchKeys = std::vector {
        std::string(UPNP_SEARCH_CLASS),
        std::string(UPNP_SEARCH_PLAY_COUNT),
        std::string(UPNP_SEARCH_LAST_PLAYED),
        std::string(UPNP_SEARCH_PLAY_GROUP),
    };
    searchKeys.reserve(to_underlying(MetadataFields::M_MAX) + to_underlying(ResourceAttribute::MAX));
    for (auto&& [field, meta] : MetaEnumMapper::mt_keys) {
        searchKeys.emplace_back(meta);
    }
    searchKeys.emplace_back("res");
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = EnumMapper::getAttributeName(resAttrId);
        searchKeys.push_back(fmt::format("res@{}", attrName));
    }
    return fmt::format("{}", fmt::join(searchKeys, ","));
}

std::shared_ptr<CdsObject> SQLDatabase::checkRefID(const std::shared_ptr<CdsObject>& obj)
{
    if (!obj->isVirtual())
        throw DatabaseException("checkRefID called for a non-virtual object", LINE_MESSAGE);

    int refID = obj->getRefID();
    fs::path location = obj->getLocation();

    if (location.empty())
        throw DatabaseException("tried to check refID without a location set", LINE_MESSAGE);

    if (refID > 0) {
        try {
            auto refObj = loadObject(refID);
            if (refObj && refObj->getLocation() == location)
                return refObj;
        } catch (const std::runtime_error&) {
            throw DatabaseException("illegal refID was set", LINE_MESSAGE);
        }
    }

    // This should never happen - but fail softly
    // It means that something doesn't set the refID correctly
    log_warning("Failed to loadObject with refid: {}", refID);

    return findObjectByPath(location, UNUSED_CLIENT_GROUP);
}

std::vector<std::shared_ptr<AddUpdateTable<CdsObject>>> SQLDatabase::_addUpdateObject(
    const std::shared_ptr<CdsObject>& obj,
    Operation op,
    int* changedContainer)
{
    std::shared_ptr<CdsObject> refObj;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef) {
        if (obj->isPureItem())
            throw DatabaseException("Tried to add pure item with PLAYLIST_REF flag set", LINE_MESSAGE);
        if (obj->getRefID() <= 0)
            throw DatabaseException(fmt::format("PLAYLIST_REF flag set for '{}' but refId is <=0", obj->getLocation().c_str()), LINE_MESSAGE);
        refObj = loadObject(obj->getRefID());
        log_vdebug("Setting Playlist {} ref to {}", obj->getLocation().c_str(), refObj->getLocation().c_str());
        if (!refObj)
            throw DatabaseException("PLAYLIST_REF flag set but refId doesn't point to an existing object", LINE_MESSAGE);
    } else if (obj->isVirtual() && obj->isPureItem()) {
        hasReference = true;
        refObj = checkRefID(obj);
        if (!refObj)
            throw DatabaseException("Tried to add or update a virtual object with illegal reference id and an illegal location", LINE_MESSAGE);
    } else if (obj->getRefID() > 0) {
        if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            hasReference = true;
            refObj = loadObject(obj->getRefID());
            if (!refObj)
                throw DatabaseException("OBJECT_FLAG_ONLINE_SERVICE and refID set but refID doesn't point to an existing object", LINE_MESSAGE);
        } else if (obj->isContainer()) {
            // in this case it's a playlist-container. that's ok
            // we don't need to do anything
        } else
            throw DatabaseException("refId set, but it makes no sense", LINE_MESSAGE);
    }

    std::map<BrowseColumn, std::string> cdsObjectSql;
    cdsObjectSql.emplace(BrowseColumn::ObjectType, quote(obj->getObjectType()));

    if (hasReference || playlistRef)
        cdsObjectSql.emplace(BrowseColumn::RefId, quote(refObj->getID()));
    else if (op == Operation::Update)
        cdsObjectSql.emplace(BrowseColumn::RefId, SQL_NULL);

    if (!hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql.emplace(BrowseColumn::UpnpClass, quote(obj->getClass()));
    else if (op == Operation::Update)
        cdsObjectSql.emplace(BrowseColumn::UpnpClass, SQL_NULL);

    setCol(cdsObjectSql, BrowseColumn::DcTitle, obj->getTitle(), browseColMap);
    setCol(cdsObjectSql, BrowseColumn::SortKey, obj->getSortKey(), browseColMap);

    if (op == Operation::Update)
        cdsObjectSql.emplace(BrowseColumn::Auxdata, SQL_NULL);

    auto&& auxData = obj->getAuxData();
    if (!auxData.empty() && (!hasReference || auxData != refObj->getAuxData())) {
        cdsObjectSql.insert_or_assign(BrowseColumn::Auxdata, quote(URLUtils::dictEncode(auxData)));
    }

    const bool useResourceRef = obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    cdsObjectSql.emplace(BrowseColumn::Flags, quote(obj->getFlags()));

    if (obj->getMTime() > std::chrono::seconds::zero()) {
        cdsObjectSql.emplace(BrowseColumn::LastModified, quote(obj->getMTime().count()));
    } else {
        cdsObjectSql.emplace(BrowseColumn::LastModified, SQL_NULL);
    }
    cdsObjectSql.emplace(BrowseColumn::LastUpdated, quote(currentTime().count()));

    int parentID = obj->getParentID();
    if (obj->isContainer() && op == Operation::Update) {
        fs::path dbLocation = addLocationPrefix(obj->isVirtual() ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX, obj->getLocation());
        setCol(cdsObjectSql, BrowseColumn::Location, dbLocation, browseColMap);
        cdsObjectSql.emplace(BrowseColumn::LocationHash, quote(stringHash(dbLocation.string())));
    } else if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (!hasReference) {
            fs::path loc = item->getLocation();
            if (loc.empty())
                throw DatabaseException("tried to create or update a non-referenced item without a location set", LINE_MESSAGE);
            if (obj->isPureItem()) {
                if (parentID < 0) {
                    parentID = ensurePathExistence(loc.parent_path(), changedContainer);
                    obj->setParentID(parentID);
                }
                fs::path dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                setCol(cdsObjectSql, BrowseColumn::Location, dbLocation, browseColMap);
                cdsObjectSql.emplace(BrowseColumn::LocationHash, quote(stringHash(dbLocation.string())));
            } else {
                // URLs
                setCol(cdsObjectSql, BrowseColumn::Location, loc, browseColMap);
                cdsObjectSql.emplace(BrowseColumn::LocationHash, SQL_NULL);
            }
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace(BrowseColumn::Location, SQL_NULL);
            cdsObjectSql.emplace(BrowseColumn::LocationHash, SQL_NULL);
        }

        if (item->getTrackNumber() > 0) {
            cdsObjectSql.emplace(BrowseColumn::TrackNumber, quote(item->getTrackNumber()));
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace(BrowseColumn::TrackNumber, SQL_NULL);
        }

        if (item->getPartNumber() > 0) {
            cdsObjectSql.emplace(BrowseColumn::PartNumber, quote(item->getPartNumber()));
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace(BrowseColumn::PartNumber, SQL_NULL);
        }

        if (!item->getServiceID().empty()) {
            if (!hasReference || std::static_pointer_cast<CdsItem>(refObj)->getServiceID() != item->getServiceID())
                cdsObjectSql.emplace(BrowseColumn::ServiceId, quote(item->getServiceID()));
            else
                cdsObjectSql.emplace(BrowseColumn::ServiceId, SQL_NULL);
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace(BrowseColumn::ServiceId, SQL_NULL);
        }

        setCol(cdsObjectSql, BrowseColumn::MimeType, item->getMimeType(), browseColMap);
    }

    if (obj->getParentID() == INVALID_OBJECT_ID) {
        throw DatabaseException(fmt::format("Tried to create or update an object {} with an illegal parent id {}", obj->getLocation().c_str(), obj->getParentID()), LINE_MESSAGE);
    }

    cdsObjectSql.emplace(BrowseColumn::ParentId, quote(parentID));

    std::vector<std::shared_ptr<AddUpdateTable<CdsObject>>> returnVal;
    // check for a duplicate (virtual) object
    if (hasReference && op != Operation::Update) {
        auto where = std::vector {
            browseColumnMapper->getClause(BrowseColumn::ParentId, fmt::format("{:d}", obj->getParentID()), true),
            browseColumnMapper->getClause(BrowseColumn::RefId, fmt::format("{:d}", refObj->getID()), true),
            browseColumnMapper->getClause(BrowseColumn::DcTitle, quote(obj->getTitle()), true),
        };
        auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
            browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
            browseColumnMapper->getTableName(),
            fmt::join(where, " AND ")));
        // if duplicate items is found - ignore
        if (res && res->getNumRows() > 0) {
            op = Operation::Update;
            auto row = res->nextRow();
            obj->setID(row->col_int(0, INVALID_OBJECT_ID));
            return returnVal;
        }
    }
    auto ot = std::make_shared<Object2Table>(std::move(cdsObjectSql), op, browseColumnMapper);
    returnVal.push_back(std::move(ot));

    if (!hasReference || obj->getMetaData() != refObj->getMetaData()) {
        generateMetaDataDBOperations(obj, op, returnVal);
    }

    if (!hasReference || (!useResourceRef && !refObj->resourcesEqual(obj))) {
        log_debug("Updating Resources {}", obj->getLocation().string());
        generateResourceDBOperations(obj, op, returnVal);
    }

    return returnVal;
}

void SQLDatabase::addObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw DatabaseException("Tried to add an object with an object ID set", LINE_MESSAGE);

    auto tables = _addUpdateObject(obj, Operation::Insert, changedContainer);

    beginTransaction("addObject");
    for (auto&& addUpdateTable : tables) {
        auto qb = addUpdateTable->sqlForInsert(obj);
        log_debug("Generated insert: {}", qb);

        if (addUpdateTable->hasInsertResult()) {
            int newId = exec(qb, true);
            obj->setID(newId);
        } else {
            exec(CDS_OBJECT_TABLE, qb, obj->getID());
        }
    }
    commit("addObject");
}

void SQLDatabase::updateObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer)
{
    std::vector<std::shared_ptr<AddUpdateTable<CdsObject>>> data;
    if (obj->getID() == CDS_ID_FS_ROOT) {
        std::map<BrowseColumn, std::string> cdsObjectSql;

        cdsObjectSql[BrowseColumn::DcTitle] = quote(obj->getTitle(), browseColMap.at(BrowseColumn::DcTitle).length);
        cdsObjectSql[BrowseColumn::UpnpClass] = quote(obj->getClass());

        auto ot = std::make_shared<Object2Table>(std::move(cdsObjectSql), Operation::Update, browseColumnMapper);
        data.push_back(std::move(ot));
    } else {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw DatabaseException(fmt::format("Tried to update an object with a forbidden ID ({})", obj->getID()), LINE_MESSAGE);
        data = _addUpdateObject(obj, Operation::Update, changedContainer);
    }

    beginTransaction("updateObject");
    for (auto&& addUpdateTable : data) {
        Operation op = addUpdateTable->getOperation();
        auto qb = [this, &obj, op, &addUpdateTable]() {
            switch (op) {
            case Operation::Insert:
                return addUpdateTable->sqlForInsert(obj);
            case Operation::InsertMulti:
                return addUpdateTable->sqlForMultiInsert(obj);
            case Operation::Update:
                return addUpdateTable->sqlForUpdate(obj);
            case Operation::Delete:
                return addUpdateTable->sqlForDelete(obj);
            }
            return std::string();
        }();

        log_debug("upd_query: {}", qb);
        switch (op) {
        case Operation::Insert:
        case Operation::InsertMulti:
        case Operation::Update:
            exec(CDS_OBJECT_TABLE, qb, obj->getID());
            break;
        case Operation::Delete:
            del(addUpdateTable->getTableName(), qb, { obj->getID() });
            break;
        }
    }
    commit("updateObject");
}

std::shared_ptr<CdsObject> SQLDatabase::loadObject(int objectID)
{
    if (dynamicContainers.find(objectID) != dynamicContainers.end()) {
        return dynamicContainers.at(objectID);
    }

    beginTransaction("loadObject");
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {}",
        sql_browse_columns,
        sql_browse_query,
        browseColumnMapper->getClause(BrowseColumn::Id, quote(objectID)));
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(UNUSED_CLIENT_GROUP, row);
            commit("loadObject");
            return result;
        }
    }
    log_debug("sql_query = {}", loadSql);
    commit("loadObject");
    throw ObjectNotFoundException(fmt::format("Object not found: {}", objectID));
}

std::shared_ptr<CdsObject> SQLDatabase::loadObject(const std::string& group, int objectID)
{
    if (dynamicContainers.find(objectID) != dynamicContainers.end()) {
        return dynamicContainers.at(objectID);
    }

    beginTransaction("loadObject");
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {} = {}", sql_browse_columns, sql_browse_query, browseColumnMapper->mapQuoted(BrowseColumn::Id), objectID);
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(group, row);
            commit("loadObject");
            return result;
        }
    }
    log_debug("sql_query = {}", loadSql);
    commit("loadObject");
    throw ObjectNotFoundException(fmt::format("Object not found: {}", objectID));
}

std::shared_ptr<CdsObject> SQLDatabase::loadObjectByServiceID(const std::string& serviceID, const std::string& group)
{
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {} = {}", sql_browse_columns, sql_browse_query, browseColumnMapper->mapQuoted(BrowseColumn::ServiceId), quote(serviceID));
    beginTransaction("loadObjectByServiceID");
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            commit("loadObjectByServiceID");
            return createObjectFromRow(group, row);
        }
    }
    commit("loadObjectByServiceID");

    return {};
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::findObjectByContentClass(
    const std::string& contentClass,
    int startIndex,
    int count,
    const std::string& group)
{
    auto srcParam = SearchParam(fmt::to_string(CDS_ID_ROOT),
        fmt::format("{} derivedfrom \"{}\"", MetaEnumMapper::getMetaFieldName(MetadataFields::M_CONTENT_CLASS), contentClass),
        "", startIndex, count, false, group, true, true);
    log_debug("Running content class search for '{}'", contentClass);
    auto result = this->search(srcParam);
    log_debug("Content class search {} returned {}", contentClass, srcParam.getTotalMatches());
    return result;
}

std::vector<int> SQLDatabase::getServiceObjectIDs(char servicePrefix)
{
    auto getSql = fmt::format("SELECT {} FROM {} WHERE {} LIKE {}",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->mapQuoted(BrowseColumn::ServiceId, true), quote(std::string(1, servicePrefix) + '%'));

    beginTransaction("getServiceObjectIDs");
    auto res = select(getSql);
    commit("getServiceObjectIDs");
    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", CDS_OBJECT_TABLE), LINE_MESSAGE);

    std::vector<int> objectIDs;
    objectIDs.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        objectIDs.push_back(row->col_int(0, INVALID_OBJECT_ID));
    }

    return objectIDs;
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::browse(BrowseParam& param)
{
    const auto parent = param.getObject();
    bool getContainers = param.getFlag(BROWSE_CONTAINERS);
    bool getItems = param.getFlag(BROWSE_ITEMS);
    const auto forbiddenDirectories = param.getForbiddenDirectories();
    log_vdebug("browse forbiddenDirectories {}", fmt::join(forbiddenDirectories, ","));

    if (param.getDynamicContainers() && dynamicContainers.find(parent->getID()) != dynamicContainers.end()) {
        auto dynConfig = dynamicContentList->getKey(parent->getLocation());
        if (dynConfig) {
            auto reqCount = (param.getRequestedCount() <= 0 || param.getRequestedCount() > dynConfig->getMaxCount()) ? dynConfig->getMaxCount() : param.getRequestedCount();
            auto srcParam = SearchParam(fmt::to_string(parent->getParentID()), dynConfig->getFilter(), dynConfig->getSort(), // get params from config
                param.getStartingIndex(), reqCount, false, param.getGroup(),
                getContainers, getItems); // get params from browse
            srcParam.setForbiddenDirectories(forbiddenDirectories);
            log_debug("Running Dynamic search {} in {} '{}'", parent->getID(), parent->getParentID(), dynConfig->getFilter());
            auto result = this->search(srcParam);
            auto numMatches = srcParam.getTotalMatches() > dynConfig->getMaxCount() ? dynConfig->getMaxCount() : srcParam.getTotalMatches();
            log_debug("Dynamic search {} returned {}", parent->getID(), numMatches);
            param.setTotalMatches(numMatches);
            return result;
        }
        log_warning("Dynamic content {} error '{}'", parent->getID(), parent->getLocation().string());
    }

    bool hideFsRoot = param.getFlag(BROWSE_HIDE_FS_ROOT);
    int childCount = 1;
    if (param.getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        childCount = getChildCount(parent->getID(), getContainers, getItems, hideFsRoot);
        param.setTotalMatches(childCount);
    } else {
        param.setTotalMatches(1);
    }

    std::vector<std::string> where;
    std::string orderBy;
    std::string limit;
    std::string addColumns;
    std::string addJoin;

    if (param.getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {

        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseColumn::ParentId), parent->getID()));

        if (parent->getID() == CDS_ID_ROOT && hideFsRoot)
            where.push_back(fmt::format("{} != {:d}", browseColumnMapper->mapQuoted(BrowseColumn::Id), CDS_ID_FS_ROOT));

        if (getItems && !forbiddenDirectories.empty()) {
            std::vector<std::string> forbiddenList;
            for (auto&& forbDir : forbiddenDirectories) {
                forbiddenList.push_back(fmt::format("({0} is not null AND {0} like {1})", browseColumnMapper->mapQuoted(BrowseColumn::RefLocation), quote(addLocationPrefix(LOC_FILE_PREFIX, forbDir, "%"))));
                forbiddenList.push_back(fmt::format("({0} is not null AND {0} like {1})", browseColumnMapper->mapQuoted(BrowseColumn::Location), quote(addLocationPrefix(LOC_FILE_PREFIX, forbDir, "%"))));
            }
            where.push_back(fmt::format("(NOT (({0} & {1}) = {1} AND ({2})) OR ({0} & {1}) != {1})", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType), OBJECT_TYPE_ITEM, fmt::join(forbiddenList, " OR ")));
        }

        // order by code..
        auto orderByCode = [&]() {
            std::string orderQb;
            if (param.getFlag(BROWSE_TRACK_SORT)) {
                orderQb = fmt::format("{},{}", browseColumnMapper->mapQuoted(BrowseColumn::PartNumber), browseColumnMapper->mapQuoted(BrowseColumn::TrackNumber));
            } else {
                SortParser sortParser(browseColumnMapper, playstatusColumnMapper, metaColumnMapper, param.getSortCriteria());
                orderQb = sortParser.parse(addColumns, addJoin);
            }
            if (orderQb.empty()) {
                if (sortKeyEnabled)
                    orderQb = browseColumnMapper->mapQuoted(BrowseColumn::SortKey);
                else
                    orderQb = browseColumnMapper->mapQuoted(BrowseColumn::DcTitle);
            }
            return orderQb;
        };

        auto limitCode = [](int startingIndex, int requestedCount) {
            if (startingIndex > 0 && requestedCount > 0) {
                return fmt::format(" LIMIT {} OFFSET {}", requestedCount, startingIndex);
            } else if (startingIndex > 0) {
                return fmt::format(" LIMIT ~0 OFFSET {}", startingIndex);
            } else if (requestedCount > 0) {
                return fmt::format(" LIMIT {}", requestedCount);
            }
            return std::string();
        };

        if (!getContainers && !getItems) {
            auto zero = std::string("0 = 1");
            where.push_back(std::move(zero));
        } else if (getContainers && !getItems) {
            where.push_back(fmt::format("{} = {:d}", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType), OBJECT_TYPE_CONTAINER));
            // Sorting by UpnpClass will avoid mixing different types of containers
            // "Special" containers like "All Songs" (which are of upnp_class 'object.container') will be displayed before
            // albums (which are of upnp_class 'object.container.album.musicAlbum')
            orderBy = fmt::format(" ORDER BY {}, {}", browseColumnMapper->mapQuoted(BrowseColumn::UpnpClass), orderByCode());
        } else if (!getContainers && getItems) {
            where.push_back(fmt::format("({0} & {1}) = {1}", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType), OBJECT_TYPE_ITEM));
            orderBy = fmt::format(" ORDER BY {}", orderByCode());
        } else {
            // ORDER BY (object_type = OBJECT_TYPE_CONTAINER) ensures that containers are returned before items
            // Sorting by UpnpClass will avoid mixing different types of containers
            orderBy = fmt::format(" ORDER BY ({} = {}) DESC, {}, {}", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType), OBJECT_TYPE_CONTAINER,
                browseColumnMapper->mapQuoted(BrowseColumn::UpnpClass), orderByCode());
        }

        limit = limitCode(param.getStartingIndex(), param.getRequestedCount());
    } else { // metadata
        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseColumn::Id), parent->getID()));
        limit = " LIMIT 1";
    }
    auto qb = fmt::format("SELECT {} {} FROM {} {} WHERE {}{}{}", sql_browse_columns, addColumns, sql_browse_query, addJoin, fmt::join(where, " AND "), orderBy, limit);
    log_debug("QUERY: {}", qb);
    beginTransaction("browse");
    std::shared_ptr<SQLResult> sqlResult = select(qb);
    commit("browse");

    std::vector<std::shared_ptr<CdsObject>> result;
    std::vector<std::shared_ptr<CdsContainer>> containers;
    result.reserve(sqlResult->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        auto obj = createObjectFromRow(param.getGroup(), row);
        if (obj->isContainer()) {
            containers.push_back(std::static_pointer_cast<CdsContainer>(obj));
        }
        result.push_back(std::move(obj));
    }

    // update childCount fields of containers (query all containers in one batch)
    if (!containers.empty()) {
        std::vector<int> contIds;
        contIds.reserve(containers.size());
        std::transform(containers.begin(), containers.end(), std::back_inserter(contIds),
            [](auto&& container) { return container->getID(); });

        const auto childCounts = getChildCounts(contIds, getContainers, getItems, hideFsRoot);
        for (auto&& cont : containers) {
            auto it = childCounts.find(cont->getID());
            if (it != childCounts.end())
                cont->setChildCount(it->second);
            else
                cont->setChildCount(0);
        }
    }

    if (param.getDynamicContainers() && dynamicContentEnabled && getContainers && param.getStartingIndex() == 0 && param.getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        if (dynamicContainers.size() < dynamicContentList->size()) {
            initDynContainers(parent);
        }
        for (auto&& [dynId, dynFolder] : dynamicContainers) {
            if (dynFolder->getParentID() == parent->getID()) {
                result.push_back(dynFolder);
                childCount++;
            }
        }
        param.setTotalMatches(childCount);
    }
    return result;
}

void SQLDatabase::initDynContainers(const std::shared_ptr<CdsObject>& sParent)
{
    if (dynamicContentEnabled && dynamicContentList && dynamicContainers.size() < dynamicContentList->size()) {
        bool hasCaseSensitiveNames = config->getBoolOption(ConfigVal::IMPORT_CASE_SENSITIVE_TAGS);
        for (std::size_t count = 0; count < dynamicContentList->size(); count++) {
            auto dynConfig = dynamicContentList->get(count);
            auto location = dynConfig->getLocation().parent_path();
            if (!hasCaseSensitiveNames) {
                location = toLower(location);
            }
            auto parent = location != "/" ? findObjectByPath(location, UNUSED_CLIENT_GROUP, DbFileType::Virtual) : loadObject(CDS_ID_ROOT);
            if (sParent && (sParent->getLocation() == location || (sParent->getLocation().empty() && location == "/"))) {
                parent = sParent;
            }
            if (!parent)
                parent = findObjectByPath(location, UNUSED_CLIENT_GROUP, DbFileType::Directory);
            auto dynId = parent ? static_cast<std::int32_t>(static_cast<std::int64_t>(-(parent->getID() + 1)) * 10000 - count) : INVALID_OBJECT_ID;
            // create runtime container
            if (parent && dynamicContainers.find(dynId) == dynamicContainers.end()) {
                auto dynFolder = std::make_shared<CdsContainer>();
                dynFolder->setTitle(dynConfig->getTitle());
                dynFolder->setID(dynId);
                dynFolder->setParentID(parent->getID());
                location = dynConfig->getLocation();
                if (!hasCaseSensitiveNames) {
                    location = toLower(location);
                }
                dynFolder->setLocation(location);
                dynFolder->setClass(UPNP_CLASS_DYNAMIC_CONTAINER);
                dynFolder->setUpnpShortcut(dynConfig->getUpnpShortcut());

                auto image = dynConfig->getImage();
                std::error_code ec;
                if (!image.empty() && isRegularFile(image, ec)) {
                    auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, ResourcePurpose::Thumbnail);
                    std::string type = image.extension().string().substr(1);
                    auto mimeType = mime->getMimeType(image, fmt::format("image/{}", type));
                    if (!mimeType.second.empty()) {
                        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(mimeType.second));
                        resource->addAttribute(ResourceAttribute::RESOURCE_FILE, image);
                    }
                    dynFolder->addResource(resource);
                }
                dynamicContainers.emplace(dynId, std::move(dynFolder));
            }
        }
    }
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::search(SearchParam& param)
{
    auto searchParser = SearchParser(*sqlEmitter, param.getSearchCriteria());
    std::shared_ptr<ASTNode> rootNode = searchParser.parse();
    std::string searchSQL(rootNode->emitSQL());
    if (searchSQL.empty())
        throw DatabaseException("failed to generate SQL for search", LINE_MESSAGE);

    bool getContainers = param.getContainers();
    bool getItems = param.getItems();
    if (getContainers && !getItems) {
        searchSQL.append(fmt::format(" AND ({} = {:d})", searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_CONTAINER));
    } else if (!getContainers && getItems) {
        searchSQL.append(fmt::format(" AND (({0} & {1}) = {1})", searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_ITEM));
    }
    if (param.getSearchableContainers()) {
        searchSQL.append(fmt::format(" AND ({0} & {1} = {1} OR {2} != {3})",
            searchColumnMapper->mapQuoted(SearchColumn::Flags), OBJECT_FLAG_SEARCHABLE, searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_CONTAINER));
    }

    const auto forbiddenDirectories = param.getForbiddenDirectories();
    log_vdebug("search forbiddenDirectories {}", fmt::join(forbiddenDirectories, ","));
    if (getItems && !forbiddenDirectories.empty()) {
        std::vector<std::string> forbiddenList;
        for (auto&& forbDir : forbiddenDirectories) {
            forbiddenList.push_back(fmt::format("({0} is not null AND {0} like {1})", searchColumnMapper->mapQuoted(SearchColumn::Location), quote(addLocationPrefix(LOC_FILE_PREFIX, forbDir, "%"))));
        }
        searchSQL.append(fmt::format("AND (NOT (({0} & {1}) = {1} AND ({2})) OR ({0} & {1}) != {1})", searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_ITEM, fmt::join(forbiddenList, " OR ")));
    }
    log_debug("Search query resolves to SQL [\n{}\n]", searchSQL);

    bool rootContainer = param.getContainerID().empty() || param.getContainerID() == "0";

    std::string countSQL;
    if (rootContainer) {
        // Use faster, non-recursive search for root container
        countSQL = fmt::format("SELECT COUNT(DISTINCT {}) FROM {} WHERE {}", searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), sql_search_query, searchSQL);
    } else {
        // Use recursive container search
        const std::string countSelect = fmt::format("COUNT(DISTINCT {})", searchColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        countSQL = fmt::format(sql_search_container_query_format, param.getContainerID(), countSelect);
        countSQL += fmt::format(" WHERE {}", searchSQL);
    }

    log_debug("Search count resolves to SQL [\n{}\n]", countSQL);
    beginTransaction("search");
    auto sqlResult = select(countSQL);
    commit("search");

    auto countRow = sqlResult->nextRow();
    if (countRow) {
        param.setTotalMatches(countRow->col_int(0, 0));
    }

    std::string addColumns;
    std::string addJoin;
    // order by code..
    auto orderByCode = [&]() {
        SortParser sortParser(searchColumnMapper, playstatusColumnMapper, metaColumnMapper, param.getSortCriteria());
        auto orderQb = sortParser.parse(addColumns, addJoin);
        if (orderQb.empty()) {
            if (sortKeyEnabled)
                orderQb = searchColumnMapper->mapQuoted(SearchColumn::SortKey);
            else
                orderQb = searchColumnMapper->mapQuoted(SearchColumn::DcTitle);
        }
        return orderQb;
    };

    auto orderBy = fmt::format(" ORDER BY {}", orderByCode());

    auto startingIndex = param.getStartingIndex();
    auto requestedCount = param.getRequestedCount();
    auto limitCode = [&]() {
        if (startingIndex > 0 && requestedCount > 0) {
            return fmt::format(" LIMIT {} OFFSET {}", requestedCount, startingIndex);
        } else if (startingIndex > 0) {
            return fmt::format(" LIMIT ~0 OFFSET {}", startingIndex);
        } else if (requestedCount > 0) {
            return fmt::format(" LIMIT {}", requestedCount);
        }
        return std::string();
    };

    std::string limit = limitCode();
    log_vdebug("limitCode {}", limit);

    std::string retrievalSQL;
    if (rootContainer) {
        // Use faster, non-recursive search for root container
        retrievalSQL = fmt::format("SELECT DISTINCT {} {} FROM {} {} WHERE {}{}{}", sql_search_columns, addColumns, sql_search_query, addJoin, searchSQL, orderBy, limit);
    } else {
        // Use recursive container search
        const std::string retrievalSelect = fmt::format("DISTINCT {} {}", sql_search_columns, addColumns);
        retrievalSQL = fmt::format(sql_search_container_query_format, param.getContainerID(), retrievalSelect);
        retrievalSQL += fmt::format(" {} WHERE {}{}{}", addJoin, searchSQL, orderBy, limit);
    }

    log_debug("Search statement resolves to SQL [\n{}\n]", retrievalSQL);
    beginTransaction("search 2");
    sqlResult = select(retrievalSQL);
    commit("search 2");

    std::vector<std::shared_ptr<CdsObject>> result;
    result.reserve(sqlResult->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        result.push_back(createObjectFromSearchRow(param.getGroup(), row));
    }

    if (static_cast<long long>(result.size()) < requestedCount) {
        param.setTotalMatches(startingIndex + result.size()); // make sure we do not report too many hits
    }

    return result;
}

int SQLDatabase::getChildCount(int contId, bool containers, bool items, bool hideFsRoot)
{
    if (!containers && !items)
        return 0;

    auto where = std::vector {
        browseColumnMapper->getClause(BrowseColumn::ParentId, contId, true)
    };
    if (containers && !items)
        where.push_back(browseColumnMapper->getClause(BrowseColumn::ObjectType, OBJECT_TYPE_CONTAINER, true));
    else if (items && !containers)
        where.push_back(fmt::format("({0} & {1}) = {1}", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true), OBJECT_TYPE_ITEM));
    if (contId == CDS_ID_ROOT && hideFsRoot) {
        where.push_back(fmt::format("{} != {:d}", browseColumnMapper->mapQuoted(BrowseColumn::Id, true), CDS_ID_FS_ROOT));
    }

    beginTransaction("getChildCount");
    auto res = select(fmt::format("SELECT COUNT(*) FROM {} WHERE {}", browseColumnMapper->getTableName(), fmt::join(where, " AND ")));
    commit("getChildCount");

    if (res) {
        auto row = res->nextRow();
        if (row) {
            return row->col_int(0, 0);
        }
    }
    return 0;
}

std::map<int, int> SQLDatabase::getChildCounts(const std::vector<int>& contId, bool containers, bool items, bool hideFsRoot)
{
    std::map<int, int> result;
    if (!containers && !items)
        return result;

    if (contId.empty())
        return result;

    auto where = std::vector {
        fmt::format("{} IN ({})", browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true), fmt::join(contId, ","))
    };
    if (containers && !items)
        where.push_back(browseColumnMapper->getClause(BrowseColumn::ObjectType, OBJECT_TYPE_CONTAINER, true));
    else if (items && !containers)
        where.push_back(fmt::format("({0} & {1}) = {1}", browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true), OBJECT_TYPE_ITEM));
    if (hideFsRoot && std::find(contId.begin(), contId.end(), CDS_ID_ROOT) != contId.end()) {
        where.push_back(fmt::format("{} != {:d}", browseColumnMapper->mapQuoted(BrowseColumn::Id, true), CDS_ID_FS_ROOT));
    }

    beginTransaction("getChildCounts");
    auto res = select(fmt::format("SELECT {0}, COUNT(*) FROM {1} WHERE {2} GROUP BY {0} ORDER BY {0}",
        browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true),
        browseColumnMapper->getTableName(),
        fmt::join(where, " AND ")));
    commit("getChildCounts");

    if (res) {
        std::unique_ptr<SQLRow> row;
        while ((row = res->nextRow())) {
            result.emplace_hint(result.end(), row->col_int(0, INVALID_OBJECT_ID), row->col_int(1, 0));
        }
    }
    return result;
}

std::vector<std::string> SQLDatabase::getMimeTypes()
{
    beginTransaction("getMimeTypes");
    auto res = select(fmt::format("SELECT DISTINCT {0} FROM {1} WHERE {0} IS NOT NULL ORDER BY {0}",
        browseColumnMapper->mapQuoted(BrowseColumn::MimeType, true),
        browseColumnMapper->getTableName()));
    commit("getMimeTypes");

    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", CDS_OBJECT_TABLE), LINE_MESSAGE);

    std::vector<std::string> arr;
    arr.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        arr.push_back(row->col(0));
    }

    return arr;
}

std::map<std::string, std::shared_ptr<CdsContainer>> SQLDatabase::getShortcuts()
{
    auto srcParam = SearchParam(fmt::to_string(CDS_ID_ROOT),
        fmt::format("{} != \"\"", MetaEnumMapper::getMetaFieldName(MetadataFields::M_UPNP_SHORTCUT)),
        "", 0, 0, false, UNUSED_CLIENT_GROUP, true, false);
    std::map<std::string, std::shared_ptr<CdsContainer>> result;

    auto searchResult = this->search(srcParam);
    log_debug("Shortcut search returned {}", srcParam.getTotalMatches());
    for (auto&& obj : searchResult) {
        if (obj->isContainer()) {
            auto cont = std::static_pointer_cast<CdsContainer>(obj);
            result[cont->getUpnpShortcut()] = cont;
        }
    }
    for (auto&& dynFolder : dynamicContainers) {
        auto shortcut = dynFolder.second->getUpnpShortcut();
        if (!shortcut.empty()) {
            result[shortcut] = dynFolder.second;
        }
    }

    return result;
}

std::shared_ptr<CdsObject> SQLDatabase::findObjectByPath(
    const fs::path& fullpath,
    const std::string& group,
    DbFileType fileType)
{
    auto fileTypeList = fileType == DbFileType::Any ? std::vector<DbFileType> { DbFileType::File, DbFileType::Directory, DbFileType::Virtual } : std::vector<DbFileType> { fileType };
    for (auto&& ft : fileTypeList) {
        std::string dbLocation = [&fullpath, ft] {
            std::error_code ec;
            if (ft == DbFileType::File || (ft == DbFileType::Auto && isRegularFile(fullpath, ec)))
                return addLocationPrefix(LOC_FILE_PREFIX, fullpath);
            if (ft == DbFileType::Virtual)
                return addLocationPrefix(LOC_VIRT_PREFIX, fullpath);
            return addLocationPrefix(LOC_DIR_PREFIX, fullpath);
        }();

        auto where = std::vector {
            browseColumnMapper->getClause(BrowseColumn::LocationHash, quote(stringHash(dbLocation))),
            browseColumnMapper->getClause(BrowseColumn::Location, quote(dbLocation)),
            fmt::format("{} IS NULL", browseColumnMapper->mapQuoted(BrowseColumn::RefId)),
        };
        auto findSql = fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1", sql_browse_columns, sql_browse_query, fmt::join(where, " AND "));

        beginTransaction("findObjectByPath");
        auto res = select(findSql);
        log_debug("{} -> res={} ({})", findSql, !!res, res ? res->getNumRows() : -1);
        if (!res) {
            commit("findObjectByPath");
            throw DatabaseException(fmt::format("error while doing select: {}", findSql), LINE_MESSAGE);
        }
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(group, row);
            commit("findObjectByPath");
            return result;
        }
    }

    commit("findObjectByPath");
    return nullptr;
}

int SQLDatabase::findObjectIDByPath(const fs::path& fullpath, DbFileType fileType)
{
    auto obj = findObjectByPath(fullpath, UNUSED_CLIENT_GROUP, fileType);
    if (!obj)
        return INVALID_OBJECT_ID;
    return obj->getID();
}

int SQLDatabase::ensurePathExistence(const fs::path& path, int* changedContainer)
{
    if (changedContainer)
        *changedContainer = INVALID_OBJECT_ID;

    if (path == std::string(1, DIR_SEPARATOR))
        return CDS_ID_FS_ROOT;

    auto obj = findObjectByPath(path, UNUSED_CLIENT_GROUP, DbFileType::Directory);
    if (obj)
        return obj->getID();

    int parentID = ensurePathExistence(path.parent_path(), changedContainer);

    if (changedContainer && *changedContainer == INVALID_OBJECT_ID)
        *changedContainer = parentID;

    std::vector<std::pair<std::string, std::string>> itemMetadata;
    itemMetadata.emplace_back(MetaEnumMapper::getMetaFieldName(MetadataFields::M_DATE), grbLocaltime("{:%FT%T%z}", toSeconds(fs::last_write_time(path))));

    auto f2i = converterManager->f2i();
    auto [mval, err] = f2i->convert(path.filename());
    if (!err.empty()) {
        log_warning("{}: {}", path.filename().string(), err);
    }
    return createContainer(parentID, mval, path, OBJECT_FLAG_RESTRICTED, false, "", INVALID_OBJECT_ID, itemMetadata);
}

int SQLDatabase::createContainer(
    int parentID,
    const std::string& name,
    const std::string& virtualPath,
    int flags,
    bool isVirtual,
    const std::string& upnpClass,
    int refID,
    const std::vector<std::pair<std::string, std::string>>& itemMetadata,
    const std::vector<std::shared_ptr<CdsResource>>& itemResources)
{
    log_debug("Creating Container: parent: {}, name: '{}', path '{}', flags: {}, isVirt: {}, upnpClass: '{}', refId: {}",
        parentID, name, virtualPath, flags, isVirtual, upnpClass, refID);
    if (refID > 0) {
        auto refObj = loadObject(refID);
        if (!refObj)
            throw DatabaseException("tried to create container with refID set, but refID doesn't point to an existing object", LINE_MESSAGE);
    }
    std::string dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), virtualPath);

    auto dict = std::map<BrowseColumn, std::string> {
        { BrowseColumn::ParentId, fmt::to_string(parentID) },
        { BrowseColumn::ObjectType, fmt::to_string(OBJECT_TYPE_CONTAINER) },
        { BrowseColumn::Flags, fmt::to_string(flags) },
        { BrowseColumn::UpnpClass, !upnpClass.empty() ? quote(upnpClass) : quote(UPNP_CLASS_CONTAINER) },
        { BrowseColumn::DcTitle, quote(name) },
        { BrowseColumn::SortKey, quote(name) },
        { BrowseColumn::Location, quote(dbLocation) },
        { BrowseColumn::LocationHash, quote(stringHash(dbLocation)) },
        { BrowseColumn::RefId, (refID > 0) ? quote(refID) : SQL_NULL },
    };
    beginTransaction("createContainer");
    Object2Table ot(std::move(dict), Operation::Insert, browseColumnMapper);
    int newId = exec(ot.sqlForInsert(nullptr), true); // true = get last id#
    log_debug("Created object row, id: {}", newId);

    const std::string newIdStr = quote(newId);
    if (!itemMetadata.empty()) {
        std::vector<std::map<MetadataColumn, std::string>> multiDict;
        multiDict.reserve(itemMetadata.size());
        for (auto&& [key, val] : itemMetadata) {
            auto mDict = std::map<MetadataColumn, std::string> {
                { MetadataColumn::ItemId, newIdStr },
                { MetadataColumn::PropertyName, quote(key) },
                { MetadataColumn::PropertyValue, quote(val) },
            };
            multiDict.push_back(std::move(mDict));
        }
        Metadata2Table mt(std::move(multiDict), metaColumnMapper);
        exec(METADATA_TABLE, mt.sqlForMultiInsert(nullptr), newId);
        log_debug("Wrote metadata for cds_object {}", newId);
    }

    if (!itemResources.empty()) {
        int resId = 0;
        for (auto&& resource : itemResources) {
            auto rDict = std::map<ResourceColumn, std::string> {
                { ResourceColumn::ItemId, quote(newId) },
                { ResourceColumn::ResId, quote(resId) },
                { ResourceColumn::HandlerType, quote(to_underlying(resource->getHandlerType())) },
                { ResourceColumn::Purpose, quote(to_underlying(resource->getPurpose())) },
            };
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                rDict[ResourceColumn::Options] = quote(URLUtils::dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                rDict[ResourceColumn::Parameters] = quote(URLUtils::dictEncode(parameters));
            }
            std::map<ResourceAttribute, std::string> rAttr;
            for (auto&& [key, val] : resource->getAttributes()) {
                rAttr[key] = quote(val);
            }
            Resource2Table rt(std::move(rDict), std::move(rAttr), Operation::Insert, resColumnMapper);
            exec(RESOURCE_TABLE, rt.sqlForInsert(nullptr), newId);
            resId++;
        }
        log_debug("Wrote resources for cds_object {}", newId);
    }
    commit("createContainer");

    return newId;
}

void SQLDatabase::deleteAll(std::string_view tableName)
{
    del(tableName, "", {});
}

void SQLDatabase::deleteRows(std::string_view tableName, const std::string& key, const std::vector<int>& values)
{
    del(tableName, fmt::format("{} IN ({})", identifier(key), fmt::join(values, ",")), values);
}

fs::path SQLDatabase::buildContainerPath(int parentID, const std::string& title)
{
    if (parentID == CDS_ID_ROOT)
        return title;

    beginTransaction("buildContainerPath");
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
        browseColumnMapper->mapQuoted(BrowseColumn::Location, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->getClause(BrowseColumn::Id, parentID, true)));
    commit("buildContainerPath");
    if (!res)
        return {};

    auto row = res->nextRow();
    if (!row)
        return {};

    auto [path, prefix] = stripLocationPrefix(fmt::format("{}{}{}", row->col(0), VIRTUAL_CONTAINER_SEPARATOR, title));
    if (prefix != LOC_VIRT_PREFIX)
        throw DatabaseException("Tried to build a virtual container path with an non-virtual parentID", LINE_MESSAGE);

    return path.relative_path();
}

bool SQLDatabase::addContainer(int parentContainerId, std::string virtualPath, const std::shared_ptr<CdsContainer>& cont, int* containerID)
{
    log_debug("Adding container for path: {}, lastRefId: {}, containerId: {}", virtualPath.c_str(), cont->getRefID(), containerID ? *containerID : -999);

    if (parentContainerId == INVALID_OBJECT_ID) {
        *containerID = INVALID_OBJECT_ID;
        return false;
    }

    reduceString(virtualPath, VIRTUAL_CONTAINER_SEPARATOR);
    if (virtualPath == std::string(1, VIRTUAL_CONTAINER_SEPARATOR)) {
        *containerID = CDS_ID_ROOT;
        return false;
    }
    std::string dbLocation = addLocationPrefix(cont->isVirtual() ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX, virtualPath);

    beginTransaction("addContainer");
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} AND {} LIMIT 1",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->getClause(BrowseColumn::LocationHash, quote(stringHash(dbLocation)), true),
        browseColumnMapper->getClause(BrowseColumn::Location, quote(dbLocation), true)));
    if (res) {
        auto row = res->nextRow();
        if (row) {
            if (containerID) {
                *containerID = row->col_int(0, INVALID_OBJECT_ID);
                log_debug("Found container for path: {}, lastRefId: {} -> containerId: {}", virtualPath.c_str(), cont->getRefID(), *containerID);
            }
            commit("addContainer");
            return false;
        }
    }
    commit("addContainer");

    if (cont->getMetaData(MetadataFields::M_DATE).empty())
        cont->addMetaData(MetadataFields::M_DATE, grbLocaltime("{:%FT%T%z}", cont->getMTime()));

    *containerID = createContainer(parentContainerId, cont->getTitle(), virtualPath, cont->getFlags(), cont->isVirtual(), cont->getClass(), cont->getFlag(OBJECT_FLAG_PLAYLIST_REF) ? cont->getRefID() : INVALID_OBJECT_ID, cont->getMetaData(), cont->getResources());
    return true;
}

std::string SQLDatabase::addLocationPrefix(char prefix, const fs::path& path, const std::string_view& suffix)
{
    return fmt::format("{}{}{}", prefix, path.string(), suffix);
}

std::pair<fs::path, char> SQLDatabase::stripLocationPrefix(std::string_view dbLocation)
{
    if (dbLocation.empty())
        return { "", LOC_ILLEGAL_PREFIX };

    return { dbLocation.substr(1), dbLocation.at(0) };
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromRow(const std::string& group, const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, BrowseColumn::ObjectType));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, BrowseColumn::Id)));
    obj->setRefID(stoiString(getCol(row, BrowseColumn::RefId)));

    obj->setParentID(std::stoi(getCol(row, BrowseColumn::ParentId)));
    obj->setTitle(getCol(row, BrowseColumn::DcTitle));
    obj->setSortKey(getCol(row, BrowseColumn::SortKey));
    obj->setClass(fallbackString(getCol(row, BrowseColumn::UpnpClass), getCol(row, BrowseColumn::RefUpnpClass)));
    obj->setFlags(std::stoi(getCol(row, BrowseColumn::Flags)));
    obj->setMTime(std::chrono::seconds(stoulString(getCol(row, BrowseColumn::LastModified))));
    obj->setUTime(std::chrono::seconds(stoulString(getCol(row, BrowseColumn::LastUpdated))));

    auto metaData = retrieveMetaDataForObject(obj->getID());
    if (!metaData.empty()) {
        obj->setMetaData(std::move(metaData));
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        metaData = retrieveMetaDataForObject(obj->getRefID());
        if (!metaData.empty())
            obj->setMetaData(std::move(metaData));
    }

    std::string auxdataStr = fallbackString(getCol(row, BrowseColumn::Auxdata), getCol(row, BrowseColumn::RefAuxdata));
    std::map<std::string, std::string> aux = URLUtils::dictDecode(auxdataStr);
    obj->setAuxData(aux);

    bool resourceZeroOk = false;
    auto resources = retrieveResourcesForObject(obj->getID());
    if (!resources.empty()) {
        resourceZeroOk = true;
        obj->setResources(std::move(resources));
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        resources = retrieveResourcesForObject(obj->getRefID());
        if (!resources.empty()) {
            resourceZeroOk = true;
            obj->setResources(std::move(resources));
        }
    }

    obj->setVirtual((obj->getRefID() != CDS_ID_ROOT && obj->isPureItem()) || (obj->isItem() && !obj->isPureItem())); // gets set to true for virtual containers below

    bool matchedType = false;

    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        cont->setUpdateID(std::stoi(getCol(row, BrowseColumn::UpdateId)));
        auto [location, locationPrefix] = stripLocationPrefix(getCol(row, BrowseColumn::Location));
        cont->setLocation(location);
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);

        std::string autoscanPersistent = getCol(row, BrowseColumn::AsPersistent);
        if (!autoscanPersistent.empty()) {
            if (remapBool(autoscanPersistent))
                cont->setAutoscanType(AutoscanType::Config);
            else
                cont->setAutoscanType(AutoscanType::Ui);
        } else
            cont->setAutoscanType(AutoscanType::None);
        matchedType = true;
    } else if (obj->isItem()) {
        if (!resourceZeroOk)
            throw DatabaseException("tried to create object without at least one resource", LINE_MESSAGE);

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(fallbackString(getCol(row, BrowseColumn::MimeType), getCol(row, BrowseColumn::RefMimeType)));
        if (obj->isPureItem()) {
            if (!obj->isVirtual())
                item->setLocation(stripLocationPrefix(getCol(row, BrowseColumn::Location)).first);
            else
                item->setLocation(stripLocationPrefix(getCol(row, BrowseColumn::RefLocation)).first);
        } else { // URLs
            item->setLocation(fallbackString(getCol(row, BrowseColumn::Location), getCol(row, BrowseColumn::RefLocation)));
        }

        item->setTrackNumber(stoiString(getCol(row, BrowseColumn::TrackNumber)));
        item->setPartNumber(stoiString(getCol(row, BrowseColumn::PartNumber)));

        if (!getCol(row, BrowseColumn::RefServiceId).empty())
            item->setServiceID(getCol(row, BrowseColumn::RefServiceId));
        else
            item->setServiceID(getCol(row, BrowseColumn::ServiceId));

        if (!group.empty()) {
            auto playStatus = getPlayStatus(group, obj->getID());
            if (playStatus)
                item->setPlayStatus(playStatus);
        }
        matchedType = true;
    }

    if (!matchedType) {
        throw DatabaseException(fmt::format("Unknown object type: {}", objectType), LINE_MESSAGE);
    }

    return obj;
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromSearchRow(const std::string& group, const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, SearchColumn::ObjectType));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, SearchColumn::Id)));
    obj->setRefID(stoiString(getCol(row, SearchColumn::RefId)));

    obj->setParentID(std::stoi(getCol(row, SearchColumn::ParentId)));
    obj->setTitle(getCol(row, SearchColumn::DcTitle));
    obj->setSortKey(getCol(row, SearchColumn::SortKey));
    obj->setClass(getCol(row, SearchColumn::UpnpClass));
    obj->setFlags(std::stoi(getCol(row, SearchColumn::Flags)));

    auto metaData = retrieveMetaDataForObject(obj->getID());
    if (!metaData.empty())
        obj->setMetaData(std::move(metaData));

    bool resourceZeroOk = false;
    auto resources = retrieveResourcesForObject(obj->getID());
    if (!resources.empty()) {
        resourceZeroOk = true;
        obj->setResources(std::move(resources));
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        resources = retrieveResourcesForObject(obj->getRefID());
        if (!resources.empty()) {
            resourceZeroOk = true;
            obj->setResources(std::move(resources));
        }
    }

    if (obj->isItem()) {
        if (!resourceZeroOk)
            throw DatabaseException("tried to create object without at least one resource", LINE_MESSAGE);

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(getCol(row, SearchColumn::MimeType));
        if (obj->isPureItem()) {
            item->setLocation(stripLocationPrefix(getCol(row, SearchColumn::Location)).first);
        } else { // URLs
            item->setLocation(getCol(row, SearchColumn::Location));
        }

        item->setPartNumber(stoiString(getCol(row, SearchColumn::PartNumber)));
        item->setTrackNumber(stoiString(getCol(row, SearchColumn::TrackNumber)));

        auto playStatus = getPlayStatus(group, obj->getID());
        if (playStatus)
            item->setPlayStatus(playStatus);
    } else if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsItem>(obj);
        cont->setLocation(stripLocationPrefix(getCol(row, SearchColumn::Location)).first);
    } else {
        throw DatabaseException(fmt::format("Unknown object type: {}", objectType), LINE_MESSAGE);
    }

    return obj;
}

std::vector<std::pair<std::string, std::string>> SQLDatabase::retrieveMetaDataForObject(int objectId)
{
    auto query = fmt::format("{} FROM {} WHERE {}",
        sql_meta_query,
        metaColumnMapper->getTableName(),
        metaColumnMapper->getClause(MetadataColumn::ItemId, quote(objectId), true));
    auto res = select(query);
    if (!res)
        return {};

    std::vector<std::pair<std::string, std::string>> metaData;
    metaData.reserve(res->getNumRows());

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        metaData.emplace_back(getCol(row, MetadataColumn::PropertyName), getCol(row, MetadataColumn::PropertyValue));
    }
    return metaData;
}

long long SQLDatabase::getFileStats(const StatsParam& stats)
{
    auto where = std::vector {
        fmt::format("{} != {:d}", searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_CONTAINER),
    };
    if (!stats.getMimeType().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::MimeType), quote(fmt::format("{}%", stats.getMimeType()))));
    }
    if (!stats.getUpnpClass().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::UpnpClass), quote(fmt::format("{}%", stats.getUpnpClass()))));
    }
    where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::Location), quote(fmt::format("{}%", stats.getVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX))));

    std::string mode;
    std::string join;
    switch (stats.getMode()) {
    case StatsParam::StatsMode::Count:
        mode = "COUNT(*)";
        break;
    case StatsParam::StatsMode::Size: {
        join = fmt::format("LEFT JOIN {} ON {} = {}", resourceColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        mode = fmt::format("SUM({}.{})", resourceColumnMapper->getAlias(), identifier("size")); // size is a ResourceAttribute converted to a column
        break;
    }
    }
    auto query = fmt::format("SELECT {} FROM {}{} WHERE {}", mode, searchColumnMapper->tableQuoted(), join, fmt::join(where, " AND "));
    auto res = select(query);
    std::unique_ptr<SQLRow> row;
    if (res && (row = res->nextRow())) {
        return row->col_long(0, 0);
    }
    return 0;
}

std::map<std::string, long long> SQLDatabase::getGroupStats(const StatsParam& stats)
{
    auto where = std::vector {
        fmt::format("{} != {:d}", searchColumnMapper->mapQuoted(SearchColumn::ObjectType), OBJECT_TYPE_CONTAINER),
    };
    if (!stats.getMimeType().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::MimeType), quote(fmt::format("{}%", stats.getMimeType()))));
    }
    if (!stats.getUpnpClass().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::UpnpClass), quote(fmt::format("{}%", stats.getUpnpClass()))));
    }
    where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchColumn::Location), quote(fmt::format("{}%", stats.getVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX))));

    std::string mode;
    std::string join;
    switch (stats.getMode()) {
    case StatsParam::StatsMode::Count:
        mode = "COUNT(*)";
        break;
    case StatsParam::StatsMode::Size: {
        join = fmt::format("LEFT JOIN {} ON {} = {}", resourceColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        mode = fmt::format("SUM({}.{})", resourceColumnMapper->getAlias(), identifier("size"));
        break;
    }
    }
    auto query = fmt::format("SELECT {}, {} FROM {}{} WHERE {} GROUP BY {}", searchColumnMapper->mapQuoted(SearchColumn::UpnpClass), mode, searchColumnMapper->tableQuoted(), join, fmt::join(where, " AND "), searchColumnMapper->mapQuoted(SearchColumn::UpnpClass));
    auto res = select(query);
    std::unique_ptr<SQLRow> row;
    std::map<std::string, long long> result;
    if (res) {
        while ((row = res->nextRow())) {
            result[row->col(0)] = row->col_long(1, 0);
        }
    }
    return result;
}

std::string SQLDatabase::incrementUpdateIDs(const std::unordered_set<int>& ids)
{
    if (ids.empty())
        return {};

    beginTransaction("incrementUpdateIDs");

    exec(fmt::format("UPDATE {0} SET {1} = {1} + 1 WHERE {2} IN ({3})",
        browseColumnMapper->getTableName(),
        browseColumnMapper->mapQuoted(BrowseColumn::UpdateId, true),
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true), fmt::join(ids, ",")));

    auto res = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {0} IN ({3})",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->mapQuoted(BrowseColumn::UpdateId, true),
        browseColumnMapper->getTableName(),
        fmt::join(ids, ",")));
    if (!res) {
        rollback("incrementUpdateIDs 2");
        throw DatabaseException("Error while fetching update ids", LINE_MESSAGE);
    }
    commit("incrementUpdateIDs 2");

    std::unique_ptr<SQLRow> row;
    std::vector<std::string> rows;
    rows.reserve(res->getNumRows());
    while ((row = res->nextRow())) {
        rows.push_back(fmt::format("{},{}", row->col(0), row->col(1)));
    }

    if (rows.empty())
        return {};

    return fmt::format("{}", fmt::join(rows, ","));
}

std::size_t SQLDatabase::getObjects(int parentID, bool withoutContainer, std::unordered_set<int>& ret, bool full)
{
    auto getSql = fmt::format("SELECT {}, {} FROM {} WHERE {}",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->getClause(BrowseColumn::ParentId, parentID, true));
    auto res = select(getSql);
    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", browseColumnMapper->getTableName()), LINE_MESSAGE);

    if (res->getNumRows() == 0)
        return 0;

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        if ((!withoutContainer && !full) || row->col_int(1, OBJECT_TYPE_CONTAINER) != OBJECT_TYPE_CONTAINER)
            ret.insert(row->col_int(0, INVALID_OBJECT_ID));
        else if (!withoutContainer && full)
            getObjects(row->col_int(0, INVALID_OBJECT_ID), withoutContainer, ret, full);
    }
    return ret.size();
}

std::vector<int> SQLDatabase::getRefObjects(int objectId)
{
    auto getSql = fmt::format("SELECT {}, {} FROM {} WHERE {} AND {} != {}",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->mapQuoted(BrowseColumn::LastUpdated, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->getClause(BrowseColumn::RefId, objectId, true),
        browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true), OBJECT_TYPE_CONTAINER);

    auto res = select(getSql);
    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", browseColumnMapper->getTableName()), LINE_MESSAGE);

    std::vector<int> result;
    if (res->getNumRows() == 0)
        return result;

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        result.emplace_back(row->col_int(0, INVALID_OBJECT_ID));
    }
    return result;
}

std::unordered_set<int> SQLDatabase::getUnreferencedObjects()
{
    auto colId = browseColumnMapper->mapQuoted(BrowseColumn::Id, true);
    auto table = browseColumnMapper->getTableName();
    auto colRefId = browseColumnMapper->mapQuoted(BrowseColumn::RefId, true);

    auto getSql = fmt::format("SELECT {} FROM {} WHERE {} IS NOT NULL AND {} NOT IN (SELECT {} FROM {})", colId, table, colRefId, colRefId, colId, table);
    auto res = select(getSql);
    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", table), LINE_MESSAGE);

    std::unordered_set<int> ret;
    if (res->getNumRows() == 0)
        return ret;

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        ret.insert(row->col_int(0, INVALID_OBJECT_ID));
    }
    return ret;
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObjects(const std::unordered_set<int>& list, bool all)
{
    std::size_t count = list.size();
    if (count == 0)
        return nullptr;

    auto it = std::find_if(list.begin(), list.end(), IS_FORBIDDEN_CDS_ID);
    if (it != list.end()) {
        throw DatabaseException(fmt::format("Tried to delete a forbidden ID ({})", *it), LINE_MESSAGE);
    }

    auto res = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {0} IN ({3})",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true),
        browseColumnMapper->getTableName(),
        fmt::join(list, ",")));
    if (!res)
        throw DatabaseException(fmt::format("error selecting form {}", CDS_OBJECT_TABLE), LINE_MESSAGE);

    std::vector<std::int32_t> items;
    std::vector<std::int32_t> containers;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        const std::int32_t objectID = row->col_int(0, INVALID_OBJECT_ID);
        const int objectType = row->col_int(1, 0);
        if (IS_CDS_CONTAINER(objectType))
            containers.push_back(objectID);
        else
            items.push_back(objectID);
    }

    auto rr = _recursiveRemove(items, containers, all);
    return _purgeEmptyContainers(std::move(rr));
}

void SQLDatabase::_removeObjects(const std::vector<std::int32_t>& objectIDs)
{
    auto sel = fmt::format("SELECT {}, {}, {} FROM {} JOIN {} ON {} = {} WHERE {} IN ({})",
        asColumnMapper->mapQuoted(ASColumn::Id),
        asColumnMapper->mapQuoted(ASColumn::Persistent),
        browseColumnMapper->mapQuoted(BrowseColumn::Location),
        asColumnMapper->tableQuoted(),
        browseColumnMapper->tableQuoted(),
        browseColumnMapper->mapQuoted(BrowseColumn::Id),
        asColumnMapper->mapQuoted(ASColumn::ObjId),
        browseColumnMapper->mapQuoted(BrowseColumn::Id),
        fmt::join(objectIDs, ","));
    log_debug("{}", sel);

    beginTransaction("_removeObjects");
    auto res = select(sel);
    if (res) {
        log_debug("relevant autoscans!");
        std::vector<int> deleteAs;
        std::unique_ptr<SQLRow> row;
        while ((row = res->nextRow())) {
            const int colId = row->col_int(0, INVALID_OBJECT_ID); // ASColumn::id
            bool persistent = remapBool(row->col_int(1, 0));
            if (persistent) {
                auto location = std::get<0>(stripLocationPrefix(row->col(2)));
                auto fields = std::map<AutoscanColumn, std::string> {
                    { AutoscanColumn::Id, quote(colId) },
                    { AutoscanColumn::ObjId, SQL_NULL },
                    { AutoscanColumn::Location, quote(location.string()) },
                };
                Autoscan2Table at(fields, Operation::Update, autoscanColumnMapper);
                exec(AUTOSCAN_TABLE, at.sqlForUpdate(nullptr), colId);
            } else {
                deleteAs.push_back(colId);
            }
            log_debug("relevant autoscan: {}; persistent: {}", colId, persistent);
        }

        if (!deleteAs.empty()) {
            deleteRows(AUTOSCAN_TABLE, "id", deleteAs);
            log_debug("deleting autoscans: {}", fmt::to_string(fmt::join(deleteAs, ", ")));
        }
    }

    deleteRows(CDS_OBJECT_TABLE, "id", objectIDs);
    del(RESOURCE_TABLE, fmt::format("{} IN ('{}')", identifier(EnumMapper::getAttributeName(ResourceAttribute::FANART_OBJ_ID)), fmt::join(objectIDs, "','")), objectIDs);
    commit("_removeObjects");
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObject(int objectID, const fs::path& path, bool all)
{
    auto res = select(fmt::format("SELECT {}, {} FROM {} WHERE {} LIMIT 1",
        browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true),
        browseColumnMapper->mapQuoted(BrowseColumn::RefId, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->getClause(BrowseColumn::Id, objectID, true)));
    if (!res)
        return nullptr;

    auto row = res->nextRow();
    if (!row)
        return nullptr;

    const int objectType = row->col_int(0, 0);
    bool isContainer = IS_CDS_CONTAINER(objectType);
    log_vdebug("Removing {} from {}", row->col(2), row->col(3));
    if (all && !isContainer) {
        if (!row->isNullOrEmpty(1)) {
            const int refId = row->col_int(1, INVALID_OBJECT_ID);
            if (!IS_FORBIDDEN_CDS_ID(refId))
                objectID = refId;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw DatabaseException(fmt::format("Tried to delete a forbidden ID ({})", objectID), LINE_MESSAGE);
    std::vector<std::int32_t> itemIds;
    std::vector<std::int32_t> containerIds;
    if (isContainer) {
        containerIds.push_back(objectID);
    } else {
        itemIds.push_back(objectID);
    }
    auto changedContainers = _recursiveRemove(itemIds, containerIds, all);
    if (!path.empty())
        del(RESOURCE_TABLE, fmt::format("{} = {}", identifier(EnumMapper::getAttributeName(ResourceAttribute::RESOURCE_FILE)), quote(path.string())), {});
    return _purgeEmptyContainers(std::move(changedContainers));
}

Database::ChangedContainers SQLDatabase::_recursiveRemove(
    const std::vector<std::int32_t>& items,
    const std::vector<std::int32_t>& containers,
    bool all)
{
    log_debug("start");

    ChangedContainers changedContainers;

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

    auto itemIds = std::vector(items);
    auto containerIds = std::vector(containers);
    auto parentIds = std::vector(items);
    auto removeIds = std::vector(containers);

    // select statements
    auto parentSql = fmt::format("SELECT DISTINCT {} FROM {} WHERE {} IN",
        browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true));
    auto itemSql = fmt::format("SELECT DISTINCT {}, {} FROM {} WHERE {} IN",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true),
        browseColumnMapper->getTableName(),
        browseColumnMapper->mapQuoted(BrowseColumn::RefId, true));
    auto containersSql = all //
        ? fmt::format("SELECT DISTINCT {}, {}, {} FROM {} WHERE {} IN",
              browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
              browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true),
              browseColumnMapper->mapQuoted(BrowseColumn::RefId, true),
              browseColumnMapper->getTableName(),
              browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true)) //
        : fmt::format("SELECT DISTINCT {}, {} FROM {} WHERE {} IN",
              browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
              browseColumnMapper->mapQuoted(BrowseColumn::ObjectType, true),
              browseColumnMapper->getTableName(),
              browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true));

    // collect container for update signals
    if (!containers.empty()) {
        std::copy(containers.begin(), containers.end(), std::back_inserter(parentIds));
        auto sql = fmt::format("{} ({})", parentSql, fmt::join(parentIds, ","));
        res = select(sql);
        if (!res)
            throw DatabaseException(fmt::format("Sql error: {}", sql), LINE_MESSAGE);
        parentIds.clear();
        while ((row = res->nextRow())) {
            changedContainers.ui.push_back(row->col_int(0, INVALID_OBJECT_ID));
        }
    }

    int count = 0;
    while (!itemIds.empty() || !parentIds.empty() || !containerIds.empty()) {
        // collect child entries
        if (!parentIds.empty()) {
            // add ids to remove
            std::copy(parentIds.begin(), parentIds.end(), std::back_inserter(removeIds));
            auto sql = fmt::format("{} ({})", parentSql, fmt::join(parentIds, ","));
            res = select(sql);
            if (!res)
                throw DatabaseException(fmt::format("Sql error: {}", sql), LINE_MESSAGE);
            parentIds.clear();
            while ((row = res->nextRow())) {
                changedContainers.upnp.push_back(row->col_int(0, INVALID_OBJECT_ID));
            }
        }

        // collect entries
        if (!itemIds.empty()) {
            auto sql = fmt::format("{} ({})", itemSql, fmt::join(itemIds, ","));
            res = select(sql);
            if (!res)
                throw DatabaseException(fmt::format("Sql error: {}", sql), LINE_MESSAGE);
            itemIds.clear();
            while ((row = res->nextRow())) {
                removeIds.push_back(row->col_int(0, INVALID_OBJECT_ID));
                changedContainers.upnp.push_back(row->col_int(1, INVALID_OBJECT_ID));
            }
        }

        // collect entries in container
        if (!containerIds.empty()) {
            auto sql = fmt::format("{} ({})", containersSql, fmt::join(containerIds, ","));
            res = select(sql);
            if (!res)
                throw DatabaseException(fmt::format("Sql error: {}", sql), LINE_MESSAGE);
            containerIds.clear();
            while ((row = res->nextRow())) {
                const int objId = row->col_int(0, INVALID_OBJECT_ID);
                const int objType = row->col_int(1, 0);
                if (IS_CDS_CONTAINER(objType)) {
                    containerIds.push_back(objId);
                    removeIds.push_back(objId);
                } else {
                    if (all) {
                        if (!row->isNullOrEmpty(2)) {
                            const std::int32_t refId = row->col_int(2, INVALID_OBJECT_ID);
                            parentIds.push_back(refId);
                            itemIds.push_back(refId);
                            removeIds.push_back(refId);
                        } else {
                            removeIds.push_back(objId);
                            itemIds.push_back(objId);
                        }
                    } else {
                        removeIds.push_back(objId);
                        itemIds.push_back(objId);
                    }
                }
            }
        }

        // remove entries found
        if (removeIds.size() > MAX_REMOVE_SIZE) {
            _removeObjects(removeIds);
            removeIds.clear();
        }

        if (count++ > MAX_REMOVE_RECURSION)
            throw DatabaseException("There seems to be an infinite loop...", LINE_MESSAGE);
    }

    if (!removeIds.empty())
        _removeObjects(removeIds);
    log_debug("end");
    return changedContainers;
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::_purgeEmptyContainers(ChangedContainers maybeEmpty)
{
    log_debug("start upnp: {}; ui: {}", fmt::to_string(fmt::join(maybeEmpty.upnp, ",")), fmt::to_string(fmt::join(maybeEmpty.ui, ",")));
    auto changedContainers = std::make_unique<ChangedContainers>();
    if (maybeEmpty.upnp.empty() && maybeEmpty.ui.empty())
        return changedContainers;

    // Create temporary mapper
    constexpr const char* folAlias = "fol";
    constexpr const char* cldAlias = "cld";
    static const std::map<BrowseColumn, SearchProperty> purgeColMap {
        { BrowseColumn::Id, { folAlias, browseColMap.at(BrowseColumn::Id).field } },
        { BrowseColumn::ParentId, { folAlias, browseColMap.at(BrowseColumn::ParentId).field } },
        { BrowseColumn::ObjectType, { folAlias, browseColMap.at(BrowseColumn::ObjectType).field } },
        { BrowseColumn::Flags, { folAlias, browseColMap.at(BrowseColumn::Flags).field } },
        { BrowseColumn::RefId, { cldAlias, browseColMap.at(BrowseColumn::ParentId).field } }, // use RefId to store cld.parent_id
    };
    static const std::vector<std::pair<std::string, BrowseColumn>> purgeSortMap {};
    auto pcm = EnumColumnMapper<BrowseColumn>(table_quote_begin, table_quote_end, folAlias, CDS_OBJECT_TABLE, purgeSortMap, purgeColMap);

    auto fields = std::vector {
        pcm.mapQuoted(BrowseColumn::Id),
        fmt::format("COUNT({})", pcm.mapQuoted(BrowseColumn::RefId)),
        pcm.mapQuoted(BrowseColumn::ParentId),
        pcm.mapQuoted(BrowseColumn::Flags),
    };
    // prepare select statement
    std::string selectSql = fmt::format("SELECT {0} FROM {1} {2} LEFT JOIN {1} {3} ON {4} = {5} WHERE {6} AND {4}",
        fmt::join(fields, ","),
        pcm.getTableName(),
        folAlias,
        cldAlias,
        pcm.mapQuoted(BrowseColumn::Id), pcm.mapQuoted(BrowseColumn::RefId),
        pcm.getClause(BrowseColumn::ObjectType, quote(OBJECT_TYPE_CONTAINER)));

    std::vector<std::int32_t> del;
    std::unique_ptr<SQLRow> row;

    auto selUi = std::vector(maybeEmpty.ui);
    auto selUpnp = std::vector(maybeEmpty.upnp);

    bool again;
    int count = 0;
    do {
        again = false;

        // find deletable containers for upnp
        if (!selUpnp.empty()) {
            auto sql = fmt::format("{} IN ({}) GROUP BY {}", selectSql, fmt::join(selUpnp, ","), pcm.mapQuoted(BrowseColumn::Id));
            log_debug("upnp-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUpnp.clear();
            if (!res)
                throw DatabaseException(fmt::format("error selecting form {}", CDS_OBJECT_TABLE), LINE_MESSAGE);
            while ((row = res->nextRow())) {
                const int flags = row->col_int(3, 0);
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers->upnp.push_back(row->col_int(0, INVALID_OBJECT_ID));
                else if (row->col(1) == "0") {
                    del.push_back(row->col_int(0, INVALID_OBJECT_ID));
                    selUi.push_back(row->col_int(2, INVALID_OBJECT_ID));
                } else {
                    selUpnp.push_back(row->col_int(0, INVALID_OBJECT_ID));
                }
            }
        }

        // find deletable containers from ui
        if (!selUi.empty()) {
            auto sql = fmt::format("{} IN ({}) GROUP BY {}", selectSql, fmt::join(selUi, ","), pcm.mapQuoted(BrowseColumn::Id));
            log_debug("ui-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUi.clear();
            if (!res)
                throw DatabaseException(fmt::format("error selecting form {}", CDS_OBJECT_TABLE), LINE_MESSAGE);
            while ((row = res->nextRow())) {
                const int flags = row->col_int(3, 0);
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER) {
                    changedContainers->ui.push_back(row->col_int(0, INVALID_OBJECT_ID));
                    changedContainers->upnp.push_back(row->col_int(0, INVALID_OBJECT_ID));
                } else if (row->col(1) == "0") {
                    del.push_back(row->col_int(0, INVALID_OBJECT_ID));
                    selUi.push_back(row->col_int(2, INVALID_OBJECT_ID));
                } else {
                    selUi.push_back(row->col_int(0, INVALID_OBJECT_ID));
                }
            }
        }

        // delete everything
        log_vdebug("selecting: {}; removing: {}", selectSql, fmt::join(del, ","));
        if (!del.empty()) {
            _removeObjects(del);
            del.clear();
            if (!selUi.empty() || !selUpnp.empty())
                again = true;
        }
        if (count++ >= MAX_REMOVE_RECURSION)
            throw DatabaseException("there seems to be an infinite loop...", LINE_MESSAGE);
    } while (again);

    // get list of updated containers
    auto& changedUi = changedContainers->ui;
    auto& changedUpnp = changedContainers->upnp;
    if (!selUi.empty()) {
        std::copy(selUi.begin(), selUi.end(), std::back_inserter(changedUi));
        std::copy(selUi.begin(), selUi.end(), std::back_inserter(changedUpnp));
    }
    if (!selUpnp.empty()) {
        std::copy(selUpnp.begin(), selUpnp.end(), std::back_inserter(changedUpnp));
    }
    log_vdebug("end; changedContainers (upnp): {}", fmt::join(changedUpnp, ","));
    log_debug("end; changedContainers (upnp): {}", changedUpnp.size());
    log_vdebug("end; changedContainers (ui): {}", fmt::join(changedUi, ","));
    log_debug("end; changedContainers (ui): {}", changedUi.size());

    return changedContainers;
}

std::string SQLDatabase::getInternalSetting(const std::string& key)
{
    auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} = {3} LIMIT 1",
        identifier("value"), identifier(INTERNAL_SETTINGS_TABLE), identifier("key"), quote(key)));
    if (!res)
        return {};

    auto row = res->nextRow();
    if (!row)
        return {};
    return row->col(0);
}

/* config methods */
std::vector<ConfigValue> SQLDatabase::getConfigValues()
{
    std::vector<std::string> fields;
    fields.reserve(configColumnMap.size());
    for (auto&& [key, col] : configColumnMap) {
        fields.push_back(fmt::format("{}.{}", identifier(col.alias), identifier(col.field)));
    }
    std::vector<ConfigValue> result;
    auto res = select(fmt::format("SELECT {} FROM {}", fmt::join(fields, ", "), configColumnMapper->tableQuoted()));
    if (!res)
        return result;

    result.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        result.push_back({ //
            getCol(row, ConfigColumn::Key),
            getCol(row, ConfigColumn::Item),
            getCol(row, ConfigColumn::ItemValue),
            getCol(row, ConfigColumn::Status) });
    }

    log_debug("loaded {} items from {}", result.size(), CONFIG_VALUE_TABLE);
    return result;
}

void SQLDatabase::removeConfigValue(const std::string& item)
{
    log_info("Deleting config item '{}'", item);
    if (item == "*") {
        deleteAll(CONFIG_VALUE_TABLE);
    } else {
        deleteRow(CONFIG_VALUE_TABLE, "item", item);
    }
}

void SQLDatabase::updateConfigValue(
    const std::string& key,
    const std::string& item,
    const std::string& value,
    const std::string& status)
{
    auto res = select(fmt::format("SELECT 1 FROM {} WHERE {} LIMIT 1",
        configColumnMapper->getTableName(),
        configColumnMapper->getClause(ConfigColumn::Item, quote(item), true)));
    if (!res || res->getNumRows() == 0) {
        auto dict = std::map<ConfigColumn, std::string> {
            { ConfigColumn::Key, quote(key) },
            { ConfigColumn::Item, quote(item) },
            { ConfigColumn::ItemValue, quote(value) },
            { ConfigColumn::Status, quote(status) },
        };
        Config2Table ct(std::move(dict), Operation::Insert, configColumnMapper);
        execOnly(ct.sqlForInsert(nullptr));
        log_debug("inserted for {} as {} = {}", key, item, value);
    } else {
        auto dict = std::map<ConfigColumn, std::string> {
            { ConfigColumn::Item, quote(item) },
            { ConfigColumn::ItemValue, quote(value) },
        };
        Config2Table ct(std::move(dict), Operation::Update, configColumnMapper);
        execOnly(ct.sqlForUpdate(nullptr));
        log_debug("updated for {} as {} = {}", key, item, value);
    }
}

/* clients methods */
std::vector<ClientObservation> SQLDatabase::getClients()
{
    std::vector<std::string> fields;
    fields.reserve(clientColumnMap.size());
    for (auto&& [key, col] : clientColumnMap) {
        fields.push_back(fmt::format("{}", identifier(col.field)));
    }

    std::vector<ClientObservation> result;
    auto res = select(fmt::format("SELECT {} FROM {}", fmt::join(fields, ", "), clientColumnMapper->getTableName()));
    if (!res)
        return result;

    result.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto net = std::make_shared<GrbNet>(getCol(row, ClientColumn::Addr), getColInt(row, ClientColumn::AddrFamily, AF_INET));
        net->setPort(getColInt(row, ClientColumn::Port, 0));
        result.emplace_back(net,
            getCol(row, ClientColumn::UserAgent),
            std::chrono::seconds(getColInt(row, ClientColumn::Last, 0)),
            std::chrono::seconds(getColInt(row, ClientColumn::Age, 0)),
            nullptr);
    }

    log_debug("Loaded {} items from {}", result.size(), CLIENTS_TABLE);
    return result;
}

void SQLDatabase::saveClients(const std::vector<ClientObservation>& cache)
{
    deleteAll(CLIENTS_TABLE);
    auto multiDict = std::vector<std::map<ClientColumn, std::string>>();
    for (auto& client : cache) {
        auto dict = std::map<ClientColumn, std::string> {
            { ClientColumn::Addr, quote(client.addr->getNameInfo(false)) },
            { ClientColumn::Port, quote(client.addr->getPort()) },
            { ClientColumn::AddrFamily, quote(client.addr->getAdressFamily()) },
            { ClientColumn::UserAgent, quote(client.userAgent) },
            { ClientColumn::Last, quote(client.last.count()) },
            { ClientColumn::Age, quote(client.age.count()) },
        };
        multiDict.push_back(std::move(dict));
    }
    Client2Table ct(std::move(multiDict), clientColumnMapper);
    execOnly(ct.sqlForMultiInsert(nullptr));
}

std::shared_ptr<ClientStatusDetail> SQLDatabase::getPlayStatus(const std::string& group, int objectId)
{
    std::vector<std::string> fields;
    fields.reserve(playstatusColMap.size());
    for (auto&& [key, col] : playstatusColMap) {
        fields.push_back(fmt::format("{}", playstatusColumnMapper->mapQuoted(key)));
    }
    std::vector<std::string> where {
        playstatusColumnMapper->getClause(PlaystatusColumn::Group, quote(group)),
        playstatusColumnMapper->getClause(PlaystatusColumn::ItemId, quote(objectId)),
    };
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {}",
        fmt::join(fields, ", "),
        playstatusColumnMapper->tableQuoted(),
        fmt::join(where, " AND ")));
    if (!res)
        return {};

    std::unique_ptr<SQLRow> row;
    if ((row = res->nextRow())) {
        log_debug("Loaded {},{} items from {}", group, objectId, PLAYSTATUS_TABLE);
        return std::make_shared<ClientStatusDetail>(
            getCol(row, PlaystatusColumn::Group),
            getColInt(row, PlaystatusColumn::ItemId, objectId),
            getColInt(row, PlaystatusColumn::PlayCount, 0),
            getColInt(row, PlaystatusColumn::LastPlayed, 0),
            getColInt(row, PlaystatusColumn::LastPlayedPosition, 0),
            getColInt(row, PlaystatusColumn::BookMarkPosition, 0));
    }

    return {};
}

std::vector<std::shared_ptr<ClientStatusDetail>> SQLDatabase::getPlayStatusList(int objectId)
{
    std::vector<std::string> fields;
    fields.reserve(playstatusColMap.size());
    for (auto&& [key, col] : playstatusColMap) {
        fields.push_back(fmt::format("{}", playstatusColumnMapper->mapQuoted(key)));
    }
    std::vector<std::string> where {
        playstatusColumnMapper->getClause(PlaystatusColumn::ItemId, quote(objectId)),
    };
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {}",
        fmt::join(fields, ", "),
        playstatusColumnMapper->tableQuoted(),
        fmt::join(where, " AND ")));
    if (!res)
        return {};

    std::vector<std::shared_ptr<ClientStatusDetail>> result;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        log_debug("Loaded {},{} items from {}", row->col(0), objectId, PLAYSTATUS_TABLE);
        auto status = std::make_shared<ClientStatusDetail>(
            getCol(row, PlaystatusColumn::Group),
            getColInt(row, PlaystatusColumn::ItemId, objectId),
            getColInt(row, PlaystatusColumn::PlayCount, 0),
            getColInt(row, PlaystatusColumn::LastPlayed, 0),
            getColInt(row, PlaystatusColumn::LastPlayedPosition, 0),
            getColInt(row, PlaystatusColumn::BookMarkPosition, 0));
        result.push_back(std::move(status));
    }

    return result;
}

void SQLDatabase::savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail)
{
    beginTransaction("savePlayStatus");

    std::vector<std::string> where {
        playstatusColumnMapper->getClause(PlaystatusColumn::Group, quote(detail->getGroup()), true),
        playstatusColumnMapper->getClause(PlaystatusColumn::ItemId, quote(detail->getItemId()), true),
    };
    auto res = select(fmt::format("SELECT 1 FROM {} WHERE {} LIMIT 1",
        playstatusColumnMapper->getTableName(),
        fmt::join(where, " AND ")));
    auto doUpdate = res && res->getNumRows() > 0;

    if (doUpdate) {
        auto dict = std::map<PlaystatusColumn, std::string> {
            { PlaystatusColumn::PlayCount, quote(detail->getPlayCount()) },
            { PlaystatusColumn::LastPlayed, quote(detail->getLastPlayed().count()) },
            { PlaystatusColumn::LastPlayedPosition, quote(detail->getLastPlayedPosition().count()) },
            { PlaystatusColumn::BookMarkPosition, quote(detail->getBookMarkPosition().count()) },
        };
        Playstatus2Table pt(std::move(dict), Operation::Update, playstatusColumnMapper);
        execOnly(pt.sqlForUpdate(detail));
    } else {
        auto dict = std::map<PlaystatusColumn, std::string> {
            { PlaystatusColumn::Group, quote(detail->getGroup()) },
            { PlaystatusColumn::ItemId, quote(detail->getItemId()) },
            { PlaystatusColumn::PlayCount, quote(detail->getPlayCount()) },
            { PlaystatusColumn::LastPlayed, quote(detail->getLastPlayed().count()) },
            { PlaystatusColumn::LastPlayedPosition, quote(detail->getLastPlayedPosition().count()) },
            { PlaystatusColumn::BookMarkPosition, quote(detail->getBookMarkPosition().count()) },
        };
        Playstatus2Table pt(std::move(dict), Operation::Insert, playstatusColumnMapper);
        execOnly(pt.sqlForInsert(detail));
    }

    commit("savePlayStatus");
}

std::vector<std::map<std::string, std::string>> SQLDatabase::getClientGroupStats()
{
    auto res = select(fmt::format("SELECT {}, COUNT(*), SUM({}), MAX({}), COUNT(NULLIF({},0)) FROM {} GROUP BY {}",
        playstatusColumnMapper->mapQuoted(PlaystatusColumn::Group),
        playstatusColumnMapper->mapQuoted(PlaystatusColumn::PlayCount),
        playstatusColumnMapper->mapQuoted(PlaystatusColumn::LastPlayed),
        playstatusColumnMapper->mapQuoted(PlaystatusColumn::BookMarkPosition),
        playstatusColumnMapper->tableQuoted(),
        playstatusColumnMapper->mapQuoted(PlaystatusColumn::Group)));
    if (!res)
        return {};

    std::vector<std::map<std::string, std::string>> result;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        log_debug("Loaded {} stats from {}", row->col(0), PLAYSTATUS_TABLE);
        std::map<std::string, std::string> stats;
        stats["name"] = row->col(0);
        stats["count"] = fmt::format("{}", row->col_int(1, -1));
        stats["playCount"] = fmt::format("{}", row->col_int(2, -1));
        stats["last"] = grbLocaltime("{:%a %b %d %H:%M:%S %Y}", std::chrono::seconds(row->col_int(3, 0)));
        stats["bookmarks"] = fmt::format("{}", row->col_int(4, -1));
        result.push_back(std::move(stats));
    }
    return result;
}

void SQLDatabase::updateAutoscanList(AutoscanScanMode scanmode, const std::shared_ptr<AutoscanList>& list)
{
    log_debug("Setting persistent autoscans untouched - scanmode: {};", AutoscanDirectory::mapScanmode(scanmode));

    beginTransaction("updateAutoscanList");
    auto dict = std::map<AutoscanColumn, std::string> {
        { AutoscanColumn::Touched, quote(false) },
    };
    auto whereDict = std::map<AutoscanColumn, std::string> {
        { AutoscanColumn::Persistent, quote(true) },
        { AutoscanColumn::ScanMode, quote(AutoscanDirectory::mapScanmode(scanmode)) },
    };
    Autoscan2Table at(dict, Operation::Update, autoscanColumnMapper);
    exec(at.sqlForUpdateAll(whereDict));
    commit("updateAutoscanList");

    std::size_t listSize = list->size();
    log_debug("updating/adding persistent autoscans (count: {})", listSize);
    for (std::size_t i = 0; i < listSize; i++) {
        log_debug("getting ad {} from list..", i);
        auto ad = list->get(i);
        if (!ad)
            continue;

        // only persistent asD should be given to getAutoscanList
        assert(ad->persistent());
        // the scanmode should match the given parameter
        assert(ad->getScanMode() == scanmode);

        fs::path location = ad->getLocation();
        if (location.empty())
            throw DatabaseException("AutoscanDirectoy with illegal location given to SQLDatabase::updateAutoscanPersistentList", LINE_MESSAGE);

        int objectID = findObjectIDByPath(location);
        log_debug("objectID = {}", objectID);
        auto where = (objectID == INVALID_OBJECT_ID)
            ? autoscanColumnMapper->getClause(AutoscanColumn::Location, quote(location))
            : autoscanColumnMapper->getClause(AutoscanColumn::ObjId, quote(objectID));

        beginTransaction("updateAutoscanList x");
        auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} LIMIT 1",
            autoscanColumnMapper->mapQuoted(AutoscanColumn::Id), autoscanColumnMapper->tableQuoted(), where));
        if (!res) {
            rollback("updateAutoscanList x");
            throw DatabaseException("query error while selecting from autoscan list", LINE_MESSAGE);
        }
        commit("updateAutoscanList x");

        auto row = res->nextRow();
        if (row) {
            ad->setDatabaseID(row->col_int(0, INVALID_OBJECT_ID));
            updateAutoscanDirectory(ad);
        } else
            addAutoscanDirectory(ad);
    }

    beginTransaction("updateAutoscanList delete");
    whereDict = std::map<AutoscanColumn, std::string> {
        { AutoscanColumn::Touched, quote(false) },
        { AutoscanColumn::ScanMode, quote(AutoscanDirectory::mapScanmode(scanmode)) },
    };
    dict = {};
    at = Autoscan2Table(dict, Operation::Delete, autoscanColumnMapper);
    del(AUTOSCAN_TABLE, at.sqlForDeleteAll(whereDict), {});
    commit("updateAutoscanList delete");
}

std::shared_ptr<AutoscanList> SQLDatabase::getAutoscanList(AutoscanScanMode scanmode)
{
    std::string selectSql = fmt::format("{} WHERE {}",
        sql_autoscan_query,
        autoscanColumnMapper->getClause(AutoscanColumn::ScanMode, quote(AutoscanDirectory::mapScanmode(scanmode))));

    auto res = select(selectSql);
    if (!res)
        throw DatabaseException("query error while fetching autoscan list", LINE_MESSAGE);

    auto ret = std::make_shared<AutoscanList>();
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto adir = _fillAutoscanDirectory(row);
        if (!adir)
            _removeAutoscanDirectory(row->col_int(0, INVALID_OBJECT_ID));
        else
            ret->add(adir);
    }
    return ret;
}

std::shared_ptr<AutoscanDirectory> SQLDatabase::getAutoscanDirectory(int objectID)
{
    std::string selectSql = fmt::format("{} WHERE {}", sql_autoscan_query, autoscanColumnMapper->getClause(AutoscanColumn::ItemId, quote(objectID)));

    auto res = select(selectSql);
    if (!res)
        throw DatabaseException("query error while fetching autoscan", LINE_MESSAGE);

    auto row = res->nextRow();
    if (!row)
        return nullptr;

    return _fillAutoscanDirectory(row);
}

std::shared_ptr<AutoscanDirectory> SQLDatabase::_fillAutoscanDirectory(const std::unique_ptr<SQLRow>& row)
{
    int objectID = INVALID_OBJECT_ID;
    std::string objectIDstr = getCol(row, AutoscanColumn::ObjId);
    if (!objectIDstr.empty())
        objectID = std::stoi(objectIDstr);
    int databaseID = std::stoi(getCol(row, AutoscanColumn::Id));

    fs::path location;
    if (objectID == INVALID_OBJECT_ID) {
        location = getCol(row, AutoscanColumn::Location);
    } else {
        auto [l, prefix] = stripLocationPrefix(getCol(row, AutoscanColumn::ObjLocation));
        location = l;
        if (prefix != LOC_DIR_PREFIX)
            return nullptr;
    }

    AutoscanScanMode mode = AutoscanDirectory::remapScanmode(getCol(row, AutoscanColumn::ScanMode));
    bool recursive = remapBool(getCol(row, AutoscanColumn::Recursive));
    int mt = getColInt(row, AutoscanColumn::MediaType, 0);
    bool hidden = remapBool(getCol(row, AutoscanColumn::Hidden));
    bool followSymlinks = remapBool(getCol(row, AutoscanColumn::FollowSymlinks));
    bool persistent = remapBool(getCol(row, AutoscanColumn::Persistent));
    int retryCount = getColInt(row, AutoscanColumn::RetryCount, 0);
    bool dirTypes = remapBool(getCol(row, AutoscanColumn::DirTypes));
    bool forceRescan = remapBool(getCol(row, AutoscanColumn::ForceRescan));
    auto containerMap = AutoscanDirectory::ContainerTypesDefaults;
    containerMap[AutoscanMediaMode::Audio] = getCol(row, AutoscanColumn::CtAudio);
    containerMap[AutoscanMediaMode::Image] = getCol(row, AutoscanColumn::CtImage);
    containerMap[AutoscanMediaMode::Video] = getCol(row, AutoscanColumn::CtVideo);
    int interval = 0;
    if (mode == AutoscanScanMode::Timed)
        interval = std::stoi(getCol(row, AutoscanColumn::Interval));
    auto lastModified = std::chrono::seconds(std::stol(getCol(row, AutoscanColumn::LastModified)));

    log_info("Loading autoscan location: {}; recursive: {}, mt: {}/{}, last_modified: {}", location.c_str(), recursive, mt, AutoscanDirectory::mapMediaType(mt), lastModified > std::chrono::seconds::zero() ? grbLocaltime("{:%Y-%m-%d %H:%M:%S}", lastModified) : "unset");

    auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, persistent, interval, hidden, followSymlinks, mt, containerMap);
    dir->setObjectID(objectID);
    dir->setDatabaseID(databaseID);
    dir->setRetryCount(retryCount);
    dir->setDirTypes(dirTypes);
    dir->setForceRescan(forceRescan);
    dir->setCurrentLMT("", lastModified);
    if (lastModified > std::chrono::seconds::zero()) {
        dir->setCurrentLMT(location, lastModified);
    }
    dir->updateLMT();
    return dir;
}

void SQLDatabase::addAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw DatabaseException("addAutoscanDirectory called with adir==nullptr", LINE_MESSAGE);
    if (adir->getDatabaseID() >= 0)
        throw DatabaseException("tried to add autoscan directory with a database id set", LINE_MESSAGE);
    int objectID = (adir->getLocation() == FS_ROOT_DIRECTORY) ? CDS_ID_FS_ROOT : findObjectIDByPath(adir->getLocation());
    if (!adir->persistent() && objectID < 0)
        throw DatabaseException("tried to add non-persistent autoscan directory with an illegal objectID or location", LINE_MESSAGE);

    auto pathIds = _checkOverlappingAutoscans(adir);

    _autoscanChangePersistentFlag(objectID, true);

    auto fields = std::map<AutoscanColumn, std::string> {
        { AutoscanColumn::ObjId, objectID >= 0 ? quote(objectID) : SQL_NULL },
        { AutoscanColumn::ScanMode, quote(AutoscanDirectory::mapScanmode(adir->getScanMode())) },
        { AutoscanColumn::Recursive, quote(adir->getRecursive()) },
        { AutoscanColumn::MediaType, quote(adir->getMediaType()) },
        { AutoscanColumn::Hidden, quote(adir->getHidden()) },
        { AutoscanColumn::FollowSymlinks, quote(adir->getFollowSymlinks()) },
        { AutoscanColumn::CtAudio, quote(adir->getContainerTypes().at(AutoscanMediaMode::Audio)) },
        { AutoscanColumn::CtImage, quote(adir->getContainerTypes().at(AutoscanMediaMode::Image)) },
        { AutoscanColumn::CtVideo, quote(adir->getContainerTypes().at(AutoscanMediaMode::Video)) },
        { AutoscanColumn::Interval, quote(adir->getInterval().count()) },
        { AutoscanColumn::LastModified, quote(adir->getPreviousLMT().count()) },
        { AutoscanColumn::Persistent, quote(adir->persistent()) },
        { AutoscanColumn::Location, objectID >= 0 ? SQL_NULL : quote(adir->getLocation()) },
        { AutoscanColumn::PathIds, pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ","))) },
        { AutoscanColumn::RetryCount, quote(adir->getRetryCount()) },
        { AutoscanColumn::DirTypes, quote(adir->hasDirTypes()) },
        { AutoscanColumn::ForceRescan, quote(adir->getForceRescan()) },
    };
    if (adir->getPreviousLMT() > std::chrono::seconds::zero()) {
        fields[AutoscanColumn::LastModified] = quote(adir->getPreviousLMT().count());
    }
    Autoscan2Table at(fields, Operation::Insert, autoscanColumnMapper);
    adir->setDatabaseID(exec(at.sqlForInsert(adir), true));
}

void SQLDatabase::updateAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw DatabaseException("updateAutoscanDirectory called with adir==nullptr", LINE_MESSAGE);

    log_debug("id: {}, obj_id: {}", adir->getDatabaseID(), adir->getObjectID());

    auto pathIds = _checkOverlappingAutoscans(adir);

    int objectID = adir->getObjectID();
    int objectIDold = _getAutoscanObjectID(adir->getDatabaseID());
    if (objectIDold != objectID) {
        if (objectIDold != INVALID_OBJECT_ID)
            _autoscanChangePersistentFlag(objectIDold, false);
        _autoscanChangePersistentFlag(objectID, true);
    }
    auto fields = std::map<AutoscanColumn, std::string> {
        { AutoscanColumn::ObjId, objectID >= 0 ? quote(objectID) : SQL_NULL },
        { AutoscanColumn::ScanMode, quote(AutoscanDirectory::mapScanmode(adir->getScanMode())) },
        { AutoscanColumn::Recursive, quote(adir->getRecursive()) },
        { AutoscanColumn::MediaType, quote(adir->getMediaType()) },
        { AutoscanColumn::Hidden, quote(adir->getHidden()) },
        { AutoscanColumn::FollowSymlinks, quote(adir->getFollowSymlinks()) },
        { AutoscanColumn::CtAudio, quote(adir->getContainerTypes().at(AutoscanMediaMode::Audio)) },
        { AutoscanColumn::CtImage, quote(adir->getContainerTypes().at(AutoscanMediaMode::Image)) },
        { AutoscanColumn::CtVideo, quote(adir->getContainerTypes().at(AutoscanMediaMode::Video)) },
        { AutoscanColumn::Interval, quote(adir->getInterval().count()) },
        { AutoscanColumn::Persistent, quote(adir->persistent()) },
        { AutoscanColumn::Location, objectID >= 0 ? SQL_NULL : quote(adir->getLocation()) },
        { AutoscanColumn::PathIds, pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ","))) },
        { AutoscanColumn::RetryCount, quote(adir->getRetryCount()) },
        { AutoscanColumn::DirTypes, quote(adir->hasDirTypes()) },
        { AutoscanColumn::ForceRescan, quote(adir->getForceRescan()) },
        { AutoscanColumn::Touched, quote(true) },
    };
    if (adir->getPreviousLMT() > std::chrono::seconds::zero()) {
        fields[AutoscanColumn::LastModified] = quote(adir->getPreviousLMT().count());
    }
    Autoscan2Table at(fields, Operation::Update, autoscanColumnMapper);
    exec(AUTOSCAN_TABLE, at.sqlForUpdate(adir), adir->getDatabaseID());
}

void SQLDatabase::removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    _removeAutoscanDirectory(adir->getDatabaseID());
}

void SQLDatabase::_removeAutoscanDirectory(int autoscanID)
{
    if (autoscanID == INVALID_OBJECT_ID) {
        log_warning("cannot delete autoscan with illegal ID");
        return;
    }
    int objectID = _getAutoscanObjectID(autoscanID);
    deleteRow(AUTOSCAN_TABLE, "id", autoscanID);
    if (objectID != INVALID_OBJECT_ID)
        _autoscanChangePersistentFlag(objectID, false);
}

int SQLDatabase::_getAutoscanObjectID(int autoscanID)
{
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
        autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId, true),
        autoscanColumnMapper->getTableName(),
        autoscanColumnMapper->getClause(AutoscanColumn::Id, autoscanID, true)));
    if (!res)
        throw DatabaseException("error while doing select on autoscan", LINE_MESSAGE);
    auto row = res->nextRow();
    if (row && !row->isNullOrEmpty(0))
        return row->col_int(0, INVALID_OBJECT_ID);

    return INVALID_OBJECT_ID;
}

void SQLDatabase::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID) {
        log_warning("cannot change autoscan to persistent {} with illegal ID", persistent);
        return;
    }
    auto dict = std::map<BrowseColumn, std::string> {
        { BrowseColumn::Id, quote(objectID) },
        { BrowseColumn::Flags, fmt::format("({} {}{})", browseColumnMapper->mapQuoted(BrowseColumn::Flags, true), (persistent ? " | " : " & ~"), OBJECT_FLAG_PERSISTENT_CONTAINER) }
    };
    auto ot = Object2Table(std::move(dict), Operation::Update, browseColumnMapper);
    exec(ot.sqlForUpdate(nullptr));
}

void SQLDatabase::checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    (void)_checkOverlappingAutoscans(adir);
}

std::vector<int> SQLDatabase::_checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw DatabaseException("_checkOverlappingAutoscans called with adir==nullptr", LINE_MESSAGE);
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID) {
        log_warning("cannot check autoscan with illegal ID");
        return {};
    }
    int databaseID = adir->getDatabaseID();

    std::unique_ptr<SQLRow> row;
    {
        auto where = std::vector {
            autoscanColumnMapper->getClause(AutoscanColumn::ObjId, checkObjectID, true),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", autoscanColumnMapper->mapQuoted(AutoscanColumn::Id, true), databaseID));

        auto res = select(fmt::format("SELECT {} FROM {} WHERE {}",
            autoscanColumnMapper->mapQuoted(AutoscanColumn::Id, true),
            autoscanColumnMapper->getTableName(),
            fmt::join(where, " AND ")));
        if (!res)
            throw DatabaseException(fmt::format("error selecting form {}", AUTOSCAN_TABLE), LINE_MESSAGE);

        row = res->nextRow();
        if (row) {
            auto obj = loadObject(checkObjectID);
            if (!obj)
                throw DatabaseException("Referenced object (by Autoscan) not found.", LINE_MESSAGE);
            log_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw DatabaseException(fmt::format("There is already an Autoscan set on {}", obj->getLocation().c_str()), LINE_MESSAGE);
        }
    }

    if (adir->getRecursive()) {
        auto where = std::vector {
            fmt::format("{} LIKE {}", autoscanColumnMapper->mapQuoted(AutoscanColumn::PathIds, true), quote(fmt::format("%,{},%", checkObjectID))),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", autoscanColumnMapper->mapQuoted(AutoscanColumn::Id, true), databaseID));
        auto qRec = fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
            autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId, true),
            autoscanColumnMapper->getTableName(),
            fmt::join(where, " AND "));
        log_debug("------------ {}", qRec);
        auto res = select(qRec);
        if (!res)
            throw DatabaseException(fmt::format("error selecting form {}", AUTOSCAN_TABLE), LINE_MESSAGE);
        row = res->nextRow();
        if (row) {
            const int objectID = row->col_int(0, INVALID_OBJECT_ID);
            log_debug("-------------- {}", objectID);
            auto obj = loadObject(objectID);
            if (!obj)
                throw DatabaseException("Referenced object (by Autoscan) not found.", LINE_MESSAGE);
            log_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw DatabaseException(fmt::format("Overlapping Autoscans are not allowed. There is already an Autoscan set on {}", obj->getLocation().c_str()), LINE_MESSAGE);
        }
    }

    {
        auto pathIDs = getPathIDs(checkObjectID);
        if (pathIDs.empty())
            throw DatabaseException("getPathIDs returned empty", LINE_MESSAGE);

        auto where = std::vector {
            fmt::format("{} IN ({})", autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId, true), fmt::join(pathIDs, ",")),
            autoscanColumnMapper->getClause(AutoscanColumn::Recursive, quote(true), true),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", autoscanColumnMapper->mapQuoted(AutoscanColumn::Id, true), databaseID));
        auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
            autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId, true),
            autoscanColumnMapper->getTableName(),
            fmt::join(where, " AND ")));
        if (!res)
            throw DatabaseException(fmt::format("error selecting form {}", AUTOSCAN_TABLE), LINE_MESSAGE);
        if (!(row = res->nextRow()))
            return pathIDs;
    }

    const int objectID = row->col_int(0, INVALID_OBJECT_ID);
    auto obj = loadObject(objectID);
    if (!obj) {
        throw DatabaseException("Referenced object (by Autoscan) not found.", LINE_MESSAGE);
    }
    log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
    throw DatabaseException(fmt::format("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str()), LINE_MESSAGE);
}

std::vector<int> SQLDatabase::getPathIDs(int objectID)
{
    std::vector<int> pathIDs;
    if (objectID == INVALID_OBJECT_ID)
        return pathIDs;

    auto sel = fmt::format("SELECT {} FROM {} ",
        browseColumnMapper->mapQuoted(BrowseColumn::ParentId, true),
        browseColumnMapper->getTableName());

    while (objectID != CDS_ID_ROOT) {
        pathIDs.push_back(objectID);
        auto res = select(fmt::format("{} WHERE {} LIMIT 1", sel, browseColumnMapper->getClause(BrowseColumn::Id, objectID, true)));
        if (!res)
            break;
        auto row = res->nextRow();
        if (!row)
            break;
        objectID = row->col_int(0, INVALID_OBJECT_ID);
    }
    return pathIDs;
}

void SQLDatabase::generateMetaDataDBOperations(
    const std::shared_ptr<CdsObject>& obj,
    Operation op,
    std::vector<std::shared_ptr<AddUpdateTable<CdsObject>>>& operations) const
{
    const auto& dict = obj->getMetaData();
    operations.reserve(operations.size() + dict.size());
    if (op == Operation::Insert) {
        for (auto&& [key, val] : dict) {
            auto mt = std::make_shared<Metadata2Table>(
                std::map<MetadataColumn, std::string> {
                    { MetadataColumn::PropertyName, quote(key) },
                    { MetadataColumn::PropertyValue, quote(val, metaColMap.at(MetadataColumn::PropertyValue).length) } },
                Operation::Insert, metaColumnMapper);
            operations.push_back(std::move(mt));
        }
    } else {
        // delete current metadata from DB
        auto mt = std::make_shared<Metadata2Table>(std::map<MetadataColumn, std::string>(), Operation::Delete, metaColumnMapper);
        operations.push_back(std::move(mt));
        if (op != Operation::Delete) {
            for (auto&& [key, val] : dict) {
                auto mtk = std::make_shared<Metadata2Table>(
                    std::map<MetadataColumn, std::string> {
                        { MetadataColumn::PropertyName, quote(key) },
                        { MetadataColumn::PropertyValue, quote(val, metaColMap.at(MetadataColumn::PropertyValue).length) } },
                    Operation::Insert, metaColumnMapper);
                operations.push_back(std::move(mtk));
            }
        }
    }
}

std::vector<std::shared_ptr<CdsResource>> SQLDatabase::retrieveResourcesForObject(int objectId)
{
    auto rsql = fmt::format("{} FROM {} WHERE {} ORDER BY {}",
        sql_resource_query,
        resColumnMapper->getTableName(),
        resColumnMapper->getClause(ResourceColumn::ItemId, objectId, true),
        resColumnMapper->mapQuoted(ResourceColumn::ResId, true));
    log_debug("SQLDatabase::retrieveResourcesForObject {}", rsql);
    auto&& res = select(rsql);

    if (!res)
        return {};

    std::vector<std::shared_ptr<CdsResource>> resources;
    resources.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto resource = std::make_shared<CdsResource>(
            EnumMapper::remapContentHandler(std::stoi(getCol(row, ResourceColumn::HandlerType))),
            EnumMapper::remapPurpose(std::stoi(getCol(row, ResourceColumn::Purpose))),
            getCol(row, ResourceColumn::Options),
            getCol(row, ResourceColumn::Parameters));
        resource->setResId(resources.size());
        for (auto&& resAttrId : ResourceAttributeIterator()) {
            auto index = to_underlying(ResourceColumn::Attributes) + to_underlying(resAttrId);
            auto value = row->col_c_str(index);
            if (value) {
                resource->addAttribute(resAttrId, value);
            }
        }
        resources.push_back(std::move(resource));
    }

    return resources;
}

void SQLDatabase::generateResourceDBOperations(
    const std::shared_ptr<CdsObject>& obj,
    Operation op,
    std::vector<std::shared_ptr<AddUpdateTable<CdsObject>>>& operations)
{
    const auto& resources = obj->getResources();
    operations.reserve(operations.size() + resources.size());
    if (op == Operation::Insert) {
        std::size_t resId = 0;
        for (auto&& resource : resources) {
            std::map<ResourceColumn, std::string> resourceSql;
            resourceSql[ResourceColumn::ResId] = quote(resId);
            resourceSql[ResourceColumn::HandlerType] = quote(to_underlying(resource->getHandlerType()));
            resourceSql[ResourceColumn::Purpose] = quote(to_underlying(resource->getPurpose()));
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                resourceSql[ResourceColumn::Options] = quote(URLUtils::dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql[ResourceColumn::Parameters] = quote(URLUtils::dictEncode(parameters));
            }
            std::map<ResourceAttribute, std::string> resourceAttrs;
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceAttrs[key] = quote(val);
            }
            auto rt = std::make_shared<Resource2Table>(std::move(resourceSql), std::move(resourceAttrs), Operation::Insert, resColumnMapper);
            operations.push_back(std::move(rt));
            resId++;
        }
    } else {
        // get current resoures from DB
        auto dbResources = retrieveResourcesForObject(obj->getID());
        std::size_t resId = 0;
        for (auto&& resource : resources) {
            Operation operation = resId < dbResources.size() ? Operation::Update : Operation::Insert;
            std::map<ResourceColumn, std::string> resourceSql;
            resourceSql[ResourceColumn::ResId] = quote(resId);
            resourceSql[ResourceColumn::HandlerType] = quote(to_underlying(resource->getHandlerType()));
            resourceSql[ResourceColumn::Purpose] = quote(to_underlying(resource->getPurpose()));
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                resourceSql[ResourceColumn::Options] = quote(URLUtils::dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql[ResourceColumn::Parameters] = quote(URLUtils::dictEncode(parameters));
            }
            std::map<ResourceAttribute, std::string> resourceAttrs;
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceAttrs[key] = quote(val);
            }
            auto rt = std::make_shared<Resource2Table>(std::move(resourceSql), std::move(resourceAttrs), operation, resColumnMapper);
            operations.push_back(std::move(rt));
            resId++;
        }
        // res_id in db resources but not obj resources, so needs a delete
        while (resId < dbResources.size()) {
            if (dbResources.at(resId)) {
                std::map<ResourceColumn, std::string> resourceSql;
                std::map<ResourceAttribute, std::string> resourceAttrs;
                resourceSql[ResourceColumn::ResId] = quote(resId);
                auto rt = std::make_shared<Resource2Table>(std::move(resourceSql), std::move(resourceAttrs), Operation::Delete, resColumnMapper);
                operations.push_back(std::move(rt));
            }
            ++resId;
        }
    }
}

// column metadata is dropped in DBVERSION 12
bool SQLDatabase::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required");
    auto res = select(fmt::format("SELECT COUNT(*) FROM {} WHERE {} IS NOT NULL",
        browseColumnMapper->getTableName(),
        identifier("metadata"))); // metadata column is removed!
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    res = select(fmt::format("SELECT COUNT(*) FROM {}", metaColumnMapper->getTableName()));
    int metadataRowCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_metadata rows having metadata: {}", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required");
        return true;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata");

    auto resIds = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        identifier("metadata"),
        browseColumnMapper->getTableName()));
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow())) {
        migrateMetadata(row->col_int(0, INVALID_OBJECT_ID), row->col(1));
        ++objectsUpdated;
    }
    log_info("Migrated metadata - object count: {}", objectsUpdated);
    return true;
}

void SQLDatabase::migrateMetadata(int objectId, const std::string& metadataStr)
{
    std::map<std::string, std::string> itemMetadata = URLUtils::dictDecode(metadataStr);

    if (!itemMetadata.empty()) {
        log_debug("Migrating metadata for cds object {}", objectId);
        std::vector<std::map<MetadataColumn, std::string>> multiDict;
        multiDict.reserve(itemMetadata.size());
        for (auto&& [key, val] : itemMetadata) {
            auto mDict = std::map<MetadataColumn, std::string> {
                { MetadataColumn::ItemId, quote(objectId) },
                { MetadataColumn::PropertyName, quote(key) },
                { MetadataColumn::PropertyValue, quote(val) },
            };
            multiDict.push_back(std::move(mDict));
        }
        Metadata2Table mt(std::move(multiDict), metaColumnMapper);
        exec(METADATA_TABLE, mt.sqlForMultiInsert(nullptr), objectId);
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", objectId);
    }
}

void SQLDatabase::prepareResourceTable(std::string_view addColumnCmd)
{
    auto resourceAttributes = splitString(getInternalSetting("resource_attribute"), ',');
    bool addedAttribute = false;
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto&& resAttrib = EnumMapper::getAttributeName(resAttrId);
        if (std::find(resourceAttributes.begin(), resourceAttributes.end(), resAttrib) == resourceAttributes.end()) {
            _exec(fmt::format(addColumnCmd, resAttrib));
            log_info("'{}': Adding column '{}'", RESOURCE_TABLE, resAttrib);
            resourceAttributes.push_back(resAttrib);
            addedAttribute = true;
        }
    }
    if (addedAttribute)
        storeInternalSetting("resource_attribute", fmt::format("{}", fmt::join(resourceAttributes, ",")));
}

// column resources is dropped in DBVERSION 13
bool SQLDatabase::doResourceMigration()
{
    if (!addResourceColumnCmd.empty())
        prepareResourceTable(addResourceColumnCmd);

    log_debug("Checking if resources migration is required");
    auto res = select(
        fmt::format("SELECT COUNT(*) FROM {} WHERE {} IS NOT NULL",
            browseColumnMapper->getTableName(),
            identifier("resources"))); // resources column is removed!
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having resources: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    res = select(
        fmt::format("SELECT COUNT(*) FROM {}", resColumnMapper->getTableName()));
    int resourceRowCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having entries: {}", RESOURCE_TABLE, resourceRowCount);

    if (expectedConversionCount > 0 && resourceRowCount > 0) {
        log_info("No resources migration required");
        return true;
    }

    log_info("About to migrate resources from {} to {}", CDS_OBJECT_TABLE, RESOURCE_TABLE);

    auto resIds = select(
        fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
            browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
            identifier("resources"),
            browseColumnMapper->getTableName()));
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow())) {
        migrateResources(row->col_int(0, INVALID_OBJECT_ID), row->col(1));
        ++objectsUpdated;
    }
    log_info("Migrated resources - object count: {}", objectsUpdated);
    return true;
}

void SQLDatabase::migrateResources(int objectId, const std::string& resourcesStr)
{
    if (!resourcesStr.empty()) {
        log_debug("Migrating resources for cds object {}", objectId);
        auto resources = splitString(resourcesStr, RESOURCE_SEP);
        std::size_t resId = 0;
        for (auto&& resString : resources) {
            std::map<ResourceAttribute, std::string> resourceVals;
            auto&& resource = CdsResource::decode(resString);
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceVals[key] = quote(val);
            }
            auto dict = std::map<ResourceColumn, std::string> {
                { ResourceColumn::ItemId, quote(objectId) },
                { ResourceColumn::ResId, quote(resId) },
                { ResourceColumn::HandlerType, quote(to_underlying(resource->getHandlerType())) },
                { ResourceColumn::Purpose, quote(to_underlying(resource->getPurpose())) },
            };
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                dict[ResourceColumn::Options] = quote(URLUtils::dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                dict[ResourceColumn::Parameters] = quote(URLUtils::dictEncode(parameters));
            }
            Resource2Table rt(std::move(dict), std::move(resourceVals), Operation::Insert, resColumnMapper);
            exec(RESOURCE_TABLE, rt.sqlForInsert(nullptr), objectId);
            resId++;
        }
    } else {
        log_debug("Skipping migration - no resources for cds object {}", objectId);
    }
}

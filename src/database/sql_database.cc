/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sql_database.cc - this file is part of MediaTomb.

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

/// \file sql_database.cc

#include "sql_database.h" // API

#include <algorithm>
#include <fmt/chrono.h>
#include <vector>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "config/config_setup.h"
#include "config/dynamic_content.h"
#include "content/autoscan.h"
#include "metadata/metadata_handler.h"
#include "search_handler.h"
#include "upnp_xml.h"
#include "util/grb_net.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "util/upnp_clients.h"

#define MAX_REMOVE_SIZE 1000
#define MAX_REMOVE_RECURSION 500

#define SQL_NULL "NULL"

#define RESOURCE_SEP '|'

#define ITM_ALIAS "f"
#define REF_ALIAS "rf"
#define AUS_ALIAS "as"
#define SRC_ALIAS "c"
#define MTA_ALIAS "m"
#define RES_ALIAS "re"

// database
#define LOC_DIR_PREFIX 'D'
#define LOC_FILE_PREFIX 'F'
#define LOC_VIRT_PREFIX 'V'
#define LOC_ILLEGAL_PREFIX 'X'

/// \brief browse column ids
/// enum for createObjectFromRow's mode parameter
enum class BrowseCol {
    Id = 0,
    RefId,
    ParentId,
    ObjectType,
    UpnpClass,
    DcTitle,
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

/// \brief search column ids
enum class SearchCol {
    Id = 0,
    RefId,
    ParentId,
    ObjectType,
    UpnpClass,
    DcTitle,
    MimeType,
    Flags,
    PartNumber,
    TrackNumber,
    Location,
    LastModified,
    LastUpdated,
};

/// \brief meta column ids
enum class MetadataCol {
    ItemId = 0,
    PropertyName,
    PropertyValue
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

/// \brief autoscan column ids
enum class AutoscanCol {
    Id = 0,
    ObjId,
    Persistent,
};

/// \brief autoscan column ids
enum class AutoscanColumn {
    Id = 0,
    ObjId,
    ScanMode,
    Recursive,
    MedaType,
    Hidden,
    Interval,
    LastModified,
    Persistent,
    Location,
    ObjLocation,
};

/// \brief Map browse column ids to column names
// map ensures entries are in correct order, each value of BrowseCol must be present
static const std::map<BrowseCol, std::pair<std::string, std::string>> browseColMap {
    { BrowseCol::Id, { ITM_ALIAS, "id" } },
    { BrowseCol::RefId, { ITM_ALIAS, "ref_id" } },
    { BrowseCol::ParentId, { ITM_ALIAS, "parent_id" } },
    { BrowseCol::ObjectType, { ITM_ALIAS, "object_type" } },
    { BrowseCol::UpnpClass, { ITM_ALIAS, "upnp_class" } },
    { BrowseCol::DcTitle, { ITM_ALIAS, "dc_title" } },
    { BrowseCol::Location, { ITM_ALIAS, "location" } },
    { BrowseCol::LocationHash, { ITM_ALIAS, "location_hash" } },
    { BrowseCol::Auxdata, { ITM_ALIAS, "auxdata" } },
    { BrowseCol::UpdateId, { ITM_ALIAS, "update_id" } },
    { BrowseCol::MimeType, { ITM_ALIAS, "mime_type" } },
    { BrowseCol::Flags, { ITM_ALIAS, "flags" } },
    { BrowseCol::PartNumber, { ITM_ALIAS, "part_number" } },
    { BrowseCol::TrackNumber, { ITM_ALIAS, "track_number" } },
    { BrowseCol::ServiceId, { ITM_ALIAS, "service_id" } },
    { BrowseCol::LastModified, { ITM_ALIAS, "last_modified" } },
    { BrowseCol::LastUpdated, { ITM_ALIAS, "last_updated" } },
    { BrowseCol::RefUpnpClass, { REF_ALIAS, "upnp_class" } },
    { BrowseCol::RefLocation, { REF_ALIAS, "location" } },
    { BrowseCol::RefAuxdata, { REF_ALIAS, "auxdata" } },
    { BrowseCol::RefMimeType, { REF_ALIAS, "mime_type" } },
    { BrowseCol::RefServiceId, { REF_ALIAS, "service_id" } },
    { BrowseCol::AsPersistent, { AUS_ALIAS, "persistent" } },
};

/// \brief Map search oolumn ids to column names
// map ensures entries are in correct order, each value of SearchCol must be present
static const std::map<SearchCol, std::pair<std::string, std::string>> searchColMap {
    { SearchCol::Id, { SRC_ALIAS, "id" } },
    { SearchCol::RefId, { SRC_ALIAS, "ref_id" } },
    { SearchCol::ParentId, { SRC_ALIAS, "parent_id" } },
    { SearchCol::ObjectType, { SRC_ALIAS, "object_type" } },
    { SearchCol::UpnpClass, { SRC_ALIAS, "upnp_class" } },
    { SearchCol::DcTitle, { SRC_ALIAS, "dc_title" } },
    { SearchCol::MimeType, { SRC_ALIAS, "mime_type" } },
    { SearchCol::Flags, { SRC_ALIAS, "flags" } },
    { SearchCol::PartNumber, { SRC_ALIAS, "part_number" } },
    { SearchCol::TrackNumber, { SRC_ALIAS, "track_number" } },
    { SearchCol::Location, { SRC_ALIAS, "location" } },
    { SearchCol::LastModified, { SRC_ALIAS, "last_modified" } },
    { SearchCol::LastUpdated, { SRC_ALIAS, "last_updated" } },
};

/// \brief Map meta column ids to column names
// map ensures entries are in correct order, each value of MetadataCol must be present
static const std::map<MetadataCol, std::pair<std::string, std::string>> metaColMap {
    { MetadataCol::ItemId, { MTA_ALIAS, "item_id" } },
    { MetadataCol::PropertyName, { MTA_ALIAS, "property_name" } },
    { MetadataCol::PropertyValue, { MTA_ALIAS, "property_value" } },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanCol must be present
static const std::map<AutoscanCol, std::pair<std::string, std::string>> asColMap {
    { AutoscanCol::Id, { AUS_ALIAS, "id" } },
    { AutoscanCol::ObjId, { AUS_ALIAS, "obj_id" } },
    { AutoscanCol::Persistent, { AUS_ALIAS, "persistent" } },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanColumn must be present
static const std::map<AutoscanColumn, std::pair<std::string, std::string>> autoscanColMap {
    { AutoscanColumn::Id, { AUS_ALIAS, "id" } },
    { AutoscanColumn::ObjId, { AUS_ALIAS, "obj_id" } },
    { AutoscanColumn::ScanMode, { AUS_ALIAS, "scan_mode" } },
    { AutoscanColumn::Recursive, { AUS_ALIAS, "recursive" } },
    { AutoscanColumn::MedaType, { AUS_ALIAS, "media_type" } },
    { AutoscanColumn::Hidden, { AUS_ALIAS, "hidden" } },
    { AutoscanColumn::Interval, { AUS_ALIAS, "interval" } },
    { AutoscanColumn::LastModified, { AUS_ALIAS, "last_modified" } },
    { AutoscanColumn::Persistent, { AUS_ALIAS, "persistent" } },
    { AutoscanColumn::Location, { AUS_ALIAS, "location" } },
    { AutoscanColumn::ObjLocation, { ITM_ALIAS, "location" } },
};

/// \brief Map browse sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, BrowseCol>> browseSortMap {
    { MetadataHandler::getMetaFieldName(M_PARTNUMBER), BrowseCol::PartNumber },
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), BrowseCol::PartNumber },
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), BrowseCol::TrackNumber },
    { MetadataHandler::getMetaFieldName(M_TITLE), BrowseCol::DcTitle },
    { UPNP_SEARCH_CLASS, BrowseCol::UpnpClass },
    { UPNP_SEARCH_PATH, BrowseCol::Location },
    { UPNP_SEARCH_REFID, BrowseCol::RefId },
    { UPNP_SEARCH_PARENTID, BrowseCol::ParentId },
    { UPNP_SEARCH_ID, BrowseCol::Id },
    { UPNP_SEARCH_LAST_UPDATED, BrowseCol::LastUpdated },
    { UPNP_SEARCH_LAST_MODIFIED, BrowseCol::LastModified },
};

/// \brief Map search sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, SearchCol>> searchSortMap {
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), SearchCol::PartNumber },
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), SearchCol::TrackNumber },
    { UPNP_SEARCH_CLASS, SearchCol::UpnpClass },
    { UPNP_SEARCH_PATH, SearchCol::Location },
    { UPNP_SEARCH_REFID, SearchCol::RefId },
    { UPNP_SEARCH_PARENTID, SearchCol::ParentId },
    { UPNP_SEARCH_ID, SearchCol::Id },
    { UPNP_SEARCH_LAST_UPDATED, SearchCol::LastUpdated },
    { UPNP_SEARCH_LAST_MODIFIED, SearchCol::LastModified },
};

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, MetadataCol>> metaTagMap {
    { UPNP_SEARCH_ID, MetadataCol::ItemId },
    { META_NAME, MetadataCol::PropertyName },
    { META_VALUE, MetadataCol::PropertyValue },
};

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, AutoscanCol>> asTagMap {
    { "id", AutoscanCol::Id },
    { "obj_id", AutoscanCol::ObjId },
    { "persistent", AutoscanCol::Persistent },
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
    { "interval", AutoscanColumn::Interval },
    { "last_modified", AutoscanColumn::LastModified },
    { "persistent", AutoscanColumn::Persistent },
    { "location", AutoscanColumn::Location },
    { "obj_location", AutoscanColumn::ObjLocation },
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

static std::shared_ptr<EnumColumnMapper<BrowseCol>> browseColumnMapper;
static std::shared_ptr<EnumColumnMapper<SearchCol>> searchColumnMapper;
static std::shared_ptr<EnumColumnMapper<MetadataCol>> metaColumnMapper;
static std::shared_ptr<EnumColumnMapper<AutoscanCol>> asColumnMapper;
static std::shared_ptr<EnumColumnMapper<AutoscanColumn>> autoscanColumnMapper;
static std::shared_ptr<EnumColumnMapper<int>> resourceColumnMapper;

SQLDatabase::SQLDatabase(const std::shared_ptr<Config>& config, std::shared_ptr<Mime> mime)
    : Database(config)
    , mime(std::move(mime))
{
}

void SQLDatabase::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw_std_runtime_error("quote vars need to be overridden");

    /// \brief Map resource search keys to column ids
    // entries are handled sequentially,
    // duplicate entries are added to statement in same order if key is present in SortCriteria
    std::vector<std::pair<std::string, int>> resourceTagMap {
        { UPNP_SEARCH_ID, to_underlying(ResourceCol::ItemId) },
        { "res@id", to_underlying(ResourceCol::ResId) },
    };
    /// \brief Map resource column ids to column names
    // map ensures entries are in correct order, each value of ResourceCol must be present
    std::map<int, std::pair<std::string, std::string>> resourceColMap {
        { to_underlying(ResourceCol::ItemId), { RES_ALIAS, "item_id" } },
        { to_underlying(ResourceCol::ResId), { RES_ALIAS, "res_id" } },
        { to_underlying(ResourceCol::HandlerType), { RES_ALIAS, "handlerType" } },
        { to_underlying(ResourceCol::Purpose), { RES_ALIAS, "purpose" } },
        { to_underlying(ResourceCol::Options), { RES_ALIAS, "options" } },
        { to_underlying(ResourceCol::Parameters), { RES_ALIAS, "parameters" } },
    };

    /// \brief List of column names to be used in insert and update to ensure correct order of columns
    // only columns listed here are added to the insert and update statements
    tableColumnOrder = {
        { CDS_OBJECT_TABLE, { "ref_id", "parent_id", "object_type", "upnp_class", "dc_title", "location", "location_hash", "auxdata", "update_id", "mime_type", "flags", "part_number", "track_number", "service_id", "last_modified", "last_updated" } },
        { METADATA_TABLE, { "item_id", "property_name", "property_value" } },
        { RESOURCE_TABLE, { "item_id", "res_id", "handlerType", "purpose", "options", "parameters" } },
    };
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = CdsResource::getAttributeName(resAttrId);
        resourceTagMap.emplace_back(fmt::format("res@{}", attrName), to_underlying(ResourceCol::Attributes) + to_underlying(resAttrId));
        resourceColMap.emplace(to_underlying(ResourceCol::Attributes) + to_underlying(resAttrId), std::pair(RES_ALIAS, attrName));
        tableColumnOrder[RESOURCE_TABLE].push_back(std::move(attrName));
    }

    browseColumnMapper = std::make_shared<EnumColumnMapper<BrowseCol>>(table_quote_begin, table_quote_end, ITM_ALIAS, CDS_OBJECT_TABLE, browseSortMap, browseColMap);
    auto searchTagMap = searchSortMap;
    if (config->getBoolOption(CFG_UPNP_SEARCH_FILENAME)) {
        searchTagMap.emplace_back(MetadataHandler::getMetaFieldName(M_TITLE), SearchCol::DcTitle);
    }
    searchColumnMapper = std::make_shared<EnumColumnMapper<SearchCol>>(table_quote_begin, table_quote_end, SRC_ALIAS, CDS_OBJECT_TABLE, searchTagMap, searchColMap);
    metaColumnMapper = std::make_shared<EnumColumnMapper<MetadataCol>>(table_quote_begin, table_quote_end, MTA_ALIAS, METADATA_TABLE, metaTagMap, metaColMap);
    resourceColumnMapper = std::make_shared<EnumColumnMapper<int>>(table_quote_begin, table_quote_end, RES_ALIAS, RESOURCE_TABLE, resourceTagMap, resourceColMap);
    asColumnMapper = std::make_shared<EnumColumnMapper<AutoscanCol>>(table_quote_begin, table_quote_end, AUS_ALIAS, AUTOSCAN_TABLE, asTagMap, asColMap);
    autoscanColumnMapper = std::make_shared<EnumColumnMapper<AutoscanColumn>>(table_quote_begin, table_quote_end, AUS_ALIAS, AUTOSCAN_TABLE, autoscanTagMap, autoscanColMap);

    // Statement for UPnP browse
    {
        std::vector<std::string> buf;
        buf.reserve(browseColMap.size());
        for (auto&& [key, col] : browseColMap) {
            buf.push_back(fmt::format("{}.{}", identifier(col.first), identifier(col.second)));
        }
        auto join1 = fmt::format("LEFT JOIN {0} {1} ON {2} = {1}.{3}",
            identifier(CDS_OBJECT_TABLE), identifier(REF_ALIAS), browseColumnMapper->mapQuoted(BrowseCol::RefId), identifier(browseColMap.at(BrowseCol::Id).second));
        auto join2 = fmt::format("LEFT JOIN {} ON {} = {}", asColumnMapper->tableQuoted(), asColumnMapper->mapQuoted(AutoscanCol::ObjId), browseColumnMapper->mapQuoted(BrowseCol::Id));
        this->sql_browse_columns = fmt::format("{}", fmt::join(buf, ", "));
        this->sql_browse_query = fmt::format("{} {} {} ", browseColumnMapper->tableQuoted(), join1, join2);
    }
    // Statement for UPnP search
    {
        std::vector<std::string> colBuf;
        colBuf.reserve(searchColMap.size());
        for (auto&& [key, col] : searchColMap) {
            colBuf.push_back(fmt::format("{}.{}", identifier(col.first), identifier(col.second)));
        }
        this->sql_search_columns = fmt::format("{}", fmt::join(colBuf, ", "));

        auto join1 = fmt::format("LEFT JOIN {} ON {} = {}", metaColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), metaColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        auto join2 = fmt::format("LEFT JOIN {} ON {} = {}", resourceColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        this->sql_search_query = fmt::format("{} {} {}", searchColumnMapper->tableQuoted(), join1, join2);

        // Build container query format string
        auto sql_container_query = fmt::format(sql_search_container_query_raw, identifier("containers"), identifier("cont"), searchColumnMapper->tableQuoted(), searchColumnMapper->getAlias(), searchColumnMapper->mapQuoted(UPNP_SEARCH_PARENTID, true), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID, true), searchColumnMapper->mapQuoted(UPNP_SEARCH_REFID, true));
        this->sql_search_container_query_format = fmt::format("{} {} {}", sql_container_query, join1, join2);
    }
    // Statement for metadata
    {
        std::vector<SQLIdentifier> buf;
        buf.reserve(metaColMap.size());
        for (auto&& [key, col] : metaColMap) {
            buf.push_back(identifier(col.second)); // currently no alias
        }
        this->sql_meta_query = fmt::format("SELECT {} ", fmt::join(buf, ", "));
    }
    // Statement for autoscan
    {
        std::vector<std::string> buf;
        buf.reserve(autoscanColMap.size());
        for (auto&& [key, col] : autoscanColMap) {
            buf.push_back(fmt::format("{}.{}", identifier(col.first), identifier(col.second)));
        }
        auto join = fmt::format("LEFT JOIN {} ON {} = {}", browseColumnMapper->tableQuoted(), autoscanColumnMapper->mapQuoted(AutoscanColumn::ObjId), browseColumnMapper->mapQuoted(BrowseCol::Id));
        this->sql_autoscan_query = fmt::format("SELECT {} FROM {} {}", fmt::join(buf, ", "), autoscanColumnMapper->tableQuoted(), join);
    }
    // Statement for resource
    {
        std::vector<SQLIdentifier> buf;
        buf.reserve(resourceColMap.size());
        for (auto&& [key, col] : resourceColMap) {
            buf.push_back(identifier(col.second)); // currently no alias
        }
        this->sql_resource_query = fmt::format("SELECT {} ", fmt::join(buf, ", "));
    }

    sqlEmitter = std::make_shared<DefaultSQLEmitter>(searchColumnMapper, metaColumnMapper, resourceColumnMapper);
}

void SQLDatabase::upgradeDatabase(unsigned int dbVersion, const std::array<unsigned int, DBVERSION>& hashies, config_option_t upgradeOption, std::string_view updateVersionCommand, std::string_view addResourceColumnCmd)
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
            throw_std_runtime_error("Wrong hash for version {}", version + 1);
        }
        dbUpdates.push_back(std::move(versionCmds));
        version++;
    }

    if (version != DBVERSION)
        throw_std_runtime_error("The database upgrade file {} seems to be from another Gerbera version. Expected {}, actual {}", upgradeFile.c_str(), DBVERSION, dbVersion);

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
        throw_std_runtime_error("The database seems to be from another Gerbera version. Expected {}, actual {}", DBVERSION, dbVersion);

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
    sortKeys.reserve(to_underlying(M_MAX) + to_underlying(CdsResource::Attribute::MAX));
    for (auto&& [field, meta] : mt_keys) {
        if (std::find(sortKeys.begin(), sortKeys.end(), meta) == sortKeys.end()) {
            sortKeys.emplace_back(meta);
        }
    }
    return fmt::format("{}", fmt::join(sortKeys, ","));
}

std::string SQLDatabase::getSearchCapabilities()
{
    auto searchKeys = std::vector {
        std::string(UPNP_SEARCH_CLASS),
    };
    searchKeys.reserve(to_underlying(M_MAX) + to_underlying(CdsResource::Attribute::MAX));
    for (auto&& [field, meta] : mt_keys) {
        searchKeys.emplace_back(meta);
    }
    searchKeys.emplace_back("res");
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = CdsResource::getAttributeName(resAttrId);
        searchKeys.push_back(fmt::format("res@{}", attrName));
    }
    return fmt::format("{}", fmt::join(searchKeys, ","));
}

std::shared_ptr<CdsObject> SQLDatabase::checkRefID(const std::shared_ptr<CdsObject>& obj)
{
    if (!obj->isVirtual())
        throw_std_runtime_error("checkRefID called for a non-virtual object");

    int refID = obj->getRefID();
    fs::path location = obj->getLocation();

    if (location.empty())
        throw_std_runtime_error("tried to check refID without a location set");

    if (refID > 0) {
        try {
            auto refObj = loadObject(refID);
            if (refObj && refObj->getLocation() == location)
                return refObj;
        } catch (const std::runtime_error&) {
            throw_std_runtime_error("illegal refID was set");
        }
    }

    // This should never happen - but fail softly
    // It means that something doesn't set the refID correctly
    log_warning("Failed to loadObject with refid: {}", refID);

    return findObjectByPath(location);
}

std::vector<SQLDatabase::AddUpdateTable> SQLDatabase::_addUpdateObject(const std::shared_ptr<CdsObject>& obj, Operation op, int* changedContainer)
{
    std::shared_ptr<CdsObject> refObj;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef) {
        if (obj->isPureItem())
            throw_std_runtime_error("Tried to add pure item with PLAYLIST_REF flag set");
        if (obj->getRefID() <= 0)
            throw_std_runtime_error("PLAYLIST_REF flag set for '{}' but refId is <=0", obj->getLocation().c_str());
        refObj = loadObject(obj->getRefID());
        if (!refObj)
            throw_std_runtime_error("PLAYLIST_REF flag set but refId doesn't point to an existing object");
    } else if (obj->isVirtual() && obj->isPureItem()) {
        hasReference = true;
        refObj = checkRefID(obj);
        if (!refObj)
            throw_std_runtime_error("Tried to add or update a virtual object with illegal reference id and an illegal location");
    } else if (obj->getRefID() > 0) {
        if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            hasReference = true;
            refObj = loadObject(obj->getRefID());
            if (!refObj)
                throw_std_runtime_error("OBJECT_FLAG_ONLINE_SERVICE and refID set but refID doesn't point to an existing object");
        } else if (obj->isContainer()) {
            // in this case it's a playlist-container. that's ok
            // we don't need to do anything
        } else
            throw_std_runtime_error("refId set, but it makes no sense");
    }

    std::map<std::string, std::string> cdsObjectSql;
    cdsObjectSql.emplace("object_type", quote(obj->getObjectType()));

    if (hasReference || playlistRef)
        cdsObjectSql.emplace("ref_id", quote(refObj->getID()));
    else if (op == Operation::Update)
        cdsObjectSql.emplace("ref_id", SQL_NULL);

    if (!hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql.emplace("upnp_class", quote(obj->getClass()));
    else if (op == Operation::Update)
        cdsObjectSql.emplace("upnp_class", SQL_NULL);

    // if (!hasReference || refObj->getTitle() != obj->getTitle())
    cdsObjectSql.emplace("dc_title", quote(obj->getTitle()));
    // else if (isUpdate)
    //     cdsObjectSql.emplace("dc_title", SQL_NULL);

    if (op == Operation::Update)
        cdsObjectSql.emplace("auxdata", SQL_NULL);

    auto&& auxData = obj->getAuxData();
    if (!auxData.empty() && (!hasReference || auxData != refObj->getAuxData())) {
        cdsObjectSql.insert_or_assign("auxdata", quote(dictEncode(auxData)));
    }

    const bool useResourceRef = obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    cdsObjectSql.emplace("flags", quote(obj->getFlags()));

    if (obj->getMTime() > std::chrono::seconds::zero()) {
        cdsObjectSql.emplace("last_modified", quote(obj->getMTime().count()));
    } else {
        cdsObjectSql.emplace("last_modified", SQL_NULL);
    }
    cdsObjectSql.emplace("last_updated", quote(currentTime().count()));

    if (obj->isContainer() && op == Operation::Update && obj->isVirtual()) {
        fs::path dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, obj->getLocation());
        cdsObjectSql.emplace("location", quote(dbLocation));
        cdsObjectSql.emplace("location_hash", quote(stringHash(dbLocation.string())));
    } else if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (!hasReference) {
            fs::path loc = item->getLocation();
            if (loc.empty())
                throw_std_runtime_error("tried to create or update a non-referenced item without a location set");
            if (obj->isPureItem()) {
                int parentID = ensurePathExistence(loc.parent_path(), changedContainer);
                obj->setParentID(parentID);
                fs::path dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                cdsObjectSql.emplace("location", quote(dbLocation));
                cdsObjectSql.emplace("location_hash", quote(stringHash(dbLocation.string())));
            } else {
                // URLs
                cdsObjectSql.emplace("location", quote(loc));
                cdsObjectSql.emplace("location_hash", SQL_NULL);
            }
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace("location", SQL_NULL);
            cdsObjectSql.emplace("location_hash", SQL_NULL);
        }

        if (item->getTrackNumber() > 0) {
            cdsObjectSql.emplace("track_number", quote(item->getTrackNumber()));
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace("track_number", SQL_NULL);
        }

        if (item->getPartNumber() > 0) {
            cdsObjectSql.emplace("part_number", quote(item->getPartNumber()));
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace("part_number", SQL_NULL);
        }

        if (!item->getServiceID().empty()) {
            if (!hasReference || std::static_pointer_cast<CdsItem>(refObj)->getServiceID() != item->getServiceID())
                cdsObjectSql.emplace("service_id", quote(item->getServiceID()));
            else
                cdsObjectSql.emplace("service_id", SQL_NULL);
        } else if (op == Operation::Update) {
            cdsObjectSql.emplace("service_id", SQL_NULL);
        }

        cdsObjectSql.emplace("mime_type", quote(item->getMimeType().substr(0, 40)));
    }

    if (obj->getParentID() == INVALID_OBJECT_ID) {
        throw_std_runtime_error("Tried to create or update an object {} with an illegal parent id {}", obj->getLocation().c_str(), obj->getParentID());
    }

    cdsObjectSql.emplace("parent_id", quote(obj->getParentID()));

    std::vector<AddUpdateTable> returnVal;
    // check for a duplicate (virtual) object
    if (hasReference && op != Operation::Update) {
        auto where = std::vector {
            fmt::format("{}={:d}", identifier("parent_id"), obj->getParentID()),
            fmt::format("{}={:d}", identifier("ref_id"), refObj->getID()),
            fmt::format("{}={}", identifier("dc_title"), quote(obj->getTitle())),
        };
        auto res = select(fmt::format("SELECT 1 FROM {} WHERE {} LIMIT 1",
            identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND ")));
        // if duplicate items is found - ignore
        if (res && res->getNumRows() > 0)
            return returnVal;
    }

    returnVal.emplace_back(CDS_OBJECT_TABLE, std::move(cdsObjectSql), op);

    if (!hasReference || obj->getMetaData() != refObj->getMetaData()) {
        generateMetaDataDBOperations(obj, op, returnVal);
    }

    if (!hasReference || (!useResourceRef && !refObj->resourcesEqual(obj))) {
        generateResourceDBOperations(obj, op, returnVal);
    }

    return returnVal;
}

void SQLDatabase::addObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw_std_runtime_error("Tried to add an object with an object ID set");

    auto tables = _addUpdateObject(obj, Operation::Insert, changedContainer);

    beginTransaction("addObject");
    for (auto&& addUpdateTable : tables) {
        auto qb = sqlForInsert(obj, addUpdateTable);
        log_debug("Generated insert: {}", qb);

        if (addUpdateTable.getTableName() == CDS_OBJECT_TABLE) {
            int newId = exec(qb, true);
            obj->setID(newId);
        } else {
            exec(qb, false);
        }
    }
    commit("addObject");
}

void SQLDatabase::updateObject(const std::shared_ptr<CdsObject>& obj, int* changedContainer)
{
    std::vector<AddUpdateTable> data;
    if (obj->getID() == CDS_ID_FS_ROOT) {
        std::map<std::string, std::string> cdsObjectSql;

        cdsObjectSql["dc_title"] = quote(obj->getTitle());
        cdsObjectSql["upnp_class"] = quote(obj->getClass());

        data.emplace_back(CDS_OBJECT_TABLE, std::move(cdsObjectSql), Operation::Update);
    } else {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw_std_runtime_error("Tried to update an object with a forbidden ID ({})", obj->getID());
        data = _addUpdateObject(obj, Operation::Update, changedContainer);
    }

    beginTransaction("updateObject");
    for (auto&& addUpdateTable : data) {
        auto qb = [this, &obj, &addUpdateTable]() {
            Operation op = addUpdateTable.getOperation();
            switch (op) {
            case Operation::Insert:
                return sqlForInsert(obj, addUpdateTable);
            case Operation::Update:
                return sqlForUpdate(obj, addUpdateTable);
            case Operation::Delete:
                return sqlForDelete(obj, addUpdateTable);
            }
            return std::string();
        }();

        log_debug("upd_query: {}", qb);
        exec(qb);
    }
    commit("updateObject");
}

std::shared_ptr<CdsObject> SQLDatabase::loadObject(int objectID)
{
    if (dynamicContainers.find(objectID) != dynamicContainers.end()) {
        return dynamicContainers.at(objectID);
    }

    beginTransaction("loadObject");
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {} = {}", sql_browse_columns, sql_browse_query, browseColumnMapper->mapQuoted(BrowseCol::Id), objectID);
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow("", row);
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
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {} = {}", sql_browse_columns, sql_browse_query, browseColumnMapper->mapQuoted(BrowseCol::Id), objectID);
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

std::shared_ptr<CdsObject> SQLDatabase::loadObjectByServiceID(const std::string& serviceID)
{
    auto loadSql = fmt::format("SELECT {} FROM {} WHERE {} = {}", sql_browse_columns, sql_browse_query, browseColumnMapper->mapQuoted(BrowseCol::ServiceId), quote(serviceID));
    beginTransaction("loadObjectByServiceID");
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            commit("loadObjectByServiceID");
            return createObjectFromRow(DEFAULT_CLIENT_GROUP, row);
        }
    }
    commit("loadObjectByServiceID");

    return {};
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::findObjectByContentClass(const std::string& contentClass)
{
    auto srcParam = SearchParam(fmt::to_string(CDS_ID_ROOT),
        fmt::format("{}=\"{}\"", MetadataHandler::getMetaFieldName(M_CONTENT_CLASS), contentClass),
        "", 0, 1, false, DEFAULT_CLIENT_GROUP);
    int numMatches = 0;
    log_debug("Running content class search for '{}'", contentClass);
    auto result = this->search(srcParam, &numMatches);
    log_debug("Content class search {} returned {}", contentClass, numMatches);
    return result;
}

std::vector<int> SQLDatabase::getServiceObjectIDs(char servicePrefix)
{
    auto getSql = fmt::format("SELECT {} FROM {} WHERE {} LIKE {}",
        identifier("id"), identifier(CDS_OBJECT_TABLE),
        identifier("service_id"), quote(std::string(1, servicePrefix) + '%'));

    beginTransaction("getServiceObjectIDs");
    auto res = select(getSql);
    commit("getServiceObjectIDs");
    if (!res)
        throw_std_runtime_error("db error");

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

    if (param.getDynamicContainers() && dynamicContainers.find(parent->getID()) != dynamicContainers.end()) {
        auto dynConfig = config->getDynamicContentListOption(CFG_SERVER_DYNAMIC_CONTENT_LIST)->get(parent->getLocation());
        if (dynConfig) {
            auto reqCount = (param.getRequestedCount() <= 0 || param.getRequestedCount() > dynConfig->getMaxCount()) ? dynConfig->getMaxCount() : param.getRequestedCount();
            auto srcParam = SearchParam(fmt::to_string(parent->getParentID()), dynConfig->getFilter(), dynConfig->getSort(), // get params from config
                param.getStartingIndex(), reqCount, false, param.getGroup()); // get params from browse
            int numMatches = 0;
            auto result = this->search(srcParam, &numMatches);
            param.setTotalMatches(numMatches);
            return result;
        }
        log_warning("Dynamic content {} error '{}'", parent->getID(), parent->getLocation().string());
    }

    bool getContainers = param.getFlag(BROWSE_CONTAINERS);
    bool getItems = param.getFlag(BROWSE_ITEMS);
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
        int count = param.getRequestedCount();
        bool doLimit = true;
        if (!count) {
            if (param.getStartingIndex())
                count = std::numeric_limits<int>::max();
            else
                doLimit = false;
        }

        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::ParentId), parent->getID()));

        if (parent->getID() == CDS_ID_ROOT && hideFsRoot)
            where.push_back(fmt::format("{} != {:d}", browseColumnMapper->mapQuoted(BrowseCol::Id), CDS_ID_FS_ROOT));

        // order by code..
        auto orderByCode = [&]() {
            std::string orderQb;
            if (param.getFlag(BROWSE_TRACK_SORT)) {
                orderQb = fmt::format("{},{}", browseColumnMapper->mapQuoted(BrowseCol::PartNumber), browseColumnMapper->mapQuoted(BrowseCol::TrackNumber));
            } else {
                SortParser sortParser(browseColumnMapper, metaColumnMapper, param.getSortCriteria());
                orderQb = sortParser.parse(addColumns, addJoin);
            }
            if (orderQb.empty()) {
                orderQb = browseColumnMapper->mapQuoted(BrowseCol::DcTitle);
            }
            return orderQb;
        };

        if (!getContainers && !getItems) {
            auto zero = std::string("0 = 1");
            where.push_back(std::move(zero));
        } else if (getContainers && !getItems) {
            where.push_back(fmt::format("{} = {:d}", browseColumnMapper->mapQuoted(BrowseCol::ObjectType), OBJECT_TYPE_CONTAINER));
            orderBy = fmt::format(" ORDER BY {}", orderByCode());
        } else if (!getContainers && getItems) {
            where.push_back(fmt::format("({0} & {1}) = {1}", browseColumnMapper->mapQuoted(BrowseCol::ObjectType), OBJECT_TYPE_ITEM));
            orderBy = fmt::format(" ORDER BY {}", orderByCode());
        } else {
            // ORDER BY (object_type = OBJECT_TYPE_CONTAINER) ensures that containers are returned before items
            orderBy = fmt::format(" ORDER BY ({} = {}) DESC, {}", browseColumnMapper->mapQuoted(BrowseCol::ObjectType), OBJECT_TYPE_CONTAINER, orderByCode());
        }
        if (doLimit)
            limit = fmt::format(" LIMIT {} OFFSET {}", count, param.getStartingIndex());
    } else { // metadata
        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::Id), parent->getID()));
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

    if (param.getDynamicContainers() && config->getBoolOption(CFG_SERVER_DYNAMIC_CONTENT_LIST_ENABLED) && getContainers && param.getStartingIndex() == 0 && param.getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        auto dynContent = config->getDynamicContentListOption(CFG_SERVER_DYNAMIC_CONTENT_LIST);
        if (dynamicContainers.size() < dynContent->size()) {
            for (std::size_t count = 0; count < dynContent->size(); count++) {
                auto dynConfig = dynContent->get(count);
                if (parent->getLocation() == dynConfig->getLocation().parent_path() || (parent->getLocation().empty() && dynConfig->getLocation().parent_path() == "/")) {
                    auto dynId = std::int32_t(std::int64_t(-(parent->getID() + 1)) * 10000 - count);
                    // create runtime container
                    if (dynamicContainers.find(dynId) == dynamicContainers.end()) {
                        auto dynFolder = std::make_shared<CdsContainer>();
                        dynFolder->setTitle(dynConfig->getTitle());
                        dynFolder->setID(dynId);
                        dynFolder->setParentID(parent->getID());
                        dynFolder->setLocation(dynConfig->getLocation());
                        dynFolder->setClass("object.container.dynamicFolder");

                        auto image = dynConfig->getImage();
                        std::error_code ec;
                        if (!image.empty() && isRegularFile(image, ec)) {
                            auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, CdsResource::Purpose::Thumbnail);
                            std::string type = image.extension().string().substr(1);
                            std::string mimeType = mime->getMimeType(image, fmt::format("image/{}", type));
                            resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
                            resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, image);
                            dynFolder->addResource(resource);
                        }
                        dynamicContainers.emplace(dynId, std::move(dynFolder));
                    }
                    result.push_back(dynamicContainers[dynId]);
                    childCount++;
                }
            }
        } else {
            for (auto&& [dynId, dynFolder] : dynamicContainers) {
                if (dynFolder->getParentID() == parent->getID()) {
                    result.push_back(dynFolder);
                    childCount++;
                }
            }
        }
        param.setTotalMatches(childCount);
    }
    return result;
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::search(const SearchParam& param, int* numMatches)
{
    auto searchParser = SearchParser(*sqlEmitter, param.getSearchCriteria());
    std::shared_ptr<ASTNode> rootNode = searchParser.parse();
    std::string searchSQL(rootNode->emitSQL());
    if (searchSQL.empty())
        throw_std_runtime_error("failed to generate SQL for search");
    if (param.getSearchableContainers()) {
        searchSQL.append(fmt::format(" AND ({0} & {1} = {1} OR {2} != {3})",
            searchColumnMapper->mapQuoted(SearchCol::Flags), OBJECT_FLAG_SEARCHABLE, searchColumnMapper->mapQuoted(SearchCol::ObjectType), OBJECT_TYPE_CONTAINER));
    }

    bool rootContainer = param.getContainerID() == "0";

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

    log_debug("Search count resolves to SQL [{}]", countSQL);
    beginTransaction("search");
    auto sqlResult = select(countSQL);
    commit("search");

    auto countRow = sqlResult->nextRow();
    if (countRow) {
        *numMatches = countRow->col_int(0, 0);
    }

    std::string addColumns;
    std::string addJoin;
    // order by code..
    auto orderByCode = [&]() {
        SortParser sortParser(searchColumnMapper, metaColumnMapper, param.getSortCriteria());
        auto orderQb = sortParser.parse(addColumns, addJoin);
        if (orderQb.empty()) {
            orderQb = searchColumnMapper->mapQuoted(SearchCol::DcTitle);
        }
        return orderQb;
    };

    auto orderBy = fmt::format(" ORDER BY {}", orderByCode());
    std::string limit;

    std::size_t startingIndex = param.getStartingIndex();
    std::size_t requestedCount = param.getRequestedCount();
    if (startingIndex > 0 || requestedCount > 0) {
        limit = fmt::format(" LIMIT {} OFFSET {}", (requestedCount == 0 ? 10000000000 : requestedCount), startingIndex);
    }

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

    log_debug("Search resolves to SQL [{}]", retrievalSQL);
    beginTransaction("search 2");
    sqlResult = select(retrievalSQL);
    commit("search 2");

    std::vector<std::shared_ptr<CdsObject>> result;
    result.reserve(sqlResult->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        result.push_back(createObjectFromSearchRow(param.getGroup(), row));
    }

    if (result.size() < requestedCount) {
        *numMatches = startingIndex + result.size(); // make sure we do not report too many hits
    }

    return result;
}

int SQLDatabase::getChildCount(int contId, bool containers, bool items, bool hideFsRoot)
{
    if (!containers && !items)
        return 0;

    auto where = std::vector {
        fmt::format("{} = {}", identifier("parent_id"), contId)
    };
    if (containers && !items)
        where.push_back(fmt::format("{} = {}", identifier("object_type"), OBJECT_TYPE_CONTAINER));
    else if (items && !containers)
        where.push_back(fmt::format("({0} & {1}) = {1}", identifier("object_type"), OBJECT_TYPE_ITEM));
    if (contId == CDS_ID_ROOT && hideFsRoot) {
        where.push_back(fmt::format("{} != {:d}", identifier("id"), CDS_ID_FS_ROOT));
    }

    beginTransaction("getChildCount");
    auto res = select(fmt::format("SELECT COUNT(*) FROM {} WHERE {}", identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND ")));
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
        fmt::format("{} IN ({})", identifier("parent_id"), fmt::join(contId, ","))
    };
    if (containers && !items)
        where.push_back(fmt::format("{} = {}", identifier("object_type"), OBJECT_TYPE_CONTAINER));
    else if (items && !containers)
        where.push_back(fmt::format("({0} & {1}) = {1}", identifier("object_type"), OBJECT_TYPE_ITEM));
    if (hideFsRoot && std::find(contId.begin(), contId.end(), CDS_ID_ROOT) != contId.end()) {
        where.push_back(fmt::format("{} != {:d}", identifier("id"), CDS_ID_FS_ROOT));
    }

    beginTransaction("getChildCounts");
    auto res = select(fmt::format("SELECT {0}, COUNT(*) FROM {1} WHERE {2} GROUP BY {0} ORDER BY {0}",
        identifier("parent_id"), identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND ")));
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
        identifier("mime_type"), identifier(CDS_OBJECT_TABLE)));
    commit("getMimeTypes");

    if (!res)
        throw_std_runtime_error("db error");

    std::vector<std::string> arr;
    arr.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        arr.push_back(row->col(0));
    }

    return arr;
}

std::shared_ptr<CdsObject> SQLDatabase::findObjectByPath(const fs::path& fullpath, bool wasRegularFile)
{
    std::string dbLocation = [&fullpath, wasRegularFile] {
        std::error_code ec;
        if (wasRegularFile || isRegularFile(fullpath, ec))
            return addLocationPrefix(LOC_FILE_PREFIX, fullpath);
        return addLocationPrefix(LOC_DIR_PREFIX, fullpath);
    }();

    auto where = std::vector {
        fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::LocationHash), quote(stringHash(dbLocation))),
        fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::Location), quote(dbLocation)),
        fmt::format("{} IS NULL", browseColumnMapper->mapQuoted(BrowseCol::RefId)),
    };
    auto findSql = fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1", sql_browse_columns, sql_browse_query, fmt::join(where, " AND "));

    beginTransaction("findObjectByPath");
    auto res = select(findSql);
    if (!res) {
        commit("findObjectByPath");
        throw_std_runtime_error("error while doing select: {}", findSql);
    }

    auto row = res->nextRow();
    if (!row) {
        commit("findObjectByPath");
        return nullptr;
    }
    auto result = createObjectFromRow(DEFAULT_CLIENT_GROUP, row);
    commit("findObjectByPath");
    return result;
}

int SQLDatabase::findObjectIDByPath(const fs::path& fullpath, bool wasRegularFile)
{
    auto obj = findObjectByPath(fullpath, wasRegularFile);
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

    auto obj = findObjectByPath(path);
    if (obj)
        return obj->getID();

    int parentID = ensurePathExistence(path.parent_path(), changedContainer);

    auto f2i = StringConverter::f2i(config);
    if (changedContainer && *changedContainer == INVALID_OBJECT_ID)
        *changedContainer = parentID;

    std::vector<std::pair<std::string, std::string>> itemMetadata;
    itemMetadata.emplace_back(MetadataHandler::getMetaFieldName(M_DATE), fmt::format("{:%FT%T%z}", fmt::localtime(toSeconds(fs::last_write_time(path)).count())));
    return createContainer(parentID, f2i->convert(path.filename()), path, OBJECT_FLAG_RESTRICTED, false, "", INVALID_OBJECT_ID, itemMetadata);
}

int SQLDatabase::createContainer(int parentID, const std::string& name, const std::string& virtualPath, int flags, bool isVirtual, const std::string& upnpClass, int refID, const std::vector<std::pair<std::string, std::string>>& itemMetadata)
{
    // log_debug("Creating Container: parent: {}, name: {}, path {}, isVirt: {}, upnpClass: {}, refId: {}",
    // parentID, name.c_str(), path.c_str(), isVirtual, upnpClass.c_str(), refID);
    if (refID > 0) {
        auto refObj = loadObject(refID);
        if (!refObj)
            throw_std_runtime_error("tried to create container with refID set, but refID doesn't point to an existing object");
    }
    std::string dbLocation = addLocationPrefix((isVirtual ? LOC_VIRT_PREFIX : LOC_DIR_PREFIX), virtualPath);

    auto fields = std::vector {
        identifier("parent_id"),
        identifier("object_type"),
        identifier("flags"),
        identifier("upnp_class"),
        identifier("dc_title"),
        identifier("location"),
        identifier("location_hash"),
        identifier("ref_id"),
    };
    auto values = std::vector {
        fmt::to_string(parentID),
        fmt::to_string(OBJECT_TYPE_CONTAINER),
        fmt::to_string(flags),
        !upnpClass.empty() ? quote(upnpClass) : quote(UPNP_CLASS_CONTAINER),
        quote(name),
        quote(dbLocation),
        quote(stringHash(dbLocation)),
        (refID > 0) ? fmt::to_string(refID) : fmt::to_string(SQL_NULL),
    };

    beginTransaction("createContainer");
    int newId = insert(CDS_OBJECT_TABLE, fields, values, true); // true = get last id#
    log_debug("Created object row, id: {}", newId);

    if (!itemMetadata.empty()) {
        auto mfields = std::vector {
            identifier("item_id"),
            identifier("property_name"),
            identifier("property_value"),
        };
        std::vector<std::vector<std::string>> valuesets;
        valuesets.reserve(itemMetadata.size());
        const std::string newIdStr = fmt::to_string(newId);
        for (auto&& [key, val] : itemMetadata) {
            valuesets.push_back({ newIdStr, quote(key), quote(val) });
        }
        insertMultipleRows(METADATA_TABLE, mfields, valuesets);
        log_debug("Wrote metadata for cds_object {}", newId);
    }
    commit("createContainer");

    return newId;
}

int SQLDatabase::insert(std::string_view tableName, const std::vector<SQLIdentifier>& fields, const std::vector<std::string>& values, bool getLastInsertId)
{
    assert(fields.size() == values.size());
    auto sql = fmt::format("INSERT INTO {} ({}) VALUES ({})", identifier(tableName), fmt::join(fields, ","), fmt::join(values, ","));
    return exec(sql, getLastInsertId);
}

void SQLDatabase::insertMultipleRows(std::string_view tableName, const std::vector<SQLIdentifier>& fields, const std::vector<std::vector<std::string>>& valuesets)
{
    if (valuesets.size() == 1) {
        insert(tableName, fields, valuesets.front());
    } else if (!valuesets.empty()) {
        std::vector<std::string> tuples;
        tuples.reserve(valuesets.size());
        for (const auto& values : valuesets) {
            assert(fields.size() == values.size());
            tuples.push_back(fmt::format("({})", fmt::join(values, ",")));
        }
        auto sql = fmt::format("INSERT INTO {} ({}) VALUES {}", identifier(tableName), fmt::join(fields, ","), fmt::join(tuples, ","));
        exec(sql);
    }
}

void SQLDatabase::deleteAll(std::string_view tableName)
{
    exec(fmt::format("DELETE FROM {}", identifier(tableName)));
}

void SQLDatabase::deleteRows(std::string_view tableName, std::string_view key, const std::vector<int>& values)
{
    exec(fmt::format("DELETE FROM {} WHERE {} IN ({})", identifier(tableName), identifier(key), fmt::join(values, ",")));
}

fs::path SQLDatabase::buildContainerPath(int parentID, const std::string& title)
{
    if (parentID == CDS_ID_ROOT)
        return title;

    beginTransaction("buildContainerPath");
    auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} = {3} LIMIT 1",
        identifier("location"), identifier(CDS_OBJECT_TABLE), identifier("id"), parentID));
    commit("buildContainerPath");
    if (!res)
        return {};

    auto row = res->nextRow();
    if (!row)
        return {};

    auto [path, prefix] = stripLocationPrefix(fmt::format("{}{}{}", row->col(0), VIRTUAL_CONTAINER_SEPARATOR, title));
    if (prefix != LOC_VIRT_PREFIX)
        throw_std_runtime_error("Tried to build a virtual container path with an non-virtual parentID");

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
    std::string dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, virtualPath);

    beginTransaction("addContainer");
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} = {} AND {} = {} LIMIT 1", identifier("id"), identifier(CDS_OBJECT_TABLE), identifier("location_hash"), quote(stringHash(dbLocation)), identifier("location"), quote(dbLocation)));
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

    if (cont->getMetaData(M_DATE).empty())
        cont->addMetaData(M_DATE, fmt::format("{:%FT%T%z}", fmt::localtime(cont->getMTime().count())));

    *containerID = createContainer(parentContainerId, cont->getTitle(), virtualPath, cont->getFlags(), true, cont->getClass(), cont->getFlag(OBJECT_FLAG_PLAYLIST_REF) ? cont->getRefID() : INVALID_OBJECT_ID, cont->getMetaData());
    return true;
}

std::string SQLDatabase::addLocationPrefix(char prefix, const fs::path& path)
{
    return fmt::format("{}{}", prefix, path.string());
}

std::pair<fs::path, char> SQLDatabase::stripLocationPrefix(std::string_view dbLocation)
{
    if (dbLocation.empty())
        return { "", LOC_ILLEGAL_PREFIX };

    return { dbLocation.substr(1), dbLocation.at(0) };
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromRow(const std::string& group, const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, BrowseCol::ObjectType));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, BrowseCol::Id)));
    obj->setRefID(stoiString(getCol(row, BrowseCol::RefId)));

    obj->setParentID(std::stoi(getCol(row, BrowseCol::ParentId)));
    obj->setTitle(getCol(row, BrowseCol::DcTitle));
    obj->setClass(fallbackString(getCol(row, BrowseCol::UpnpClass), getCol(row, BrowseCol::RefUpnpClass)));
    obj->setFlags(std::stoi(getCol(row, BrowseCol::Flags)));
    obj->setMTime(std::chrono::seconds(stoulString(getCol(row, BrowseCol::LastModified))));
    obj->setUTime(std::chrono::seconds(stoulString(getCol(row, BrowseCol::LastUpdated))));

    auto metaData = retrieveMetaDataForObject(obj->getID());
    if (!metaData.empty()) {
        obj->setMetaData(std::move(metaData));
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        metaData = retrieveMetaDataForObject(obj->getRefID());
        if (!metaData.empty())
            obj->setMetaData(std::move(metaData));
    }

    std::string auxdataStr = fallbackString(getCol(row, BrowseCol::Auxdata), getCol(row, BrowseCol::RefAuxdata));
    std::map<std::string, std::string> aux = dictDecode(auxdataStr);
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

    obj->setVirtual((obj->getRefID() && obj->isPureItem()) || (obj->isItem() && !obj->isPureItem())); // gets set to true for virtual containers below

    int matchedTypes = 0;

    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        cont->setUpdateID(std::stoi(getCol(row, BrowseCol::UpdateId)));
        auto [location, locationPrefix] = stripLocationPrefix(getCol(row, BrowseCol::Location));
        cont->setLocation(location);
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);

        std::string autoscanPersistent = getCol(row, BrowseCol::AsPersistent);
        if (!autoscanPersistent.empty()) {
            if (remapBool(autoscanPersistent))
                cont->setAutoscanType(OBJECT_AUTOSCAN_CFG);
            else
                cont->setAutoscanType(OBJECT_AUTOSCAN_UI);
        } else
            cont->setAutoscanType(OBJECT_AUTOSCAN_NONE);
        matchedTypes++;
    }

    if (obj->isItem()) {
        if (!resourceZeroOk)
            throw_std_runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(fallbackString(getCol(row, BrowseCol::MimeType), getCol(row, BrowseCol::RefMimeType)));
        if (obj->isPureItem()) {
            if (!obj->isVirtual())
                item->setLocation(stripLocationPrefix(getCol(row, BrowseCol::Location)).first);
            else
                item->setLocation(stripLocationPrefix(getCol(row, BrowseCol::RefLocation)).first);
        } else // URLs
        {
            item->setLocation(fallbackString(getCol(row, BrowseCol::Location), getCol(row, BrowseCol::RefLocation)));
        }

        item->setTrackNumber(stoiString(getCol(row, BrowseCol::TrackNumber)));
        item->setPartNumber(stoiString(getCol(row, BrowseCol::PartNumber)));

        if (!getCol(row, BrowseCol::RefServiceId).empty())
            item->setServiceID(getCol(row, BrowseCol::RefServiceId));
        else
            item->setServiceID(getCol(row, BrowseCol::ServiceId));

        if (!group.empty()) {
            auto playStatus = getPlayStatus(group, obj->getID());
            if (playStatus)
                item->setPlayStatus(playStatus);
        }
        matchedTypes++;
    }

    if (!matchedTypes) {
        throw DatabaseException("", fmt::format("Unknown object type: {}", objectType));
    }

    return obj;
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromSearchRow(const std::string& group, const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, SearchCol::ObjectType));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, SearchCol::Id)));
    obj->setRefID(stoiString(getCol(row, SearchCol::RefId)));

    obj->setParentID(std::stoi(getCol(row, SearchCol::ParentId)));
    obj->setTitle(getCol(row, SearchCol::DcTitle));
    obj->setClass(getCol(row, SearchCol::UpnpClass));
    obj->setFlags(std::stoi(getCol(row, SearchCol::Flags)));

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
            throw_std_runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(getCol(row, SearchCol::MimeType));
        if (obj->isPureItem()) {
            item->setLocation(stripLocationPrefix(getCol(row, SearchCol::Location)).first);
        } else { // URLs
            item->setLocation(getCol(row, SearchCol::Location));
        }

        item->setPartNumber(stoiString(getCol(row, SearchCol::PartNumber)));
        item->setTrackNumber(stoiString(getCol(row, SearchCol::TrackNumber)));

        auto playStatus = getPlayStatus(group, obj->getID());
        if (playStatus)
            item->setPlayStatus(playStatus);
    } else if (!obj->isContainer()) {
        throw DatabaseException("", fmt::format("Unknown object type: {}", objectType));
    }

    return obj;
}

std::vector<std::pair<std::string, std::string>> SQLDatabase::retrieveMetaDataForObject(int objectId)
{
    auto query = fmt::format("{} FROM {} WHERE {} = {}",
        sql_meta_query, identifier(METADATA_TABLE), identifier("item_id"), objectId);
    auto res = select(query);
    if (!res)
        return {};

    std::vector<std::pair<std::string, std::string>> metaData;
    metaData.reserve(res->getNumRows());

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        metaData.emplace_back(getCol(row, MetadataCol::PropertyName), getCol(row, MetadataCol::PropertyValue));
    }
    return metaData;
}

long long SQLDatabase::getFileStats(const StatsParam& stats)
{
    auto where = std::vector {
        fmt::format("{} != {:d}", searchColumnMapper->mapQuoted(SearchCol::ObjectType), OBJECT_TYPE_CONTAINER),
    };
    if (!stats.getMimeType().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::MimeType), quote(fmt::format("{}%", stats.getMimeType()))));
    }
    if (!stats.getUpnpClass().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::UpnpClass), quote(fmt::format("{}%", stats.getUpnpClass()))));
    }
    where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::Location), quote(fmt::format("{}%", stats.getVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX))));

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
        fmt::format("{} != {:d}", searchColumnMapper->mapQuoted(SearchCol::ObjectType), OBJECT_TYPE_CONTAINER),
    };
    if (!stats.getMimeType().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::MimeType), quote(fmt::format("{}%", stats.getMimeType()))));
    }
    if (!stats.getUpnpClass().empty()) {
        where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::UpnpClass), quote(fmt::format("{}%", stats.getUpnpClass()))));
    }
    where.push_back(fmt::format("{} LIKE {}", searchColumnMapper->mapQuoted(SearchCol::Location), quote(fmt::format("{}%", stats.getVirtual() ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX))));

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
    auto query = fmt::format("SELECT {}, {} FROM {}{} WHERE {} GROUP BY {}", searchColumnMapper->mapQuoted(SearchCol::UpnpClass), mode, searchColumnMapper->tableQuoted(), join, fmt::join(where, " AND "), searchColumnMapper->mapQuoted(SearchCol::UpnpClass));
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
        identifier(CDS_OBJECT_TABLE), identifier("update_id"), identifier("id"), fmt::join(ids, ",")));

    auto res = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {0} IN ({3})",
        identifier("id"), identifier("update_id"), identifier(CDS_OBJECT_TABLE), fmt::join(ids, ",")));
    if (!res) {
        rollback("incrementUpdateIDs 2");
        throw_std_runtime_error("Error while fetching update ids");
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

std::unordered_set<int> SQLDatabase::getObjects(int parentID, bool withoutContainer)
{
    auto colId = identifier("id");
    auto table = identifier(CDS_OBJECT_TABLE);
    auto colParentId = identifier("parent_id");

    auto getSql = withoutContainer //
        ? fmt::format("SELECT {} FROM {} WHERE {} = {} AND {} != {}", colId, table, colParentId, parentID, identifier("object_type"), OBJECT_TYPE_CONTAINER)
        : fmt::format("SELECT {} FROM {} WHERE {} = {}", colId, table, colParentId, parentID);
    auto res = select(getSql);
    if (!res)
        throw_std_runtime_error("db error");

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
        throw_std_runtime_error("Tried to delete a forbidden ID ({})", *it);
    }

    auto res = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {0} IN ({3})",
        identifier("id"), identifier("object_type"), identifier(CDS_OBJECT_TABLE), fmt::join(list, ",")));
    if (!res)
        throw_std_runtime_error("sql error");

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
        asColumnMapper->mapQuoted(AutoscanCol::Id), asColumnMapper->mapQuoted(AutoscanCol::Persistent), browseColumnMapper->mapQuoted(BrowseCol::Location),
        asColumnMapper->tableQuoted(), browseColumnMapper->tableQuoted(),
        browseColumnMapper->mapQuoted(BrowseCol::Id), asColumnMapper->mapQuoted(AutoscanCol::ObjId), browseColumnMapper->mapQuoted(BrowseCol::Id), fmt::join(objectIDs, ","));
    log_debug("{}", sel);

    beginTransaction("_removeObjects");
    auto res = select(sel);
    if (res) {
        log_debug("relevant autoscans!");
        std::vector<int> deleteAs;
        std::unique_ptr<SQLRow> row;
        while ((row = res->nextRow())) {
            const int colId = row->col_int(0, INVALID_OBJECT_ID); // AutoscanCol::id
            bool persistent = remapBool(row->col_int(1, 0));
            if (persistent) {
                auto [location, prefix] = stripLocationPrefix(row->col(2));
                auto values = std::vector {
                    ColumnUpdate(identifier("obj_id"), SQL_NULL),
                    ColumnUpdate(identifier("location"), quote(location.string())),
                };
                updateRow(AUTOSCAN_TABLE, values, "id", colId);
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
    commit("_removeObjects");
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObject(int objectID, bool all)
{
    auto res = select(fmt::format("SELECT {}, {} FROM {} WHERE {} = {} LIMIT 1",
        identifier("object_type"), identifier("ref_id"), identifier(CDS_OBJECT_TABLE), identifier("id"), objectID));
    if (!res)
        return nullptr;

    auto row = res->nextRow();
    if (!row)
        return nullptr;

    const int objectType = row->col_int(0, 0);
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && !isContainer) {
        if (!row->isNullOrEmpty(1)) {
            const int refId = row->col_int(1, INVALID_OBJECT_ID);
            if (!IS_FORBIDDEN_CDS_ID(refId))
                objectID = refId;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw_std_runtime_error("Tried to delete a forbidden ID ({})", objectID);
    std::vector<std::int32_t> itemIds;
    std::vector<std::int32_t> containerIds;
    if (isContainer) {
        containerIds.push_back(objectID);
    } else {
        itemIds.push_back(objectID);
    }
    auto changedContainers = _recursiveRemove(itemIds, containerIds, all);
    return _purgeEmptyContainers(std::move(changedContainers));
}

Database::ChangedContainers SQLDatabase::_recursiveRemove(
    const std::vector<std::int32_t>& items, const std::vector<std::int32_t>& containers,
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
    auto parentSql = fmt::format("SELECT DISTINCT {0}parent_id{1} FROM {0}{2}{1} WHERE {0}id{1} IN", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE);
    auto itemSql = fmt::format("SELECT DISTINCT {0}id{1}, {0}parent_id{1} FROM {0}{2}{1} WHERE {0}ref_id{1} IN", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE);
    auto containersSql = all //
        ? fmt::format("SELECT DISTINCT {0}id{1}, {0}object_type{1}, {0}ref_id{1} FROM {0}{2}{1} WHERE {0}parent_id{1} IN", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE) //
        : fmt::format("SELECT DISTINCT {0}id{1}, {0}object_type{1} FROM {0}{2}{1} WHERE {0}parent_id{1} IN", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE);

    // collect container for update signals
    if (!containers.empty()) {
        std::copy(containers.begin(), containers.end(), std::back_inserter(parentIds));
        auto sql = fmt::format("{} ({})", parentSql, fmt::join(parentIds, ","));
        res = select(sql);
        if (!res)
            throw DatabaseException("", fmt::format("Sql error: {}", sql));
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
                throw DatabaseException("", fmt::format("Sql error: {}", sql));
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
                throw DatabaseException("", fmt::format("Sql error: {}", sql));
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
                throw DatabaseException("", fmt::format("Sql error: {}", sql));
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
            throw_std_runtime_error("There seems to be an infinite loop...");
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

    auto tabAlias = identifier("fol");
    auto childAlias = identifier("cld");
    auto colId = identifier("id");
    auto fields = std::vector {
        fmt::format("{}.{}", tabAlias, colId),
        fmt::format("COUNT({}.{})", childAlias, identifier("parent_id")),
        fmt::format("{}.{}", tabAlias, identifier("parent_id")),
        fmt::format("{}.{}", tabAlias, identifier("flags")),
    };
    std::string selectSql = fmt::format("SELECT {2} FROM {5} {3} LEFT JOIN {5} {4} ON {3}.{0}id{1} = {4}.{0}parent_id{1} WHERE {3}.{0}object_type{1} = {6} AND {3}.{0}id{1}",
        table_quote_begin, table_quote_end, fmt::join(fields, ","), tabAlias, childAlias, identifier(CDS_OBJECT_TABLE), OBJECT_TYPE_CONTAINER);

    std::vector<std::int32_t> del;

    std::unique_ptr<SQLRow> row;

    auto selUi = std::vector(maybeEmpty.ui);
    auto selUpnp = std::vector(maybeEmpty.upnp);

    bool again;
    int count = 0;
    do {
        again = false;

        if (!selUpnp.empty()) {
            auto sql = fmt::format("{} IN ({}) GROUP BY {}.{}", selectSql, fmt::join(selUpnp, ","), tabAlias, colId);
            log_debug("upnp-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUpnp.clear();
            if (!res)
                throw_std_runtime_error("db error");
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

        if (!selUi.empty()) {
            auto sql = fmt::format("{} IN ({}) GROUP BY {}.{}", selectSql, fmt::join(selUi, ","), tabAlias, colId);
            log_debug("ui-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUi.clear();
            if (!res)
                throw_std_runtime_error("db error");
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

        // log_debug("selecting: {}; removing: {}", selectSql, fmt::join(del, ","));
        if (!del.empty()) {
            _removeObjects(del);
            del.clear();
            if (!selUi.empty() || !selUpnp.empty())
                again = true;
        }
        if (count++ >= MAX_REMOVE_RECURSION)
            throw_std_runtime_error("there seems to be an infinite loop...");
    } while (again);

    auto& changedUi = changedContainers->ui;
    auto& changedUpnp = changedContainers->upnp;
    if (!selUi.empty()) {
        std::copy(selUi.begin(), selUi.end(), std::back_inserter(changedUi));
        std::copy(selUi.begin(), selUi.end(), std::back_inserter(changedUpnp));
    }
    if (!selUpnp.empty()) {
        std::copy(selUpnp.begin(), selUpnp.end(), std::back_inserter(changedUpnp));
    }
    // log_debug("end; changedContainers (upnp): {}", fmt::join(changedUpnp, ","));
    // log_debug("end; changedContainers (ui): {}", fmt::join(changedUi, ","));
    log_debug("end; changedContainers (upnp): {}", changedUpnp.size());
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
    auto fields = std::vector {
        identifier("item"),
        identifier("key"),
        identifier("item_value"),
        identifier("status"),
    };
    std::vector<ConfigValue> result;
    auto res = select(fmt::format("SELECT {} FROM {}", fmt::join(fields, ", "), identifier(CONFIG_VALUE_TABLE)));
    if (!res)
        return result;

    result.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        result.push_back({ row->col(1),
            row->col(0),
            row->col(2),
            row->col(3) });
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

void SQLDatabase::updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status)
{
    auto res = select(fmt::format("SELECT 1 FROM {} WHERE {} = {} LIMIT 1",
        identifier(CONFIG_VALUE_TABLE), identifier("item"), quote(item)));
    if (!res || res->getNumRows() == 0) {
        auto fields = std::vector {
            identifier("item"),
            identifier("key"),
            identifier("item_value"),
            identifier("status"),
        };
        auto values = std::vector {
            quote(item),
            quote(key),
            quote(value),
            quote(status),
        };
        insert(CONFIG_VALUE_TABLE, fields, values);
        log_debug("inserted for {} as {} = {}", key, item, value);
    } else {
        auto updates = std::vector {
            ColumnUpdate(identifier("item_value"), quote(value))
        };
        updateRow(CONFIG_VALUE_TABLE, updates, "item", item);
        log_debug("updated for {} as {} = {}", key, item, value);
    }
}

/* clients methods */
std::vector<ClientCacheEntry> SQLDatabase::getClients()
{
    auto fields = std::vector {
        identifier("addr"),
        identifier("port"),
        identifier("addrFamily"),
        identifier("userAgent"),
        identifier("last"),
        identifier("age"),
    };
    std::vector<ClientCacheEntry> result;
    auto res = select(fmt::format("SELECT {} FROM {}", fmt::join(fields, ", "), identifier(CLIENTS_TABLE)));
    if (!res)
        return result;

    result.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto net = std::make_shared<GrbNet>(row->col(0), row->col_int(2, AF_INET));
        net->setPort(row->col_int(1, 0));
        result.emplace_back(net,
            row->col(3),
            std::chrono::seconds(row->col_int(4, 0)),
            std::chrono::seconds(row->col_int(5, 0)),
            nullptr);
    }

    log_debug("Loaded {} items from {}", result.size(), CLIENTS_TABLE);
    return result;
}

void SQLDatabase::saveClients(const std::vector<ClientCacheEntry>& cache)
{
    deleteAll(CLIENTS_TABLE);
    auto fields = std::vector {
        identifier("addr"),
        identifier("port"),
        identifier("addrFamily"),
        identifier("userAgent"),
        identifier("last"),
        identifier("age"),
    };
    for (auto& client : cache) {
        auto values = std::vector {
            quote(client.addr->getNameInfo(false)),
            quote(client.addr->getPort()),
            quote(client.addr->getAdressFamily()),
            quote(client.userAgent),
            quote(client.last.count()),
            quote(client.age.count()),
        };
        insert(CLIENTS_TABLE, fields, values);
    }
}

std::shared_ptr<ClientStatusDetail> SQLDatabase::getPlayStatus(const std::string& group, int objectId)
{
    auto fields = std::vector {
        identifier("group"),
        identifier("item_id"),
        identifier("playCount"),
        identifier("lastPlayed"),
        identifier("lastPlayedPosition"),
        identifier("bookMarkPos"),
    };
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} = {} AND {} = {}",
        fmt::join(fields, ", "), identifier(PLAYSTATUS_TABLE),
        identifier("group"), quote(group), identifier("item_id"), quote(objectId)));
    if (!res)
        return {};

    std::unique_ptr<SQLRow> row;
    if ((row = res->nextRow())) {
        log_debug("Loaded {},{} items from {}", group, objectId, PLAYSTATUS_TABLE);
        return std::make_shared<ClientStatusDetail>(row->col(0), row->col_int(1, objectId), row->col_int(2, 0), row->col_int(3, 0), row->col_int(4, 0), row->col_int(5, 0));
    }

    return {};
}

std::vector<std::shared_ptr<ClientStatusDetail>> SQLDatabase::getPlayStatusList(int objectId)
{
    auto fields = std::vector {
        identifier("group"),
        identifier("item_id"),
        identifier("playCount"),
        identifier("lastPlayed"),
        identifier("lastPlayedPosition"),
        identifier("bookMarkPos"),
    };
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} = {}",
        fmt::join(fields, ", "), identifier(PLAYSTATUS_TABLE),
        identifier("item_id"), quote(objectId)));
    if (!res)
        return {};

    std::vector<std::shared_ptr<ClientStatusDetail>> result;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        log_debug("Loaded {},{} items from {}", row->col(0), objectId, PLAYSTATUS_TABLE);
        auto status = std::make_shared<ClientStatusDetail>(row->col(0), row->col_int(1, objectId), row->col_int(2, 0), row->col_int(3, 0), row->col_int(4, 0), row->col_int(5, 0));
        result.push_back(std::move(status));
    }

    return result;
}

void SQLDatabase::savePlayStatus(const std::shared_ptr<ClientStatusDetail>& detail)
{
    exec(fmt::format("DELETE FROM {} WHERE {} = {} AND {} = {}", PLAYSTATUS_TABLE, identifier("group"), quote(detail->getGroup()), identifier("item_id"), quote(detail->getItemId())));
    auto fields = std::vector {
        identifier("group"),
        identifier("item_id"),
        identifier("playCount"),
        identifier("lastPlayed"),
        identifier("lastPlayedPosition"),
        identifier("bookMarkPos"),
    };
    auto values = std::vector {
        quote(detail->getGroup()),
        quote(detail->getItemId()),
        quote(detail->getPlayCount()),
        quote(detail->getLastPlayed().count()),
        quote(detail->getLastPlayedPosition().count()),
        quote(detail->getBookMarkPosition().count()),
    };
    insert(PLAYSTATUS_TABLE, fields, values);
}

std::vector<std::map<std::string, std::string>> SQLDatabase::getClientGroupStats()
{
    auto res = select(fmt::format("SELECT {}, COUNT(*), SUM({}), MAX({}), COUNT(NULLIF({},0)) FROM {} GROUP BY {}",
        identifier("group"), identifier("playCount"), identifier("lastPlayed"), identifier("bookMarkPos"), identifier(PLAYSTATUS_TABLE), identifier("group")));
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
        stats["last"] = fmt::format("{:%a %b %d %H:%M:%S %Y}", fmt::localtime(std::chrono::seconds(row->col_int(3, 0)).count()));
        stats["bookmarks"] = fmt::format("{}", row->col_int(4, -1));
        result.push_back(std::move(stats));
    }
    return result;
}

void SQLDatabase::updateAutoscanList(AutoscanDirectory::ScanMode scanmode, const std::shared_ptr<AutoscanList>& list)
{
    log_debug("Setting persistent autoscans untouched - scanmode: {};", AutoscanDirectory::mapScanmode(scanmode));

    beginTransaction("updateAutoscanList");
    exec(fmt::format("UPDATE {0} SET {1} = {2} WHERE {3} = {4} AND {5} = {6}",
        identifier(AUTOSCAN_TABLE), identifier("touched"), quote(false), identifier("persistent"), quote(true), identifier("scan_mode"), quote(AutoscanDirectory::mapScanmode(scanmode))));
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
            throw_std_runtime_error("AutoscanDirectoy with illegal location given to SQLDatabase::updateAutoscanPersistentList");

        int objectID = findObjectIDByPath(location);
        log_debug("objectID = {}", objectID);
        auto where = (objectID == INVALID_OBJECT_ID) ? fmt::format("{} = {}", identifier("location"), quote(location)) : fmt::format("{} = {}", identifier("obj_id"), objectID);

        beginTransaction("updateAutoscanList x");
        auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} LIMIT 1", identifier("id"), identifier(AUTOSCAN_TABLE), where));
        if (!res) {
            rollback("updateAutoscanList x");
            throw DatabaseException("", "query error while selecting from autoscan list");
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
    exec(fmt::format("DELETE FROM {0} WHERE {1} = {2} AND {3} = {4}", identifier(AUTOSCAN_TABLE), identifier("touched"), quote(false), identifier("scan_mode"), quote(AutoscanDirectory::mapScanmode(scanmode))));
    commit("updateAutoscanList delete");
}

std::shared_ptr<AutoscanList> SQLDatabase::getAutoscanList(AutoscanDirectory::ScanMode scanmode)
{
    std::string selectSql = fmt::format("{2} WHERE {0}{3}{1}.{0}scan_mode{1} = {4}", table_quote_begin, table_quote_end, sql_autoscan_query, AUS_ALIAS, quote(AutoscanDirectory::mapScanmode(scanmode)));

    auto res = select(selectSql);
    if (!res)
        throw DatabaseException("", "query error while fetching autoscan list");

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
    std::string selectSql = fmt::format("{} WHERE {}.{} = {}", sql_autoscan_query, identifier(ITM_ALIAS), identifier("id"), objectID);

    auto res = select(selectSql);
    if (!res)
        throw DatabaseException("", "query error while fetching autoscan");

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

    AutoscanDirectory::ScanMode mode = AutoscanDirectory::remapScanmode(getCol(row, AutoscanColumn::ScanMode));
    bool recursive = remapBool(getCol(row, AutoscanColumn::Recursive));
    int mt = getColInt(row, AutoscanColumn::MedaType, 0);
    bool hidden = remapBool(getCol(row, AutoscanColumn::Hidden));
    bool persistent = remapBool(getCol(row, AutoscanColumn::Persistent));
    int interval = 0;
    if (mode == AutoscanDirectory::ScanMode::Timed)
        interval = std::stoi(getCol(row, AutoscanColumn::Interval));
    auto lastModified = std::chrono::seconds(std::stol(getCol(row, AutoscanColumn::LastModified)));

    log_info("Loading autoscan location: {}; recursive: {}, mt: {}/{}, last_modified: {}", location.c_str(), recursive, mt, AutoscanDirectory::mapMediaType(mt), lastModified > std::chrono::seconds::zero() ? fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(lastModified.count())) : "unset");

    auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, persistent, interval, hidden, mt);
    dir->setObjectID(objectID);
    dir->setDatabaseID(databaseID);
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
        throw_std_runtime_error("addAutoscanDirectory called with adir==nullptr");
    if (adir->getDatabaseID() >= 0)
        throw_std_runtime_error("tried to add autoscan directory with a database id set");
    int objectID = (adir->getLocation() == FS_ROOT_DIRECTORY) ? CDS_ID_FS_ROOT : findObjectIDByPath(adir->getLocation());
    if (!adir->persistent() && objectID < 0)
        throw_std_runtime_error("tried to add non-persistent autoscan directory with an illegal objectID or location");

    auto pathIds = _checkOverlappingAutoscans(adir);

    _autoscanChangePersistentFlag(objectID, true);

    auto fields = std::vector {
        identifier("obj_id"),
        identifier("scan_mode"),
        identifier("recursive"),
        identifier("media_type"),
        identifier("hidden"),
        identifier("interval"),
        identifier("last_modified"),
        identifier("persistent"),
        identifier("location"),
        identifier("path_ids"),
    };
    auto values = std::vector {
        objectID >= 0 ? quote(objectID) : SQL_NULL,
        quote(AutoscanDirectory::mapScanmode(adir->getScanMode())),
        quote(adir->getRecursive()),
        quote(adir->getMediaType()),
        quote(adir->getHidden()),
        quote(adir->getInterval().count()),
        quote(adir->getPreviousLMT().count()),
        quote(adir->persistent()),
        objectID >= 0 ? SQL_NULL : quote(adir->getLocation()),
        pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ","))),
    };
    adir->setDatabaseID(insert(AUTOSCAN_TABLE, fields, values, true));
}

void SQLDatabase::updateAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw_std_runtime_error("updateAutoscanDirectory called with adir==nullptr");

    log_debug("id: {}, obj_id: {}", adir->getDatabaseID(), adir->getObjectID());

    auto pathIds = _checkOverlappingAutoscans(adir);

    int objectID = adir->getObjectID();
    int objectIDold = _getAutoscanObjectID(adir->getDatabaseID());
    if (objectIDold != objectID) {
        _autoscanChangePersistentFlag(objectIDold, false);
        _autoscanChangePersistentFlag(objectID, true);
    }
    auto fields = std::vector {
        ColumnUpdate(identifier("obj_id"), objectID >= 0 ? quote(objectID) : SQL_NULL),
        ColumnUpdate(identifier("scan_mode"), quote(AutoscanDirectory::mapScanmode(adir->getScanMode()))),
        ColumnUpdate(identifier("recursive"), quote(adir->getRecursive())),
        ColumnUpdate(identifier("media_type"), quote(adir->getMediaType())),
        ColumnUpdate(identifier("hidden"), quote(adir->getHidden())),
        ColumnUpdate(identifier("interval"), quote(adir->getInterval().count())),
        ColumnUpdate(identifier("persistent"), quote(adir->persistent())),
        ColumnUpdate(identifier("location"), objectID >= 0 ? SQL_NULL : quote(adir->getLocation())),
        ColumnUpdate(identifier("path_ids"), pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ",")))),
        ColumnUpdate(identifier("touched"), quote(true)),
    };
    if (adir->getPreviousLMT() > std::chrono::seconds::zero()) {
        fields.emplace_back(identifier("last_modified"), quote(adir->getPreviousLMT().count()));
    }
    updateRow(AUTOSCAN_TABLE, fields, "id", adir->getDatabaseID());
}

void SQLDatabase::removeAutoscanDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    _removeAutoscanDirectory(adir->getDatabaseID());
}

void SQLDatabase::_removeAutoscanDirectory(int autoscanID)
{
    if (autoscanID == INVALID_OBJECT_ID)
        return;
    int objectID = _getAutoscanObjectID(autoscanID);
    deleteRow(AUTOSCAN_TABLE, "id", autoscanID);
    if (objectID != INVALID_OBJECT_ID)
        _autoscanChangePersistentFlag(objectID, false);
}

int SQLDatabase::_getAutoscanObjectID(int autoscanID)
{
    auto res = select(fmt::format("SELECT {} FROM {} WHERE {} = {} LIMIT 1",
        identifier("obj_id"), identifier(AUTOSCAN_TABLE), identifier("id"), autoscanID));
    if (!res)
        throw DatabaseException("", "error while doing select on ");
    auto row = res->nextRow();
    if (row && !row->isNullOrEmpty(0))
        return row->col_int(0, INVALID_OBJECT_ID);

    return INVALID_OBJECT_ID;
}

void SQLDatabase::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID || objectID == INVALID_OBJECT_ID_2)
        return;

    exec(fmt::format("UPDATE {0}{2}{1} SET {0}flags{1} = ({0}flags{1} {3}{4}) WHERE {0}id{1} = {5}", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, (persistent ? " | " : " & ~"), OBJECT_FLAG_PERSISTENT_CONTAINER, objectID));
}

void SQLDatabase::checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    (void)_checkOverlappingAutoscans(adir);
}

std::vector<int> SQLDatabase::_checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw_std_runtime_error("_checkOverlappingAutoscans called with adir==nullptr");
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID)
        return {};
    int databaseID = adir->getDatabaseID();

    std::unique_ptr<SQLRow> row;
    {
        auto where = std::vector {
            fmt::format("{} = {}", identifier("obj_id"), checkObjectID),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", identifier("id"), databaseID));

        auto res = select(fmt::format("SELECT {} FROM {} WHERE {}", identifier("id"), identifier(AUTOSCAN_TABLE), fmt::join(where, " AND ")));
        if (!res)
            throw_std_runtime_error("SQL error");

        row = res->nextRow();
        if (row) {
            auto obj = loadObject(checkObjectID);
            if (!obj)
                throw_std_runtime_error("Referenced object (by Autoscan) not found.");
            log_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw_std_runtime_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
        }
    }

    if (adir->getRecursive()) {
        auto where = std::vector {
            fmt::format("{} LIKE {}", identifier("path_ids"), quote(fmt::format("%,{},%", checkObjectID))),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", identifier("id"), databaseID));
        auto qRec = fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1", identifier("obj_id"), identifier(AUTOSCAN_TABLE), fmt::join(where, " AND "));
        log_debug("------------ {}", qRec);
        auto res = select(qRec);
        if (!res)
            throw_std_runtime_error("SQL error");
        row = res->nextRow();
        if (row) {
            const int objectID = row->col_int(0, INVALID_OBJECT_ID);
            log_debug("-------------- {}", objectID);
            auto obj = loadObject(objectID);
            if (!obj)
                throw_std_runtime_error("Referenced object (by Autoscan) not found.");
            log_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw_std_runtime_error("Overlapping Autoscans are not allowed. There is already an Autoscan set on {}", obj->getLocation().c_str());
        }
    }

    {
        auto pathIDs = getPathIDs(checkObjectID);
        if (pathIDs.empty())
            throw_std_runtime_error("getPathIDs returned nullptr");

        auto where = std::vector {
            fmt::format("{} IN ({})", identifier("obj_id"), fmt::join(pathIDs, ",")),
            fmt::format("{} = {}", identifier("recursive"), quote(true)),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{} != {}", identifier("id"), databaseID));
        auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1", identifier("obj_id"), identifier(AUTOSCAN_TABLE), fmt::join(where, " AND ")));
        if (!res)
            throw_std_runtime_error("SQL error");
        if (!(row = res->nextRow()))
            return pathIDs;
    }

    const int objectID = row->col_int(0, INVALID_OBJECT_ID);
    auto obj = loadObject(objectID);
    if (!obj) {
        throw_std_runtime_error("Referenced object (by Autoscan) not found.");
    }
    log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
    throw_std_runtime_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
}

std::vector<int> SQLDatabase::getPathIDs(int objectID)
{
    std::vector<int> pathIDs;
    if (objectID == INVALID_OBJECT_ID)
        return pathIDs;

    auto sel = fmt::format("SELECT {0}parent_id{1} FROM {0}{2}{1} WHERE {0}id{1} = ", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE);

    while (objectID != CDS_ID_ROOT) {
        pathIDs.push_back(objectID);
        auto res = select(fmt::format("{} {} LIMIT 1", sel, objectID));
        if (!res)
            break;
        auto row = res->nextRow();
        if (!row)
            break;
        objectID = row->col_int(0, INVALID_OBJECT_ID);
    }
    return pathIDs;
}

void SQLDatabase::generateMetaDataDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op, std::vector<AddUpdateTable>& operations) const
{
    const auto& dict = obj->getMetaData();
    operations.reserve(operations.size() + dict.size());
    if (op == Operation::Insert) {
        for (auto&& [key, val] : dict) {
            std::map<std::string, std::string> metadataSql;
            metadataSql.emplace("property_name", quote(key));
            metadataSql.emplace("property_value", quote(val));
            operations.emplace_back(METADATA_TABLE, std::move(metadataSql), Operation::Insert);
        }
    } else {
        // delete current metadata from DB
        operations.emplace_back(METADATA_TABLE, std::map<std::string, std::string>(), Operation::Delete);
        if (op != Operation::Delete) {
            for (auto&& [key, val] : dict) {
                std::map<std::string, std::string> metadataSql;
                metadataSql.emplace("property_name", quote(key));
                metadataSql.emplace("property_value", quote(val));
                operations.emplace_back(METADATA_TABLE, std::move(metadataSql), Operation::Insert);
            }
        }
    }
}

std::vector<std::shared_ptr<CdsResource>> SQLDatabase::retrieveResourcesForObject(int objectId)
{
    auto rsql = fmt::format("{} FROM {} WHERE {} = {} ORDER BY {}",
        sql_resource_query, identifier(RESOURCE_TABLE), identifier("item_id"), objectId, identifier("res_id"));
    log_debug("SQLDatabase::retrieveResourcesForObject {}", rsql);
    auto&& res = select(rsql);

    if (!res)
        return {};

    std::vector<std::shared_ptr<CdsResource>> resources;
    resources.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto resource = std::make_shared<CdsResource>(
            MetadataHandler::remapContentHandler(std::stoi(getCol(row, ResourceCol::HandlerType))),
            CdsResource::remapPurpose(std::stoi(getCol(row, ResourceCol::Purpose))),
            getCol(row, ResourceCol::Options),
            getCol(row, ResourceCol::Parameters));
        resource->setResId(resources.size());
        for (auto&& resAttrId : ResourceAttributeIterator()) {
            auto index = to_underlying(ResourceCol::Attributes) + to_underlying(resAttrId);
            auto value = row->col_c_str(index);
            if (value) {
                resource->addAttribute(resAttrId, value);
            }
        }
        resources.push_back(std::move(resource));
    }

    return resources;
}

void SQLDatabase::generateResourceDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op,
    std::vector<AddUpdateTable>& operations)
{
    const auto& resources = obj->getResources();
    operations.reserve(operations.size() + resources.size());
    if (op == Operation::Insert) {
        std::size_t resId = 0;
        for (auto&& resource : resources) {
            std::map<std::string, std::string> resourceSql;
            resourceSql["res_id"] = quote(resId);
            resourceSql["handlerType"] = quote(to_underlying(resource->getHandlerType()));
            resourceSql["purpose"] = quote(to_underlying(resource->getPurpose()));
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                resourceSql["options"] = quote(dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql["parameters"] = quote(dictEncode(parameters));
            }
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSql[CdsResource::getAttributeName(key)] = quote(val);
            }
            operations.emplace_back(RESOURCE_TABLE, std::move(resourceSql), Operation::Insert);
            resId++;
        }
    } else {
        // get current resoures from DB
        auto dbResources = retrieveResourcesForObject(obj->getID());
        std::size_t resId = 0;
        for (auto&& resource : resources) {
            Operation operation = resId < dbResources.size() ? Operation::Update : Operation::Insert;
            std::map<std::string, std::string> resourceSql;
            resourceSql["res_id"] = quote(resId);
            resourceSql["handlerType"] = quote(to_underlying(resource->getHandlerType()));
            resourceSql["purpose"] = quote(to_underlying(resource->getPurpose()));
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                resourceSql["options"] = quote(dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql["parameters"] = quote(dictEncode(parameters));
            }
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSql[CdsResource::getAttributeName(key)] = quote(val);
            }
            operations.emplace_back(RESOURCE_TABLE, std::move(resourceSql), operation);
            resId++;
        }
        // res_id in db resources but not obj resources, so needs a delete
        while (resId < dbResources.size()) {
            if (dbResources.at(resId)) {
                std::map<std::string, std::string> resourceSql;
                resourceSql["res_id"] = quote(resId);
                operations.emplace_back(RESOURCE_TABLE, std::move(resourceSql), Operation::Delete);
            }
            ++resId;
        }
    }
}

std::string SQLDatabase::sqlForInsert(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const
{
    const std::string& tableName = addUpdateTable.getTableName();

    if (tableName == CDS_OBJECT_TABLE && obj->getID() != INVALID_OBJECT_ID) {
        throw_std_runtime_error("Attempted to insert new object with ID!");
    }

    const auto& dict = addUpdateTable.getDict();

    std::vector<SQLIdentifier> fields;
    std::vector<std::string> values;
    fields.reserve(dict.size() + 1); // extra only used for METADATA_TABLE and RESOURCE_TABLE
    values.reserve(dict.size() + 1); // extra only used for METADATA_TABLE and RESOURCE_TABLE

    if (tableName == METADATA_TABLE || tableName == RESOURCE_TABLE) {
        fields.push_back(identifier("item_id"));
        values.push_back(fmt::to_string(obj->getID()));
    }
    for (auto&& field : tableColumnOrder.at(tableName)) {
        if (dict.find(field) != dict.end()) {
            fields.push_back(identifier(field));
            values.push_back(dict.at(field));
        }
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})", identifier(tableName), fmt::join(fields, ", "), fmt::join(values, ", "));
}

std::string SQLDatabase::sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const
{
    const std::string& tableName = addUpdateTable.getTableName();
    const auto& dict = addUpdateTable.getDict();

    if (tableName == METADATA_TABLE && dict.size() != 2)
        throw_std_runtime_error("sqlForUpdate called with invalid arguments");

    std::vector<std::string> fields;
    fields.reserve(dict.size());
    for (auto&& field : tableColumnOrder.at(tableName)) {
        if (dict.find(field) != dict.end()) {
            fields.push_back(fmt::format("{} = {}", identifier(field), dict.at(field)));
        }
    }

    std::vector<std::string> where;
    if (tableName == RESOURCE_TABLE) {
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("res_id"), dict.at("res_id")));
    } else if (tableName == METADATA_TABLE) {
        // relying on only one element when tableName is mt_metadata
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("property_name"), dict.begin()->second));
    } else {
        where.push_back(fmt::format("{} = {}", identifier("id"), obj->getID()));
    }

    return fmt::format("UPDATE {} SET {} WHERE {}", identifier(tableName), fmt::join(fields, ", "), fmt::join(where, " AND "));
}

std::string SQLDatabase::sqlForDelete(const std::shared_ptr<CdsObject>& obj, const AddUpdateTable& addUpdateTable) const
{
    const std::string& tableName = addUpdateTable.getTableName();
    const auto& dict = addUpdateTable.getDict();

    std::vector<std::string> where;
    if (tableName == RESOURCE_TABLE) {
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("res_id"), dict.at("res_id")));
    } else if (tableName == METADATA_TABLE) {
        if (!dict.empty() && dict.size() != 2)
            throw_std_runtime_error("sqlForDelete called with invalid arguments");
        // relying on only one element when tableName is mt_metadata
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        if (!dict.empty())
            where.push_back(fmt::format("{} = {}", identifier("property_name"), dict.begin()->second));
    } else {
        where.push_back(fmt::format("{} = {}", identifier("id"), obj->getID()));
    }

    return fmt::format("DELETE FROM {} WHERE {}", identifier(tableName), fmt::join(where, " AND "));
}

// column metadata is dropped in DBVERSION 12
bool SQLDatabase::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required");
    auto res = select(fmt::format("SELECT COUNT(*) FROM {} WHERE {} IS NOT NULL", identifier(CDS_OBJECT_TABLE), identifier("metadata")));
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    res = select(fmt::format("SELECT COUNT(*) FROM {}", identifier(METADATA_TABLE)));
    int metadataRowCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_metadata rows having metadata: {}", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required");
        return true;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata");

    auto resIds = select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
        identifier("id"), identifier("metadata"), identifier(CDS_OBJECT_TABLE)));
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
    std::map<std::string, std::string> dict = dictDecode(metadataStr);

    if (!dict.empty()) {
        log_debug("Migrating metadata for cds object {}", objectId);
        std::map<std::string, std::string> metadataSQLVals;
        for (auto&& [key, val] : dict) {
            metadataSQLVals[quote(key)] = quote(val);
        }
        auto fields = std::vector {
            identifier("item_id"),
            identifier("property_name"),
            identifier("property_value"),
        };
        std::vector<std::vector<std::string>> valuesets;
        valuesets.reserve(metadataSQLVals.size());
        for (auto&& [key, val] : metadataSQLVals) {
            valuesets.push_back({ fmt::to_string(objectId), key, val });
        }
        insertMultipleRows(METADATA_TABLE, fields, valuesets);
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", objectId);
    }
}

void SQLDatabase::prepareResourceTable(std::string_view addColumnCmd)
{
    auto resourceAttributes = splitString(getInternalSetting("resource_attribute"), ',');
    bool addedAttribute = false;
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto&& resAttrib = CdsResource::getAttributeName(resAttrId);
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
            identifier(CDS_OBJECT_TABLE), identifier("resources")));
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having resources: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    res = select(
        fmt::format("SELECT COUNT(*) FROM {}", identifier(RESOURCE_TABLE)));
    int resourceRowCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having entries: {}", RESOURCE_TABLE, resourceRowCount);

    if (expectedConversionCount > 0 && resourceRowCount > 0) {
        log_info("No resources migration required");
        return true;
    }

    log_info("About to migrate resources from {} to {}", CDS_OBJECT_TABLE, RESOURCE_TABLE);

    auto resIds = select(
        fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
            identifier("id"), identifier("resources"), identifier(CDS_OBJECT_TABLE)));
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
            std::map<std::string, std::string> resourceSQLVals;
            auto&& resource = CdsResource::decode(resString);
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSQLVals[CdsResource::getAttributeName(key)] = quote(val);
            }
            auto fields = std::vector {
                identifier("item_id"),
                identifier("res_id"),
                identifier("handlerType"),
                identifier("purpose"),
            };
            auto values = std::vector {
                fmt::to_string(objectId),
                fmt::to_string(resId),
                fmt::to_string(to_underlying(resource->getHandlerType())),
                fmt::to_string(to_underlying(resource->getPurpose())),
            };
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                fields.push_back(identifier("options"));
                values.push_back(quote(dictEncode(options)));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                fields.push_back(identifier("parameters"));
                values.push_back(quote(dictEncode(parameters)));
            }
            for (auto&& [key, val] : resourceSQLVals) {
                fields.push_back(identifier(key));
                values.push_back(val);
            }
            insert(RESOURCE_TABLE, fields, values);
            resId++;
        }
    } else {
        log_debug("Skipping migration - no resources for cds object {}", objectId);
    }
}

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
#include <climits>
#include <filesystem>
#include <fmt/chrono.h>
#include <string>
#include <vector>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "config/config_setup.h"
#include "config/dynamic_content.h"
#include "content/autoscan.h"
#include "metadata/metadata_handler.h"
#include "search_handler.h"
#include "upnp_xml.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

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

/// \brief browse column ids
/// enum for createObjectFromRow's mode parameter
enum class BrowseCol {
    id = 0,
    ref_id,
    parent_id,
    object_type,
    upnp_class,
    dc_title,
    location,
    location_hash,
    auxdata,
    update_id,
    mime_type,
    flags,
    part_number,
    track_number,
    service_id,
    bookmark_pos,
    last_modified,
    last_updated,
    ref_upnp_class,
    ref_location,
    ref_auxdata,
    ref_mime_type,
    ref_service_id,
    as_persistent
};

/// \brief search column ids
enum class SearchCol {
    id = 0,
    ref_id,
    parent_id,
    object_type,
    upnp_class,
    dc_title,
    mime_type,
    part_number,
    track_number,
    location,
    last_modified,
    last_updated,
};

/// \brief meta column ids
enum class MetadataCol {
    id = 0,
    item_id,
    property_name,
    property_value
};

/// \brief resource column ids
enum class ResourceCol {
    id = 0,
    item_id,
    res_id,
    handlerType,
    options,
    parameters,
    attributes, // index of first attribute
};

/// \brief autoscan column ids
enum class AutoscanCol {
    id = 0,
    obj_id,
    persistent,
};

/// \brief autoscan column ids
enum class AutoscanColumn {
    id = 0,
    obj_id,
    scan_level,
    scan_mode,
    recursive,
    hidden,
    interval,
    last_modified,
    persistent,
    location,
    obj_location,
};

/// \brief Map browse column ids to column names
// map ensures entries are in correct order, each value of BrowseCol must be present
static const std::map<BrowseCol, std::pair<std::string, std::string>> browseColMap {
    { BrowseCol::id, { ITM_ALIAS, "id" } },
    { BrowseCol::ref_id, { ITM_ALIAS, "ref_id" } },
    { BrowseCol::parent_id, { ITM_ALIAS, "parent_id" } },
    { BrowseCol::object_type, { ITM_ALIAS, "object_type" } },
    { BrowseCol::upnp_class, { ITM_ALIAS, "upnp_class" } },
    { BrowseCol::dc_title, { ITM_ALIAS, "dc_title" } },
    { BrowseCol::location, { ITM_ALIAS, "location" } },
    { BrowseCol::location_hash, { ITM_ALIAS, "location_hash" } },
    { BrowseCol::auxdata, { ITM_ALIAS, "auxdata" } },
    { BrowseCol::update_id, { ITM_ALIAS, "update_id" } },
    { BrowseCol::mime_type, { ITM_ALIAS, "mime_type" } },
    { BrowseCol::flags, { ITM_ALIAS, "flags" } },
    { BrowseCol::part_number, { ITM_ALIAS, "part_number" } },
    { BrowseCol::track_number, { ITM_ALIAS, "track_number" } },
    { BrowseCol::service_id, { ITM_ALIAS, "service_id" } },
    { BrowseCol::bookmark_pos, { ITM_ALIAS, "bookmark_pos" } },
    { BrowseCol::last_modified, { ITM_ALIAS, "last_modified" } },
    { BrowseCol::last_updated, { ITM_ALIAS, "last_updated" } },
    { BrowseCol::ref_upnp_class, { REF_ALIAS, "upnp_class" } },
    { BrowseCol::ref_location, { REF_ALIAS, "location" } },
    { BrowseCol::ref_auxdata, { REF_ALIAS, "auxdata" } },
    { BrowseCol::ref_mime_type, { REF_ALIAS, "mime_type" } },
    { BrowseCol::ref_service_id, { REF_ALIAS, "service_id" } },
    { BrowseCol::as_persistent, { AUS_ALIAS, "persistent" } },
};

/// \brief Map search oolumn ids to column names
// map ensures entries are in correct order, each value of SearchCol must be present
static const std::map<SearchCol, std::pair<std::string, std::string>> searchColMap {
    { SearchCol::id, { SRC_ALIAS, "id" } },
    { SearchCol::ref_id, { SRC_ALIAS, "ref_id" } },
    { SearchCol::parent_id, { SRC_ALIAS, "parent_id" } },
    { SearchCol::object_type, { SRC_ALIAS, "object_type" } },
    { SearchCol::upnp_class, { SRC_ALIAS, "upnp_class" } },
    { SearchCol::dc_title, { SRC_ALIAS, "dc_title" } },
    { SearchCol::mime_type, { SRC_ALIAS, "mime_type" } },
    { SearchCol::part_number, { SRC_ALIAS, "part_number" } },
    { SearchCol::track_number, { SRC_ALIAS, "track_number" } },
    { SearchCol::location, { SRC_ALIAS, "location" } },
    { SearchCol::last_modified, { SRC_ALIAS, "last_modified" } },
    { SearchCol::last_updated, { SRC_ALIAS, "last_updated" } },
};

/// \brief Map meta column ids to column names
// map ensures entries are in correct order, each value of MetadataCol must be present
static const std::map<MetadataCol, std::pair<std::string, std::string>> metaColMap {
    { MetadataCol::id, { MTA_ALIAS, "id" } },
    { MetadataCol::item_id, { MTA_ALIAS, "item_id" } },
    { MetadataCol::property_name, { MTA_ALIAS, "property_name" } },
    { MetadataCol::property_value, { MTA_ALIAS, "property_value" } },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanCol must be present
static const std::map<AutoscanCol, std::pair<std::string, std::string>> asColMap {
    { AutoscanCol::id, { AUS_ALIAS, "id" } },
    { AutoscanCol::obj_id, { AUS_ALIAS, "obj_id" } },
    { AutoscanCol::persistent, { AUS_ALIAS, "persistent" } },
};

/// \brief Map autoscan column ids to column names
// map ensures entries are in correct order, each value of AutoscanColumn must be present
static const std::map<AutoscanColumn, std::pair<std::string, std::string>> autoscanColMap {
    { AutoscanColumn::id, { AUS_ALIAS, "id" } },
    { AutoscanColumn::obj_id, { AUS_ALIAS, "obj_id" } },
    { AutoscanColumn::scan_level, { AUS_ALIAS, "scan_level" } },
    { AutoscanColumn::scan_mode, { AUS_ALIAS, "scan_mode" } },
    { AutoscanColumn::recursive, { AUS_ALIAS, "recursive" } },
    { AutoscanColumn::hidden, { AUS_ALIAS, "hidden" } },
    { AutoscanColumn::interval, { AUS_ALIAS, "interval" } },
    { AutoscanColumn::last_modified, { AUS_ALIAS, "last_modified" } },
    { AutoscanColumn::persistent, { AUS_ALIAS, "persistent" } },
    { AutoscanColumn::location, { AUS_ALIAS, "location" } },
    { AutoscanColumn::obj_location, { ITM_ALIAS, "location" } },
};

/// \brief Map browse sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, BrowseCol>> browseSortMap {
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), BrowseCol::part_number },
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), BrowseCol::track_number },
    { MetadataHandler::getMetaFieldName(M_TITLE), BrowseCol::dc_title },
    { UPNP_SEARCH_CLASS, BrowseCol::upnp_class },
    { UPNP_SEARCH_PATH, BrowseCol::location },
    { UPNP_SEARCH_REFID, BrowseCol::ref_id },
    { UPNP_SEARCH_PARENTID, BrowseCol::parent_id },
    { UPNP_SEARCH_ID, BrowseCol::id },
    { UPNP_SEARCH_LAST_UPDATED, BrowseCol::last_updated },
    { UPNP_SEARCH_LAST_MODIFIED, BrowseCol::last_modified },
};

/// \brief Map search sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, SearchCol>> searchSortMap {
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), SearchCol::part_number },
    { MetadataHandler::getMetaFieldName(M_TRACKNUMBER), SearchCol::track_number },
    { MetadataHandler::getMetaFieldName(M_TITLE), SearchCol::dc_title },
    { UPNP_SEARCH_CLASS, SearchCol::upnp_class },
    { UPNP_SEARCH_PATH, SearchCol::location },
    { UPNP_SEARCH_REFID, SearchCol::ref_id },
    { UPNP_SEARCH_PARENTID, SearchCol::parent_id },
    { UPNP_SEARCH_ID, SearchCol::id },
    { UPNP_SEARCH_LAST_UPDATED, SearchCol::last_updated },
    { UPNP_SEARCH_LAST_MODIFIED, SearchCol::last_modified },
};

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, MetadataCol>> metaTagMap {
    { "id", MetadataCol::id },
    { UPNP_SEARCH_ID, MetadataCol::item_id },
    { META_NAME, MetadataCol::property_name },
    { META_VALUE, MetadataCol::property_value },
};

/// \brief Map meta search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, AutoscanCol>> asTagMap {
    { "id", AutoscanCol::id },
    { "obj_id", AutoscanCol::obj_id },
    { "persistent", AutoscanCol::persistent },
};

/// \brief Autoscan search keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, AutoscanColumn>> autoscanTagMap {
    { "id", AutoscanColumn::id },
    { "obj_id", AutoscanColumn::obj_id },
    { "scan_level", AutoscanColumn::scan_level },
    { "scan_mode", AutoscanColumn::scan_mode },
    { "recursive", AutoscanColumn::recursive },
    { "hidden", AutoscanColumn::hidden },
    { "interval", AutoscanColumn::interval },
    { "last_modified", AutoscanColumn::last_modified },
    { "persistent", AutoscanColumn::persistent },
    { "location", AutoscanColumn::location },
    { "obj_location", AutoscanColumn::obj_location },
};

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return std::underlying_type_t<E>(e);
}

#define getCol(rw, idx) (rw)->col(to_underlying((idx)))

static std::shared_ptr<EnumColumnMapper<BrowseCol>> browseColumnMapper;
static std::shared_ptr<EnumColumnMapper<SearchCol>> searchColumnMapper;
static std::shared_ptr<EnumColumnMapper<MetadataCol>> metaColumnMapper;
static std::shared_ptr<EnumColumnMapper<AutoscanCol>> asColumnMapper;
static std::shared_ptr<EnumColumnMapper<AutoscanColumn>> autoscanColumnMapper;
static std::shared_ptr<EnumColumnMapper<int>> resourceColumnMapper;

SQLDatabase::SQLDatabase(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : Database(std::move(config))
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
        { "id", to_underlying(ResourceCol::id) },
        { UPNP_SEARCH_ID, to_underlying(ResourceCol::item_id) },
        { "res@id", to_underlying(ResourceCol::res_id) },
    };
    /// \brief Map resource column ids to column names
    // map ensures entries are in correct order, each value of ResourceCol must be present
    std::map<int, std::pair<std::string, std::string>> resourceColMap {
        { to_underlying(ResourceCol::id), { RES_ALIAS, "id" } },
        { to_underlying(ResourceCol::item_id), { RES_ALIAS, "item_id" } },
        { to_underlying(ResourceCol::res_id), { RES_ALIAS, "res_id" } },
        { to_underlying(ResourceCol::handlerType), { RES_ALIAS, "handlerType" } },
        { to_underlying(ResourceCol::options), { RES_ALIAS, "options" } },
        { to_underlying(ResourceCol::parameters), { RES_ALIAS, "parameters" } },
    };
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = MetadataHandler::getResAttrName(resAttrId);
        resourceTagMap.emplace_back(fmt::format("res@{}", attrName), to_underlying(ResourceCol::attributes) + resAttrId);
        resourceColMap[to_underlying(ResourceCol::attributes) + resAttrId] = std::pair(RES_ALIAS, attrName);
    }

    browseColumnMapper = std::make_shared<EnumColumnMapper<BrowseCol>>(table_quote_begin, table_quote_end, ITM_ALIAS, CDS_OBJECT_TABLE, browseSortMap, browseColMap);
    searchColumnMapper = std::make_shared<EnumColumnMapper<SearchCol>>(table_quote_begin, table_quote_end, SRC_ALIAS, CDS_OBJECT_TABLE, searchSortMap, searchColMap);
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
            identifier(CDS_OBJECT_TABLE), identifier(REF_ALIAS), browseColumnMapper->mapQuoted(BrowseCol::ref_id), identifier(browseColMap.at(BrowseCol::id).second));
        auto join2 = fmt::format("LEFT JOIN {} ON {} = {}", asColumnMapper->tableQuoted(), asColumnMapper->mapQuoted(AutoscanCol::obj_id), browseColumnMapper->mapQuoted(BrowseCol::id));
        this->sql_browse_query = fmt::format("SELECT {} FROM {} {} {} ", fmt::join(buf, ", "), browseColumnMapper->tableQuoted(), join1, join2);
    }
    // Statement for UPnP search
    {
        std::vector<std::string> colBuf;
        colBuf.reserve(searchColMap.size());
        for (auto&& [key, col] : searchColMap) {
            colBuf.push_back(fmt::format("{}.{}", identifier(col.first), identifier(col.second)));
        }
        this->sql_search_columns = fmt::format("{}", fmt::join(colBuf, ", "));

        auto join1 = fmt::format("INNER JOIN {} ON {} = {}", metaColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), metaColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        auto join2 = fmt::format("INNER JOIN {} ON {} = {}", resourceColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        this->sql_search_query = fmt::format("{} {} {}", searchColumnMapper->tableQuoted(), join1, join2);
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
        auto join = fmt::format("LEFT JOIN {} ON {} = {}", browseColumnMapper->tableQuoted(), autoscanColumnMapper->mapQuoted(AutoscanColumn::obj_id), browseColumnMapper->mapQuoted(BrowseCol::id));
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

void SQLDatabase::upgradeDatabase(std::string&& dbVersion, const std::array<unsigned int, DBVERSION>& hashies, config_option_t upgradeOption, const std::string& updateVersionCommand, const std::string& addResourceColumnCmd)
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

    size_t version = 1;
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        const pugi::xml_node& versionNode = versionElement.node();
        std::vector<std::pair<std::string, std::string>> versionCmds;
        auto&& myHash = stringHash(UpnpXMLBuilder::printXml(versionNode));
        if (version < DBVERSION && myHash == hashies.at(version)) {
            for (auto&& scriptNode : versionNode.children("script")) {
                std::string migration = trimString(scriptNode.attribute("migration").as_string());
                versionCmds.emplace_back(migration, trimString(scriptNode.text().as_string()));
            }
        } else {
            log_error("Wrong hash for version {}: {} != {}", version + 1, myHash, hashies.at(version));
            throw_std_runtime_error("Wrong hash for version {}", version + 1);
        }
        dbUpdates.push_back(std::move(versionCmds));
        version++;
    }

    version = 1;
    static const std::map<std::string, bool (SQLDatabase::*)()> migActions {
        { "metadata", &SQLDatabase::doMetadataMigration },
        { "resources", &SQLDatabase::doResourceMigration },
    };
    this->addResourceColumnCmd = addResourceColumnCmd;
    /* --- run database upgrades --- */
    for (auto&& upgrade : dbUpdates) {
        if (dbVersion == fmt::to_string(version)) {
            log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
            for (auto&& [migrationCmd, upgradeCmd] : upgrade) {
                bool actionResult = true;
                if (!migrationCmd.empty() && migActions.find(migrationCmd) != migActions.end())
                    actionResult = (*this.*(migActions.at(migrationCmd)))();
                if (actionResult && !upgradeCmd.empty())
                    _exec(upgradeCmd);
            }
            _exec(fmt::format(updateVersionCommand, version + 1, version));
            dbVersion = fmt::to_string(version + 1);
            log_info("Database upgrade to version {} successful.", dbVersion.c_str());
        }
        version++;
    }

    if (version != DBVERSION || dbVersion != fmt::to_string(DBVERSION))
        throw_std_runtime_error("The database seems to be from a newer version");

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
    return fmt::format("{}", fmt::join(sortKeys, ","));
}

std::string SQLDatabase::getSearchCapabilities()
{
    auto searchKeys = std::vector {
        MetadataHandler::getMetaFieldName(M_TITLE),
        std::string(UPNP_SEARCH_CLASS),
        MetadataHandler::getMetaFieldName(M_ARTIST),
        MetadataHandler::getMetaFieldName(M_ALBUM),
        MetadataHandler::getMetaFieldName(M_GENRE),
    };
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = MetadataHandler::getResAttrName(resAttrId);
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
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("illegal refID was set");
        }
    }

    // This should never happen - but fail softly
    // It means that something doesn't set the refID correctly
    log_warning("Failed to loadObject with refid: {}", refID);

    return findObjectByPath(location);
}

std::vector<std::shared_ptr<SQLDatabase::AddUpdateTable>> SQLDatabase::_addUpdateObject(const std::shared_ptr<CdsObject>& obj, Operation op, int* changedContainer)
{
    std::shared_ptr<CdsObject> refObj;
    bool hasReference = false;
    bool playlistRef = obj->getFlag(OBJECT_FLAG_PLAYLIST_REF);
    if (playlistRef) {
        if (obj->isPureItem())
            throw_std_runtime_error("Tried to add pure item with PLAYLIST_REF flag set");
        if (obj->getRefID() <= 0)
            throw_std_runtime_error("PLAYLIST_REF flag set but refId is <=0");
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
    cdsObjectSql["object_type"] = quote(obj->getObjectType());

    if (hasReference || playlistRef)
        cdsObjectSql["ref_id"] = quote(refObj->getID());
    else if (op == Operation::Update)
        cdsObjectSql["ref_id"] = SQL_NULL;

    if (!hasReference || refObj->getClass() != obj->getClass())
        cdsObjectSql["upnp_class"] = quote(obj->getClass());
    else if (op == Operation::Update)
        cdsObjectSql["upnp_class"] = SQL_NULL;

    //if (!hasReference || refObj->getTitle() != obj->getTitle())
    cdsObjectSql["dc_title"] = quote(obj->getTitle());
    //else if (isUpdate)
    //    cdsObjectSql["dc_title"] = SQL_NULL;

    if (op == Operation::Update)
        cdsObjectSql["auxdata"] = SQL_NULL;

    auto auxData = obj->getAuxData();
    if (!auxData.empty() && (!hasReference || auxData != refObj->getAuxData())) {
        cdsObjectSql["auxdata"] = quote(dictEncode(auxData));
    }

    bool useResourceRef = obj->getFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->clearFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    cdsObjectSql["flags"] = quote(obj->getFlags());

    if (obj->getMTime() > std::chrono::seconds::zero()) {
        cdsObjectSql["last_modified"] = quote(obj->getMTime().count());
    } else {
        cdsObjectSql["last_modified"] = SQL_NULL;
    }
    cdsObjectSql["last_updated"] = quote(to_seconds(std::chrono::system_clock::now()).count());

    if (obj->isContainer() && op == Operation::Update && obj->isVirtual()) {
        fs::path dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, obj->getLocation());
        cdsObjectSql["location"] = quote(dbLocation);
        cdsObjectSql["location_hash"] = quote(stringHash(dbLocation.string()));
    }

    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (!hasReference) {
            fs::path loc = item->getLocation();
            if (loc.empty())
                throw_std_runtime_error("tried to create or update a non-referenced item without a location set");
            if (obj->isPureItem()) {
                int parentID = ensurePathExistence(loc.parent_path(), changedContainer);
                obj->setParentID(parentID);
                fs::path dbLocation = addLocationPrefix(LOC_FILE_PREFIX, loc);
                cdsObjectSql["location"] = quote(dbLocation);
                cdsObjectSql["location_hash"] = quote(stringHash(dbLocation.string()));
            } else {
                // URLs
                cdsObjectSql["location"] = quote(loc);
                cdsObjectSql["location_hash"] = SQL_NULL;
            }
        } else {
            if (op == Operation::Update) {
                cdsObjectSql["location"] = SQL_NULL;
                cdsObjectSql["location_hash"] = SQL_NULL;
            }
        }

        if (item->getTrackNumber() > 0) {
            cdsObjectSql["track_number"] = quote(item->getTrackNumber());
        } else {
            if (op == Operation::Update)
                cdsObjectSql["track_number"] = SQL_NULL;
        }

        cdsObjectSql["bookmark_pos"] = quote(item->getBookMarkPos().count());

        if (item->getPartNumber() > 0) {
            cdsObjectSql["part_number"] = quote(item->getPartNumber());
        } else {
            if (op == Operation::Update)
                cdsObjectSql["part_number"] = SQL_NULL;
        }

        if (!item->getServiceID().empty()) {
            if (!hasReference || std::static_pointer_cast<CdsItem>(refObj)->getServiceID() != item->getServiceID())
                cdsObjectSql["service_id"] = quote(item->getServiceID());
            else
                cdsObjectSql["service_id"] = SQL_NULL;
        } else {
            if (op == Operation::Update)
                cdsObjectSql["service_id"] = SQL_NULL;
        }

        cdsObjectSql["mime_type"] = quote(item->getMimeType().substr(0, 40));
    }

    // check for a duplicate (virtual) object
    if (hasReference && op != Operation::Update) {
        auto where = std::vector {
            fmt::format("{}={}", identifier("parent_id"), quote(obj->getParentID())),
            fmt::format("{}={}", identifier("ref_id"), quote(refObj->getID())),
            fmt::format("{}={}", identifier("dc_title"), quote(obj->getTitle())),
        };
        auto res = select(fmt::format("SELECT {} FROM {} WHERE {} LIMIT 1",
            identifier("id"), identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND ")));
        // if duplicate items is found - ignore
        if (res && (res->nextRow()))
            return {};
    }

    if (obj->getParentID() == INVALID_OBJECT_ID) {
        throw_std_runtime_error("Tried to create or update an object {} with an illegal parent id {}", obj->getLocation().c_str(), obj->getParentID());
    }

    cdsObjectSql["parent_id"] = fmt::to_string(obj->getParentID());

    auto returnVal = std::vector {
        std::make_shared<AddUpdateTable>(CDS_OBJECT_TABLE, cdsObjectSql, op),
    };

    if (!hasReference || obj->getMetadata() != refObj->getMetadata()) {
        generateMetadataDBOperations(obj, op, returnVal);
    }

    if (!hasReference || (!useResourceRef && !refObj->resourcesEqual(obj))) {
        generateResourceDBOperations(obj, op, returnVal);
    }

    return returnVal;
}

void SQLDatabase::addObject(std::shared_ptr<CdsObject> obj, int* changedContainer)
{
    if (obj->getID() != INVALID_OBJECT_ID)
        throw_std_runtime_error("Tried to add an object with an object ID set");

    auto tables = _addUpdateObject(obj, Operation::Insert, changedContainer);

    beginTransaction("addObject");
    for (auto&& addUpdateTable : tables) {
        auto qb = sqlForInsert(obj, addUpdateTable);
        log_debug("Generated insert: {}", qb);

        if (addUpdateTable->getTableName() == CDS_OBJECT_TABLE) {
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
    std::vector<std::shared_ptr<AddUpdateTable>> data;
    if (obj->getID() == CDS_ID_FS_ROOT) {
        std::map<std::string, std::string> cdsObjectSql;

        cdsObjectSql["dc_title"] = quote(obj->getTitle());
        setFsRootName(obj->getTitle());
        cdsObjectSql["upnp_class"] = quote(obj->getClass());

        data.push_back(std::make_shared<AddUpdateTable>(CDS_OBJECT_TABLE, cdsObjectSql, Operation::Update));
    } else {
        if (IS_FORBIDDEN_CDS_ID(obj->getID()))
            throw_std_runtime_error("Tried to update an object with a forbidden ID ({})", obj->getID());
        data = _addUpdateObject(obj, Operation::Update, changedContainer);
    }

    beginTransaction("updateObject");
    for (auto&& addUpdateTable : data) {
        auto qb = [this, &obj, &addUpdateTable]() {
            Operation op = addUpdateTable->getOperation();
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
    auto loadSql = fmt::format("{} WHERE {} = {}", sql_browse_query, browseColumnMapper->mapQuoted(BrowseCol::id), objectID);
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(row);
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
    auto loadSql = fmt::format("{} WHERE {} = {}", sql_browse_query, browseColumnMapper->mapQuoted(BrowseCol::service_id), quote(serviceID));
    beginTransaction("loadObjectByServiceID");
    auto res = select(loadSql);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(row);
            commit("loadObjectByServiceID");
            return result;
        }
    }
    commit("loadObjectByServiceID");

    return nullptr;
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
        objectIDs.push_back(row->col_int(0));
    }

    return objectIDs;
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::browse(const std::unique_ptr<BrowseParam>& param)
{
    auto parent = param->getObject();

    if (dynamicContainers.find(parent->getID()) != dynamicContainers.end()) {
        auto dynConfig = config->getDynamicContentListOption(CFG_SERVER_DYNAMIC_CONTENT_LIST)->get(parent->getLocation());
        if (dynConfig) {
            auto srcParam = std::make_unique<SearchParam>(fmt::to_string(parent->getParentID()), dynConfig->getFilter(), dynConfig->getSort(), // get params from config
                param->getStartingIndex(), param->getRequestedCount() == 0 ? 1000 : param->getRequestedCount()); // get params from browse
            int numMatches = 0;
            auto result = this->search(srcParam, &numMatches);
            param->setTotalMatches(numMatches);
            return result;
        }
        log_warning("Dynamic content {} error '{}'", parent->getID(), parent->getLocation().string());
    }

    bool getContainers = param->getFlag(BROWSE_CONTAINERS);
    bool getItems = param->getFlag(BROWSE_ITEMS);
    bool hideFsRoot = param->getFlag(BROWSE_HIDE_FS_ROOT);
    int childCount = 1;
    if (param->getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        childCount = getChildCount(parent->getID(), getContainers, getItems, hideFsRoot);
        param->setTotalMatches(childCount);
    } else {
        param->setTotalMatches(1);
    }

    std::vector<std::string> where;
    std::string orderBy;
    std::string limit;

    if (param->getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        int count = param->getRequestedCount();
        bool doLimit = true;
        if (!count) {
            if (param->getStartingIndex())
                count = std::numeric_limits<int>::max();
            else
                doLimit = false;
        }

        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::parent_id), parent->getID()));

        if (parent->getID() == CDS_ID_ROOT && hideFsRoot)
            where.push_back(fmt::format("{} != {}", browseColumnMapper->mapQuoted(BrowseCol::id), quote(CDS_ID_FS_ROOT)));

        // order by code..
        auto orderByCode = [&]() {
            std::string orderQb;
            if (param->getFlag(BROWSE_TRACK_SORT)) {
                orderQb = fmt::format("{},{}", browseColumnMapper->mapQuoted(BrowseCol::part_number), browseColumnMapper->mapQuoted(BrowseCol::track_number));
            } else {
                SortParser sortParser(browseColumnMapper, param->getSortCriteria());
                orderQb = sortParser.parse();
            }
            if (orderQb.empty()) {
                orderQb = browseColumnMapper->mapQuoted(BrowseCol::dc_title);
            }
            return orderQb;
        };

        if (!getContainers && !getItems) {
            auto zero = std::string("0 = 1");
            where.push_back(std::move(zero));
        } else if (getContainers && !getItems) {
            where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::object_type), quote(OBJECT_TYPE_CONTAINER)));
            orderBy = fmt::format(" ORDER BY {}", orderByCode());
        } else if (!getContainers && getItems) {
            where.push_back(fmt::format("({0} & {1}) = {1}", browseColumnMapper->mapQuoted(BrowseCol::object_type), quote(OBJECT_TYPE_ITEM)));
            orderBy = fmt::format(" ORDER BY {}", orderByCode());
        } else {
            orderBy = fmt::format(" ORDER BY ({} = {}) DESC, {}", browseColumnMapper->mapQuoted(BrowseCol::object_type), quote(OBJECT_TYPE_CONTAINER), orderByCode());
        }
        if (doLimit)
            limit = fmt::format(" LIMIT {} OFFSET {}", count, param->getStartingIndex());
    } else { // metadata
        where.push_back(fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::id), parent->getID()));
        limit = " LIMIT 1";
    }
    auto qb = fmt::format("{} WHERE {}{}{}", sql_browse_query, fmt::join(where, " AND "), orderBy, limit);
    log_debug("QUERY: {}", qb);
    beginTransaction("browse");
    std::shared_ptr<SQLResult> sqlResult = select(qb);
    commit("browse");

    std::vector<std::shared_ptr<CdsObject>> result;
    result.reserve(sqlResult->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        result.push_back(createObjectFromRow(row));
    }

    // update childCount fields
    for (auto&& obj : result) {
        if (obj->isContainer()) {
            auto cont = std::static_pointer_cast<CdsContainer>(obj);
            cont->setChildCount(getChildCount(cont->getID(), getContainers, getItems, hideFsRoot));
        }
    }

    if (config->getBoolOption(CFG_SERVER_DYNAMIC_CONTENT_LIST_ENABLED) && getContainers && param->getStartingIndex() == 0 && param->getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        auto dynContent = config->getDynamicContentListOption(CFG_SERVER_DYNAMIC_CONTENT_LIST);
        if (dynamicContainers.size() < dynContent->size()) {
            for (size_t count = 0; count < dynContent->size(); count++) {
                auto dynConfig = dynContent->get(count);
                if (parent->getLocation() == dynConfig->getLocation().parent_path() || (parent->getLocation().empty() && dynConfig->getLocation().parent_path() == "/")) {
                    int dynId = -(parent->getID() + 1) * 10000 - count;
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
                            auto resource = std::make_shared<CdsResource>(CH_CONTAINERART);
                            std::string type = image.extension().string().substr(1);
                            std::string mimeType = mime->getMimeType(image, fmt::format("image/{}", type));
                            resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
                            resource->addAttribute(R_RESOURCE_FILE, image.string());
                            resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
                            dynFolder->addResource(std::move(resource));
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
        param->setTotalMatches(childCount);
    }
    return result;
}

std::vector<std::shared_ptr<CdsObject>> SQLDatabase::search(const std::unique_ptr<SearchParam>& param, int* numMatches)
{
    auto searchParser = std::make_unique<SearchParser>(*sqlEmitter, param->searchCriteria());
    std::shared_ptr<ASTNode> rootNode = searchParser->parse();
    std::string searchSQL(rootNode->emitSQL());
    if (searchSQL.empty())
        throw_std_runtime_error("failed to generate SQL for search");

    beginTransaction("search");
    auto countSQL = fmt::format("SELECT COUNT(*) FROM {} WHERE {}", sql_search_query, searchSQL);
    log_debug("Search count resolves to SQL [{}]", countSQL);
    auto sqlResult = select(countSQL);
    commit("search");

    auto countRow = sqlResult->nextRow();
    if (countRow) {
        *numMatches = countRow->col_int(0);
    }

    // order by code..
    auto orderByCode = [&]() {
        SortParser sortParser(searchColumnMapper, param->getSortCriteria());
        auto orderQb = sortParser.parse();
        if (orderQb.empty()) {
            orderQb = searchColumnMapper->mapQuoted(SearchCol::id);
        }
        return orderQb;
    };

    auto orderBy = fmt::format(" ORDER BY {}", orderByCode());
    std::string limit;

    size_t startingIndex = param->getStartingIndex();
    size_t requestedCount = param->getRequestedCount();
    if (startingIndex > 0 || requestedCount > 0) {
        limit = fmt::format(" LIMIT {} OFFSET {}", (requestedCount == 0 ? 10000000000 : requestedCount), startingIndex);
    }

    auto retrievalSQL = fmt::format("SELECT DISTINCT {} FROM {} WHERE {}{}{}", sql_search_columns, sql_search_query, searchSQL, orderBy, limit);

    log_debug("Search resolves to SQL [{}]", retrievalSQL);
    beginTransaction("search 2");
    sqlResult = select(retrievalSQL);
    commit("search 2");

    std::vector<std::shared_ptr<CdsObject>> result;
    result.reserve(sqlResult->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        result.push_back(createObjectFromSearchRow(row));
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
        where.push_back(fmt::format("{} != {}", identifier("id"), quote(CDS_ID_FS_ROOT)));
    }
    beginTransaction("getChildCount");
    auto res = select(fmt::format("SELECT COUNT(*) FROM {} WHERE {}", identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND ")));
    commit("getChildCount");

    if (res) {
        auto row = res->nextRow();
        if (row) {
            return row->col_int(0);
        }
    }
    return 0;
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
        if (isRegularFile(fullpath, ec) || wasRegularFile)
            return addLocationPrefix(LOC_FILE_PREFIX, fullpath);
        return addLocationPrefix(LOC_DIR_PREFIX, fullpath);
    }();

    auto where = std::vector {
        fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::location_hash), quote(stringHash(dbLocation))),
        fmt::format("{} = {}", browseColumnMapper->mapQuoted(BrowseCol::location), quote(dbLocation)),
        fmt::format("{} IS NULL", browseColumnMapper->mapQuoted(BrowseCol::ref_id)),
    };
    auto findSql = fmt::format("{} WHERE {} LIMIT 1", sql_browse_query, fmt::join(where, " AND "));

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
    auto result = createObjectFromRow(row);
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

    return createContainer(parentID, f2i->convert(path.filename()), path, false, "", INVALID_OBJECT_ID, std::map<std::string, std::string>());
}

int SQLDatabase::createContainer(int parentID, std::string name, const std::string& virtualPath, bool isVirtual, const std::string& upnpClass, int refID, const std::map<std::string, std::string>& itemMetadata)
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
        identifier("upnp_class"),
        identifier("dc_title"),
        identifier("location"),
        identifier("location_hash"),
        identifier("ref_id"),
    };
    auto values = std::vector {
        fmt::format("{}", parentID),
        fmt::format("{}", OBJECT_TYPE_CONTAINER),
        fmt::format("{}", !upnpClass.empty() ? quote(upnpClass) : quote(UPNP_CLASS_CONTAINER)),
        quote(std::move(name)),
        quote(dbLocation),
        quote(stringHash(dbLocation)),
        (refID > 0) ? fmt::format("{}", refID) : fmt::format("{}", SQL_NULL),
    };

    beginTransaction("createContainer");
    int newId = insert(CDS_OBJECT_TABLE, fields, values, true); // true = get last id#
    log_debug("Created object row, id: {}", newId);

    if (!itemMetadata.empty()) {
        for (auto&& [key, val] : itemMetadata) {
            auto mfields = std::vector {
                identifier("item_id"),
                identifier("property_name"),
                identifier("property_value"),
            };
            auto mvalues = std::vector {
                fmt::format("{}", newId),
                quote(key),
                quote(val),
            };
            insert(METADATA_TABLE, mfields, mvalues);
        }
        log_debug("Wrote metadata for cds_object {}", newId);
    }
    commit("createContainer");

    return newId;
}

int SQLDatabase::insert(const char* tableName, const std::vector<SQLIdentifier>& fields, const std::vector<std::string>& values, bool getLastInsertId)
{
    auto sql = fmt::format("INSERT INTO {} ({}) VALUES ({})", identifier(tableName), fmt::join(fields, ","), fmt::join(values, ","));
    return exec(sql, getLastInsertId);
}

fs::path SQLDatabase::buildContainerPath(int parentID, const std::string& title)
{
    if (parentID == CDS_ID_ROOT)
        return fmt::format("{}{}", VIRTUAL_CONTAINER_SEPARATOR, title);

    beginTransaction("buildContainerPath");
    auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} = {3} LIMIT 1",
        identifier("location"), identifier(CDS_OBJECT_TABLE), identifier("id"), parentID));
    commit("buildContainerPath");
    if (!res)
        return {};

    auto row = res->nextRow();
    if (!row)
        return {};

    char prefix;
    auto path = stripLocationPrefix(fmt::format("{}{}{}", row->col(0), VIRTUAL_CONTAINER_SEPARATOR, title), &prefix);
    if (prefix != LOC_VIRT_PREFIX)
        throw_std_runtime_error("Tried to build a virtual container path with an non-virtual parentID");

    return path;
}

void SQLDatabase::addContainerChain(std::string virtualPath, const std::string& lastClass, int lastRefID, int* containerID, std::deque<int>& updateID, const std::map<std::string, std::string>& lastMetadata)
{
    log_debug("Adding container Chain for path: {}, lastRefId: {}, containerId: {}", virtualPath.c_str(), lastRefID, *containerID);

    reduceString(virtualPath, VIRTUAL_CONTAINER_SEPARATOR);
    if (virtualPath == std::string(1, VIRTUAL_CONTAINER_SEPARATOR)) {
        *containerID = CDS_ID_ROOT;
        return;
    }
    std::string dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, virtualPath);

    beginTransaction("addContainerChain");
    auto res = select(fmt::format("SELECT {0}id{1} FROM {0}{2}{1} WHERE {0}location_hash{1} = {3} AND {0}location{1} = {4} LIMIT 1", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, quote(stringHash(dbLocation)), quote(dbLocation)));
    if (res) {
        auto row = res->nextRow();
        if (row) {
            if (containerID)
                *containerID = row->col_int(0);
            commit("addContainerChain");
            return;
        }
    }
    commit("addContainerChain");

    int parentContainerID = 0;
    std::string newpath, container, newClass;
    stripAndUnescapeVirtualContainerFromPath(virtualPath, newpath, container);
    auto classes = splitString(lastClass, '/');
    if (!classes.empty()) {
        newClass = classes.back();
        classes.pop_back();
    }
    addContainerChain(newpath, classes.empty() ? "" : fmt::format("{}", fmt::join(classes, "/")), INVALID_OBJECT_ID, &parentContainerID, updateID, std::map<std::string, std::string>());

    *containerID = createContainer(parentContainerID, container, virtualPath, true, newClass, lastRefID, lastMetadata);
    updateID.push_front(*containerID);
}

std::string SQLDatabase::addLocationPrefix(char prefix, const fs::path& path)
{
    return fmt::format("{}{}", prefix, path.string().c_str());
}

fs::path SQLDatabase::stripLocationPrefix(std::string_view dbLocation, char* prefix)
{
    if (dbLocation.empty()) {
        if (prefix)
            *prefix = LOC_ILLEGAL_PREFIX;
        return {};
    }
    if (prefix)
        *prefix = dbLocation.at(0);
    return dbLocation.substr(1);
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromRow(const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, BrowseCol::object_type));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, BrowseCol::id)));
    obj->setRefID(stoiString(getCol(row, BrowseCol::ref_id)));

    obj->setParentID(std::stoi(getCol(row, BrowseCol::parent_id)));
    obj->setTitle(getCol(row, BrowseCol::dc_title));
    obj->setClass(fallbackString(getCol(row, BrowseCol::upnp_class), getCol(row, BrowseCol::ref_upnp_class)));
    obj->setFlags(std::stoi(getCol(row, BrowseCol::flags)));
    obj->setMTime(std::chrono::seconds(stoulString(getCol(row, BrowseCol::last_modified))));
    obj->setUTime(std::chrono::seconds(stoulString(getCol(row, BrowseCol::last_updated))));

    auto meta = retrieveMetadataForObject(obj->getID());
    if (!meta.empty()) {
        obj->setMetadata(meta);
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        meta = retrieveMetadataForObject(obj->getRefID());
        if (!meta.empty())
            obj->setMetadata(meta);
    }

    std::string auxdataStr = fallbackString(getCol(row, BrowseCol::auxdata), getCol(row, BrowseCol::ref_auxdata));
    std::map<std::string, std::string> aux;
    dictDecode(auxdataStr, &aux);
    obj->setAuxData(aux);

    auto resources = retrieveResourcesForObject(obj->getID());
    if (!resources.empty()) {
        obj->setResources(resources);
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        resources = retrieveResourcesForObject(obj->getRefID());
        if (!resources.empty())
            obj->setResources(resources);
    }
    auto resource_zero_ok = !resources.empty();

    obj->setVirtual((obj->getRefID() && obj->isPureItem()) || (obj->isItem() && !obj->isPureItem())); // gets set to true for virtual containers below

    int matched_types = 0;

    if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        cont->setUpdateID(std::stoi(getCol(row, BrowseCol::update_id)));
        char locationPrefix;
        cont->setLocation(stripLocationPrefix(getCol(row, BrowseCol::location), &locationPrefix));
        if (locationPrefix == LOC_VIRT_PREFIX)
            cont->setVirtual(true);

        std::string autoscanPersistent = getCol(row, BrowseCol::as_persistent);
        if (!autoscanPersistent.empty()) {
            if (remapBool(autoscanPersistent))
                cont->setAutoscanType(OBJECT_AUTOSCAN_CFG);
            else
                cont->setAutoscanType(OBJECT_AUTOSCAN_UI);
        } else
            cont->setAutoscanType(OBJECT_AUTOSCAN_NONE);
        matched_types++;
    }

    if (obj->isItem()) {
        if (!resource_zero_ok)
            throw_std_runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(fallbackString(getCol(row, BrowseCol::mime_type), getCol(row, BrowseCol::ref_mime_type)));
        if (obj->isPureItem()) {
            if (!obj->isVirtual())
                item->setLocation(stripLocationPrefix(getCol(row, BrowseCol::location)));
            else
                item->setLocation(stripLocationPrefix(getCol(row, BrowseCol::ref_location)));
        } else // URLs
        {
            item->setLocation(fallbackString(getCol(row, BrowseCol::location), getCol(row, BrowseCol::ref_location)));
        }

        item->setTrackNumber(stoiString(getCol(row, BrowseCol::track_number)));
        item->setBookMarkPos(std::chrono::milliseconds(stoulString(getCol(row, BrowseCol::bookmark_pos))));
        item->setPartNumber(stoiString(getCol(row, BrowseCol::part_number)));

        if (!getCol(row, BrowseCol::ref_service_id).empty())
            item->setServiceID(getCol(row, BrowseCol::ref_service_id));
        else
            item->setServiceID(getCol(row, BrowseCol::service_id));

        matched_types++;
    }

    if (!matched_types) {
        throw DatabaseException("", fmt::format("Unknown object type: {}", objectType));
    }

    return obj;
}

std::shared_ptr<CdsObject> SQLDatabase::createObjectFromSearchRow(const std::unique_ptr<SQLRow>& row)
{
    int objectType = std::stoi(getCol(row, SearchCol::object_type));
    auto obj = CdsObject::createObject(objectType);

    /* set common properties */
    obj->setID(std::stoi(getCol(row, SearchCol::id)));
    obj->setRefID(stoiString(getCol(row, SearchCol::ref_id)));

    obj->setParentID(std::stoi(getCol(row, SearchCol::parent_id)));
    obj->setTitle(getCol(row, SearchCol::dc_title));
    obj->setClass(getCol(row, SearchCol::upnp_class));

    auto meta = retrieveMetadataForObject(obj->getID());
    if (!meta.empty())
        obj->setMetadata(meta);

    auto resources = retrieveResourcesForObject(obj->getID());
    if (!resources.empty()) {
        obj->setResources(resources);
    } else if (obj->getRefID() != CDS_ID_ROOT) {
        resources = retrieveResourcesForObject(obj->getRefID());
        if (!resources.empty())
            obj->setResources(resources);
    }

    auto resource_zero_ok = !resources.empty();

    if (obj->isItem()) {
        if (!resource_zero_ok)
            throw_std_runtime_error("tried to create object without at least one resource");

        auto item = std::static_pointer_cast<CdsItem>(obj);
        item->setMimeType(getCol(row, SearchCol::mime_type));
        if (obj->isPureItem()) {
            item->setLocation(stripLocationPrefix(getCol(row, SearchCol::location)));
        } else { // URLs
            item->setLocation(getCol(row, SearchCol::location));
        }

        item->setPartNumber(stoiString(getCol(row, SearchCol::part_number)));
        item->setTrackNumber(stoiString(getCol(row, SearchCol::track_number)));
    } else if (!obj->isContainer()) {
        throw DatabaseException("", fmt::format("Unknown object type: {}", objectType));
    }

    return obj;
}

std::map<std::string, std::string> SQLDatabase::retrieveMetadataForObject(int objectId)
{
    auto query = fmt::format("{} FROM {} WHERE {} = {}",
        sql_meta_query, identifier(METADATA_TABLE), identifier("item_id"), objectId);
    auto res = select(query);
    if (!res)
        return {};

    std::map<std::string, std::string> metadata;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        metadata[getCol(row, MetadataCol::property_name)] = getCol(row, MetadataCol::property_value);
    }
    return metadata;
}

int SQLDatabase::getTotalFiles(bool isVirtual, const std::string& mimeType, const std::string& upnpClass)
{
    auto where = std::vector {
        fmt::format("{} != {}", identifier("object_type"), quote(OBJECT_TYPE_CONTAINER)),
    };
    if (!mimeType.empty()) {
        where.push_back(fmt::format("{} LIKE {}", identifier("mime_type"), quote(fmt::format("{}%", mimeType))));
    }
    if (!upnpClass.empty()) {
        where.push_back(fmt::format("{} LIKE {}", identifier("upnp_class"), quote(fmt::format("{}%", upnpClass))));
    }
    where.push_back(fmt::format("{} LIKE {}", identifier("location"), quote(fmt::format("{}%", (isVirtual) ? LOC_VIRT_PREFIX : LOC_FILE_PREFIX))));
    //<< " AND is_virtual = 0";

    auto query = fmt::format("SELECT COUNT(*) FROM {} WHERE {}", identifier(CDS_OBJECT_TABLE), fmt::join(where, " AND "));
    auto res = select(query);

    std::unique_ptr<SQLRow> row;
    if (res && (row = res->nextRow())) {
        return row->col_int(0);
    }
    return 0;
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
    auto getSql = (withoutContainer) //
        ? fmt::format("SELECT {0}id{1} FROM {0}{2}{1} WHERE {0}parent_id{1} = {3} AND {0}object_type{1} != {4}", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, parentID, OBJECT_TYPE_CONTAINER)
        : fmt::format("SELECT {0}id{1} FROM {0}{2}{1} WHERE {0}parent_id{1} = {3}", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, parentID);
    auto res = select(getSql);
    if (!res)
        throw_std_runtime_error("db error");

    if (res->getNumRows() == 0)
        return {};

    std::unordered_set<int> ret;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        ret.insert(row->col_int(0));
    }
    return ret;
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObjects(const std::unordered_set<int>& list, bool all)
{
    size_t count = list.size();
    if (count <= 0)
        return nullptr;

    auto it = std::find_if(list.begin(), list.end(), IS_FORBIDDEN_CDS_ID);
    if (it != list.end()) {
        throw_std_runtime_error("Tried to delete a forbidden ID ({})", *it);
    }

    auto res = select(fmt::format("SELECT {0}id{1}, {0}object_type{1} FROM {0}{2}{1} WHERE {0}id{1} IN ({3})", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, fmt::join(list, ",")));
    if (!res)
        throw_std_runtime_error("sql error");

    std::vector<int32_t> items;
    std::vector<int32_t> containers;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        const int32_t objectID = row->col_int(0);
        const int objectType = row->col_int(1);
        if (IS_CDS_CONTAINER(objectType))
            containers.push_back(objectID);
        else
            items.push_back(objectID);
    }

    auto rr = _recursiveRemove(items, containers, all);
    return _purgeEmptyContainers(rr);
}

void SQLDatabase::_removeObjects(const std::vector<int32_t>& objectIDs)
{
    auto sel = fmt::format("SELECT {}, {}, {} FROM {} JOIN {} ON {} = {} WHERE {} IN ({})",
        asColumnMapper->mapQuoted(AutoscanCol::id), asColumnMapper->mapQuoted(AutoscanCol::persistent), browseColumnMapper->mapQuoted(BrowseCol::location),
        asColumnMapper->tableQuoted(), browseColumnMapper->tableQuoted(),
        browseColumnMapper->mapQuoted(BrowseCol::id), asColumnMapper->mapQuoted(AutoscanCol::obj_id), browseColumnMapper->mapQuoted(BrowseCol::id), fmt::join(objectIDs, ","));
    log_debug("{}", sel);

    beginTransaction("_removeObjects");
    auto res = select(sel);
    if (res) {
        log_debug("relevant autoscans!");
        std::vector<std::string> delete_as;
        std::unique_ptr<SQLRow> row;
        while ((row = res->nextRow())) {
            bool persistent = remapBool(row->col(1));
            if (persistent) {
                auto location = stripLocationPrefix(row->col(2));
                exec(fmt::format("UPDATE {0}{2}{1} SET {0}obj_id{1} = {3}, {0}location{1} = {4} WHERE {0}id{1} = {5}", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, SQL_NULL, quote(location.string()), quote(row->col(0))));
            } else {
                auto col = std::string(row->col_c_str(0));
                delete_as.push_back(std::move(col));
            }
            log_debug("relevant autoscan: {}; persistent: {}", row->col_c_str(0), persistent);
        }

        if (!delete_as.empty()) {
            auto delAutoscan = fmt::format("DELETE FROM {} WHERE {} IN ({})", identifier(AUTOSCAN_TABLE), identifier("id"), fmt::join(delete_as, ", "));
            exec(delAutoscan);
            log_debug("deleting autoscans: {}", delAutoscan);
        }
    }

    exec(fmt::format("DELETE FROM {} WHERE {} IN ({})", identifier(CDS_OBJECT_TABLE), identifier("id"), fmt::join(objectIDs, ",")));
    commit("_removeObjects");
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObject(int objectID, bool all)
{
    auto res = select(fmt::format("SELECT {0}object_type{1}, {0}ref_id{1} FROM {0}{2}{1} WHERE {0}id{1} = {3} LIMIT 1", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, quote(objectID)));
    if (!res)
        return nullptr;

    auto row = res->nextRow();
    if (!row)
        return nullptr;

    const int objectType = row->col_int(0);
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && !isContainer) {
        if (!row->isNull(1)) {
            const int ref_id = row->col_int(1);
            if (!IS_FORBIDDEN_CDS_ID(ref_id))
                objectID = ref_id;
        }
    }
    if (IS_FORBIDDEN_CDS_ID(objectID))
        throw_std_runtime_error("Tried to delete a forbidden ID ({})", objectID);
    std::vector<int32_t> itemIds;
    std::vector<int32_t> containerIds;
    if (isContainer) {
        containerIds.push_back(objectID);
    } else {
        itemIds.push_back(objectID);
    }
    auto changedContainers = _recursiveRemove(itemIds, containerIds, all);
    return _purgeEmptyContainers(changedContainers);
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::_recursiveRemove(
    const std::vector<int32_t>& items, const std::vector<int32_t>& containers,
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
    auto containersSql = (all) //
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
            changedContainers.ui.push_back(row->col_int(0));
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
                changedContainers.upnp.push_back(row->col_int(0));
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
                removeIds.push_back(row->col_int(0));
                changedContainers.upnp.push_back(row->col_int(1));
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
                const int objId = row->col_int(0);
                const int objType = row->col_int(1);
                if (IS_CDS_CONTAINER(objType)) {
                    containerIds.push_back(objId);
                    removeIds.push_back(objId);
                } else {
                    if (all) {
                        if (!row->isNull(2)) {
                            const int32_t refId = row->col_int(2);
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
    return std::make_unique<ChangedContainers>(changedContainers);
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::_purgeEmptyContainers(const std::unique_ptr<ChangedContainers>& maybeEmpty)
{
    log_debug("start upnp: {}; ui: {}", fmt::join(maybeEmpty->upnp, ","), fmt::join(maybeEmpty->ui, ","));
    if (maybeEmpty->upnp.empty() && maybeEmpty->ui.empty())
        return nullptr;

    constexpr auto tabAlias = "fol";
    constexpr auto childAlias = "cld";
    auto fields = std::vector {
        fmt::format("{}.{}", identifier(tabAlias), identifier("id")),
        fmt::format("COUNT({}.{})", identifier(childAlias), identifier("parent_id")),
        fmt::format("{}.{}", identifier(tabAlias), identifier("parent_id")),
        fmt::format("{}.{}", identifier(tabAlias), identifier("flags")),
    };
    std::string selectSql = fmt::format("SELECT {2} FROM {5} {3} LEFT JOIN {5} {4} ON {0}{3}{1}.{0}id{1} = {0}{4}{1}.{0}parent_id{1} WHERE {0}{3}{1}.{0}object_type{1} = {6} AND {0}{3}{1}.{0}id{1} ",
        table_quote_begin, table_quote_end, fmt::join(fields, ","), tabAlias, childAlias, CDS_OBJECT_TABLE, quote(OBJECT_TYPE_CONTAINER));

    std::vector<int32_t> del;

    std::unique_ptr<SQLRow> row;

    ChangedContainers changedContainers;

    auto selUi = std::vector(maybeEmpty->ui);
    auto selUpnp = std::vector(maybeEmpty->upnp);

    bool again;
    int count = 0;
    do {
        again = false;

        if (!selUpnp.empty()) {
            auto sql = fmt::format("{2} IN ({3}) GROUP BY {0}{4}{1}.{0}id{1}", table_quote_begin, table_quote_end, selectSql, fmt::join(selUpnp, ","), tabAlias);
            log_debug("upnp-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUpnp.clear();
            if (!res)
                throw_std_runtime_error("db error");
            while ((row = res->nextRow())) {
                const int flags = row->col_int(3);
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers.upnp.push_back(row->col_int(0));
                else if (row->col(1) == "0") {
                    del.push_back(row->col_int(0));
                    selUi.push_back(row->col_int(2));
                } else {
                    selUpnp.push_back(row->col_int(0));
                }
            }
        }

        if (!selUi.empty()) {
            auto sql = fmt::format("{2} IN ({3}) GROUP BY {0}{4}{1}.{0}id{1}", table_quote_begin, table_quote_end, selectSql, fmt::join(selUi, ","), tabAlias);
            log_debug("ui-sql: {}", sql);
            std::shared_ptr<SQLResult> res = select(sql);
            selUi.clear();
            if (!res)
                throw_std_runtime_error("db error");
            while ((row = res->nextRow())) {
                const int flags = row->col_int(3);
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER) {
                    changedContainers.ui.push_back(row->col_int(0));
                    changedContainers.upnp.push_back(row->col_int(0));
                } else if (row->col(1) == "0") {
                    del.push_back(row->col_int(0));
                    selUi.push_back(row->col_int(2));
                } else {
                    selUi.push_back(row->col_int(0));
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

    auto& changedUi = changedContainers.ui;
    auto& changedUpnp = changedContainers.upnp;
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

    return std::make_unique<ChangedContainers>(changedContainers);
}

std::string SQLDatabase::getInternalSetting(const std::string& key)
{
    auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {2} = {3} LIMIT 1",
        identifier("value"), identifier(INTERNAL_SETTINGS_TABLE), identifier("key"), quote(key)));
    if (!res)
        return "";

    auto row = res->nextRow();
    if (!row)
        return "";
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
    auto res = select(fmt::format("SELECT {} FROM {}", fmt::join(fields, ", "), identifier(CONFIG_VALUE_TABLE)));
    if (!res)
        return {};

    std::vector<ConfigValue> result;
    result.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        result.push_back({ row->col(1),
            row->col(0),
            row->col(2),
            row->col(3) });
    }

    log_debug("loading {} items", result.size());
    return result;
}

void SQLDatabase::removeConfigValue(const std::string& item)
{
    auto del = (item == "*") //
        ? fmt::format("DELETE FROM {}", identifier(CONFIG_VALUE_TABLE)) //
        : fmt::format("DELETE FROM {} WHERE {} = {}", identifier(CONFIG_VALUE_TABLE), identifier("item"), quote(item));
    log_info("Deleting config item '{}'", item);
    exec(del);
}

void SQLDatabase::updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status)
{
    auto res = select(fmt::format("SELECT {0} FROM {1} WHERE {0} = {2} LIMIT 1",
        identifier("item"), identifier(CONFIG_VALUE_TABLE), quote(item)));
    if (!res || !res->nextRow()) {
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
        log_debug("inserted for {} as {} {}", key, item, value);
    } else {
        exec(fmt::format("UPDATE {0}{2}{1} SET {0}item_value{1} = {3} WHERE {0}item{1} = {4}", table_quote_begin, table_quote_end, CONFIG_VALUE_TABLE, quote(value), quote(item)));
        log_debug("updated for {} as {} {}", key, item, value);
    }
}

void SQLDatabase::updateAutoscanList(ScanMode scanmode, std::shared_ptr<AutoscanList> list)
{
    log_debug("setting persistent autoscans untouched - scanmode: {};", AutoscanDirectory::mapScanmode(scanmode));

    beginTransaction("updateAutoscanList");
    exec(fmt::format("UPDATE {0}{2}{1} SET {0}touched{1} = {3} WHERE {0}persistent{1} = {4} AND {0}scan_mode{1} = {5}", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, mapBool(false), mapBool(true), quote(AutoscanDirectory::mapScanmode(scanmode))));
    commit("updateAutoscanList");

    size_t listSize = list->size();
    log_debug("updating/adding persistent autoscans (count: {})", listSize);
    for (size_t i = 0; i < listSize; i++) {
        log_debug("getting ad {} from list..", i);
        auto ad = list->get(i);
        if (!ad)
            continue;

        // only persistent asD should be given to getAutoscanList
        assert(ad->persistent());
        // the scanmode should match the given parameter
        assert(ad->getScanMode() == scanmode);

        std::string location = ad->getLocation();
        if (location.empty())
            throw_std_runtime_error("AutoscanDirectoy with illegal location given to SQLDatabase::updateAutoscanPersistentList");

        int objectID = findObjectIDByPath(location);
        log_debug("objectID = {}", objectID);
        auto where = (objectID == INVALID_OBJECT_ID) ? fmt::format("{} = {}", identifier("location"), quote(location)) : fmt::format("{} = {}", identifier("obj_id"), quote(objectID));

        beginTransaction("updateAutoscanList x");
        auto res = select(fmt::format("SELECT {0}id{1} FROM {0}{2}{1} WHERE {3} LIMIT 1", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, where));
        if (!res) {
            rollback("updateAutoscanList x");
            throw DatabaseException("", "query error while selecting from autoscan list");
        }
        commit("updateAutoscanList x");

        auto row = res->nextRow();
        if (row) {
            ad->setDatabaseID(row->col_int(0));
            updateAutoscanDirectory(ad);
        } else
            addAutoscanDirectory(ad);
    }

    beginTransaction("updateAutoscanList delete");
    exec(fmt::format("DELETE FROM {0}{2}{1} WHERE {0}touched{1} = {3} AND {0}scan_mode{1} = {4}", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, mapBool(false), quote(AutoscanDirectory::mapScanmode(scanmode))));
    commit("updateAutoscanList delete");
}

std::shared_ptr<AutoscanList> SQLDatabase::getAutoscanList(ScanMode scanmode)
{
    std::string selectSql = fmt::format("{2} WHERE {0}{3}{1}.{0}scan_mode{1} = {4}", table_quote_begin, table_quote_end, sql_autoscan_query, AUS_ALIAS, quote(AutoscanDirectory::mapScanmode(scanmode)));

    auto res = select(selectSql);
    if (!res)
        throw DatabaseException("", "query error while fetching autoscan list");

    auto self = getSelf();
    auto ret = std::make_shared<AutoscanList>(self);
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto adir = _fillAutoscanDirectory(row);
        if (!adir)
            _removeAutoscanDirectory(row->col_int(0));
        else
            ret->add(adir);
    }
    return ret;
}

std::shared_ptr<AutoscanDirectory> SQLDatabase::getAutoscanDirectory(int objectID)
{
    std::string selectSql = fmt::format("{2} WHERE {0}{3}{1}.{0}id{1} = {4}", table_quote_begin, table_quote_end, sql_autoscan_query, ITM_ALIAS, quote(objectID));

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
    std::string objectIDstr = getCol(row, AutoscanColumn::obj_id);
    if (!objectIDstr.empty())
        objectID = std::stoi(objectIDstr);
    int databaseID = std::stoi(getCol(row, AutoscanColumn::id));

    fs::path location;
    if (objectID == INVALID_OBJECT_ID) {
        location = getCol(row, AutoscanColumn::location);
    } else {
        char prefix;
        location = stripLocationPrefix(getCol(row, AutoscanColumn::obj_location), &prefix);
        if (prefix != LOC_DIR_PREFIX)
            return nullptr;
    }

    ScanMode mode = AutoscanDirectory::remapScanmode(getCol(row, AutoscanColumn::scan_mode));
    bool recursive = remapBool(getCol(row, AutoscanColumn::recursive));
    bool hidden = remapBool(getCol(row, AutoscanColumn::hidden));
    bool persistent = remapBool(getCol(row, AutoscanColumn::persistent));
    int interval = 0;
    if (mode == ScanMode::Timed)
        interval = std::stoi(getCol(row, AutoscanColumn::interval));
    auto last_modified = std::chrono::seconds(std::stol(getCol(row, AutoscanColumn::last_modified)));

    log_info("Loading autoscan location: {}; recursive: {}, last_modified: {}", location.c_str(), recursive, last_modified > std::chrono::seconds::zero() ? fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(last_modified.count())) : "unset");

    auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, persistent, INVALID_SCAN_ID, interval, hidden);
    dir->setObjectID(objectID);
    dir->setDatabaseID(databaseID);
    dir->setCurrentLMT("", last_modified);
    if (last_modified > std::chrono::seconds::zero()) {
        dir->setCurrentLMT(location, last_modified);
    }
    dir->updateLMT();
    return dir;
}

void SQLDatabase::addAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir)
{
    if (!adir)
        throw_std_runtime_error("addAutoscanDirectory called with adir==nullptr");
    if (adir->getDatabaseID() >= 0)
        throw_std_runtime_error("tried to add autoscan directory with a database id set");
    int objectID;
    if (adir->getLocation() == FS_ROOT_DIRECTORY)
        objectID = CDS_ID_FS_ROOT;
    else
        objectID = findObjectIDByPath(adir->getLocation());
    if (!adir->persistent() && objectID < 0)
        throw_std_runtime_error("tried to add non-persistent autoscan directory with an illegal objectID or location");

    auto pathIds = _checkOverlappingAutoscans(adir);

    _autoscanChangePersistentFlag(objectID, true);

    auto fields = std::vector {
        identifier("obj_id"),
        identifier("scan_level"),
        identifier("scan_mode"),
        identifier("recursive"),
        identifier("hidden"),
        identifier("interval"),
        identifier("last_modified"),
        identifier("persistent"),
        identifier("location"),
        identifier("path_ids"),
    };
    auto values = std::vector {
        objectID >= 0 ? quote(objectID) : SQL_NULL,
        quote("full"),
        quote(AutoscanDirectory::mapScanmode(adir->getScanMode())),
        mapBool(adir->getRecursive()),
        mapBool(adir->getHidden()),
        quote(adir->getInterval().count()),
        quote(adir->getPreviousLMT().count()),
        mapBool(adir->persistent()),
        objectID >= 0 ? SQL_NULL : quote(adir->getLocation()),
        pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ","))),
    };
    adir->setDatabaseID(insert(AUTOSCAN_TABLE, fields, values, true));
}

void SQLDatabase::updateAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir)
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
        fmt::format("{} = {}", identifier("obj_id"), objectID >= 0 ? quote(objectID) : SQL_NULL),
        fmt::format("{} = {}", identifier("scan_level"), quote("full")),
        fmt::format("{} = {}", identifier("scan_mode"), quote(AutoscanDirectory::mapScanmode(adir->getScanMode()))),
        fmt::format("{} = {}", identifier("recursive"), mapBool(adir->getRecursive())),
        fmt::format("{} = {}", identifier("hidden"), mapBool(adir->getHidden())),
        fmt::format("{} = {}", identifier("interval"), quote(adir->getInterval().count())),
        fmt::format("{} = {}", identifier("persistent"), mapBool(adir->persistent())),
        fmt::format("{} = {}", identifier("location"), objectID >= 0 ? SQL_NULL : quote(adir->getLocation())),
        fmt::format("{} = {}", identifier("path_ids"), pathIds.empty() ? SQL_NULL : quote(fmt::format(",{},", fmt::join(pathIds, ",")))),
        fmt::format("{} = {}", identifier("touched"), mapBool(true)),
    };
    if (adir->getPreviousLMT() > std::chrono::seconds::zero()) {
        fields.push_back(fmt::format("{} = {}", identifier("last_modified"), quote(adir->getPreviousLMT().count())));
    }
    exec(fmt::format("UPDATE {} SET {} WHERE {} = {}",
        identifier(AUTOSCAN_TABLE), fmt::join(fields, ","), identifier("id"), quote(adir->getDatabaseID())));
}

void SQLDatabase::removeAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir)
{
    _removeAutoscanDirectory(adir->getDatabaseID());
}

void SQLDatabase::_removeAutoscanDirectory(int autoscanID)
{
    if (autoscanID == INVALID_OBJECT_ID)
        return;
    int objectID = _getAutoscanObjectID(autoscanID);
    exec(fmt::format("DELETE FROM {0}{2}{1} WHERE {0}id{1} = {3}", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, quote(autoscanID)));
    if (objectID != INVALID_OBJECT_ID)
        _autoscanChangePersistentFlag(objectID, false);
}

int SQLDatabase::_getAutoscanObjectID(int autoscanID)
{
    auto res = select(fmt::format("SELECT {0}obj_id{1} FROM {0}{2}{1} WHERE {0}id{1} = {3} LIMIT 1", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, quote(autoscanID)));
    if (!res)
        throw DatabaseException("", "error while doing select on ");
    auto row = res->nextRow();
    if (row && !row->isNull(0))
        return row->col_int(0);

    return INVALID_OBJECT_ID;
}

void SQLDatabase::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID || objectID == INVALID_OBJECT_ID_2)
        return;

    exec(fmt::format("UPDATE {0}{2}{1} SET {0}flags{1} = ({0}flags{1} {3}{4}) WHERE {0}id{1} = {5}", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, (persistent ? " | " : " & ~"), OBJECT_FLAG_PERSISTENT_CONTAINER, quote(objectID)));
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
            fmt::format("{}obj_id{} = {}", table_quote_begin, table_quote_end, quote(checkObjectID)),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{}id{} != {}", table_quote_begin, table_quote_end, quote(databaseID)));

        auto res = select(fmt::format("SELECT {0}id{1} FROM {0}{2}{1} WHERE {3}", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, fmt::join(where, " AND ")));
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
            fmt::format("{}path_ids{} LIKE {}", table_quote_begin, table_quote_end, quote(fmt::format("%,{},%", checkObjectID))),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{}id{} != {}", table_quote_begin, table_quote_end, quote(databaseID)));
        auto qRec = fmt::format("SELECT {0}obj_id{1} FROM {0}{2}{1} WHERE {3} LIMIT 1", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, fmt::join(where, " AND "));
        log_debug("------------ {}", qRec);
        auto res = select(qRec);
        if (!res)
            throw_std_runtime_error("SQL error");
        row = res->nextRow();
        if (row) {
            const int objectID = row->col_int(0);
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
            fmt::format("{}obj_id{} IN ({})", table_quote_begin, table_quote_end, fmt::join(pathIDs, ",")),
            fmt::format("{}recursive{} = {}", table_quote_begin, table_quote_end, mapBool(true)),
        };
        if (databaseID >= 0)
            where.push_back(fmt::format("{}id{} != {}", table_quote_begin, table_quote_end, quote(databaseID)));
        auto res = select(fmt::format("SELECT {0}obj_id{1} FROM {0}{2}{1} WHERE {3} LIMIT 1", table_quote_begin, table_quote_end, AUTOSCAN_TABLE, fmt::join(where, " AND ")));
        if (!res)
            throw_std_runtime_error("SQL error");
        if (!(row = res->nextRow()))
            return pathIDs;
    }

    const int objectID = row->col_int(0);
    auto obj = loadObject(objectID);
    if (!obj) {
        throw_std_runtime_error("Referenced object (by Autoscan) not found.");
    }
    log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
    throw_std_runtime_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
}

std::vector<int> SQLDatabase::getPathIDs(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return {};

    auto sel = fmt::format("SELECT {0}parent_id{1} FROM {0}{2}{1} WHERE {0}id{1} = ", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE);

    std::vector<int> pathIDs;
    while (objectID != CDS_ID_ROOT) {
        pathIDs.push_back(objectID);
        auto res = select(fmt::format("{} {} LIMIT 1", sel, quote(objectID)));
        if (!res)
            break;
        auto row = res->nextRow();
        if (!row)
            break;
        objectID = row->col_int(0);
    }
    return pathIDs;
}

std::string SQLDatabase::getFsRootName()
{
    if (!fsRootName.empty())
        return fsRootName;
    setFsRootName();
    return fsRootName;
}

void SQLDatabase::setFsRootName(const std::string& rootName)
{
    if (!rootName.empty()) {
        fsRootName = rootName;
    } else {
        auto fsRootObj = loadObject(CDS_ID_FS_ROOT);
        fsRootName = fsRootObj->getTitle();
    }
}

void SQLDatabase::clearFlagInDB(int flag)
{
    exec(fmt::format("UPDATE {0}{2}{1} SET {0}flags{1} = ({0}flags{1} & ~{3}) WHERE {0}flags{1} & {3}", table_quote_begin, table_quote_end, CDS_OBJECT_TABLE, flag));
}

void SQLDatabase::generateMetadataDBOperations(const std::shared_ptr<CdsObject>& obj, Operation op,
    std::vector<std::shared_ptr<AddUpdateTable>>& operations)
{
    auto dict = obj->getMetadata();
    if (op == Operation::Insert) {
        for (auto&& [key, val] : dict) {
            std::map<std::string, std::string> metadataSql;
            metadataSql["property_name"] = quote(key);
            metadataSql["property_value"] = quote(val);
            operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, op));
        }
    } else {
        // get current metadata from DB: if only it really was a dictionary...
        auto dbMetadata = retrieveMetadataForObject(obj->getID());
        for (auto&& [key, val] : dict) {
            Operation operation = dbMetadata.find(key) == dbMetadata.end() ? Operation::Insert : Operation::Update;
            std::map<std::string, std::string> metadataSql;
            metadataSql["property_name"] = quote(key);
            metadataSql["property_value"] = quote(val);
            operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, operation));
        }
        for (auto&& [key, val] : dbMetadata) {
            if (dict.find(key) == dict.end()) {
                // key in db metadata but not obj metadata, so needs a delete
                std::map<std::string, std::string> metadataSql;
                metadataSql["property_name"] = quote(key);
                metadataSql["property_value"] = quote(val);
                operations.push_back(std::make_shared<AddUpdateTable>(METADATA_TABLE, metadataSql, Operation::Delete));
            }
        }
    }
}

std::vector<std::shared_ptr<CdsResource>> SQLDatabase::retrieveResourcesForObject(int objectId)
{
    auto rsql = fmt::format("{3} FROM {0}{2}{1} WHERE {0}item_id{1} = {4} ORDER BY {0}res_id{1}",
        table_quote_begin, table_quote_end, RESOURCE_TABLE, sql_resource_query, objectId);
    log_debug("SQLDatabase::retrieveResourcesForObject {}", rsql);
    auto&& res = select(rsql);

    if (!res)
        return {};

    std::vector<std::shared_ptr<CdsResource>> resources;
    resources.reserve(res->getNumRows());
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto resource = std::make_shared<CdsResource>(std::stoi(getCol(row, ResourceCol::handlerType)));
        resource->decode(getCol(row, ResourceCol::options), getCol(row, ResourceCol::parameters));
        resource->setResId(resources.size());
        for (auto&& resAttrId : ResourceAttributeIterator()) {
            auto index = to_underlying(ResourceCol::attributes) + to_underlying(resAttrId);
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
    std::vector<std::shared_ptr<AddUpdateTable>>& operations)
{
    auto resources = obj->getResources();
    if (op == Operation::Insert) {
        size_t res_id = 0;
        for (auto&& resource : resources) {
            std::map<std::string, std::string> resourceSql;
            resourceSql["res_id"] = quote(res_id);
            resourceSql["handlerType"] = quote(resource->getHandlerType());
            auto options = resource->getOptions();
            if (!options.empty()) {
                resourceSql["options"] = quote(dictEncode(options));
            }
            auto parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql["parameters"] = quote(dictEncode(parameters));
            }
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSql[key] = quote(val);
            }
            operations.push_back(std::make_shared<AddUpdateTable>(RESOURCE_TABLE, resourceSql, op));
            res_id++;
        }
    } else {
        // get current resoures from DB
        auto dbResources = retrieveResourcesForObject(obj->getID());
        size_t res_id = 0;
        for (auto&& resource : resources) {
            Operation operation = res_id < dbResources.size() ? Operation::Update : Operation::Insert;
            std::map<std::string, std::string> resourceSql;
            resourceSql["res_id"] = quote(res_id);
            resourceSql["handlerType"] = quote(resource->getHandlerType());
            auto options = resource->getOptions();
            if (!options.empty()) {
                resourceSql["options"] = quote(dictEncode(options));
            }
            auto parameters = resource->getParameters();
            if (!parameters.empty()) {
                resourceSql["parameters"] = quote(dictEncode(parameters));
            }
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSql[key] = quote(val);
            }
            operations.push_back(std::make_shared<AddUpdateTable>(RESOURCE_TABLE, resourceSql, operation));
            res_id++;
        }
        // res_id in db resources but not obj resources, so needs a delete
        for (; res_id < dbResources.size(); res_id++) {
            if (dbResources.at(res_id)) {
                std::map<std::string, std::string> resourceSql;
                resourceSql["res_id"] = quote(res_id);
                operations.push_back(std::make_shared<AddUpdateTable>(RESOURCE_TABLE, resourceSql, Operation::Delete));
            }
        }
    }
}

std::string SQLDatabase::sqlForInsert(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::vector<SQLIdentifier> fields;
    std::vector<std::string> values;
    fields.reserve(dict.size() + 1); // extra only used for METADATA_TABLE and RESOURCE_TABLE
    values.reserve(dict.size() + 1); // extra only used for METADATA_TABLE and RESOURCE_TABLE

    if (tableName == METADATA_TABLE || tableName == RESOURCE_TABLE) {
        fields.push_back(identifier("item_id"));
        values.push_back(fmt::format("{}", obj->getID()));
    }
    for (auto&& [field, value] : dict) {
        fields.push_back(identifier(field));
        values.push_back(fmt::format("{}", value));
    }

    if (tableName == CDS_OBJECT_TABLE && obj->getID() != INVALID_OBJECT_ID) {
        throw_std_runtime_error("Attempted to insert new object with ID!");
    }

    return fmt::format("INSERT INTO {} ({}) VALUES ({})", identifier(tableName), fmt::join(fields, ", "), fmt::join(values, ", "));
}

std::string SQLDatabase::sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    if (!addUpdateTable
        || (addUpdateTable->getTableName() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw_std_runtime_error("sqlForUpdate called with invalid arguments");

    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::vector<std::string> fields;
    fields.reserve(dict.size());
    for (auto&& [field, value] : dict) {
        fields.push_back(fmt::format("{} = {}", identifier(field), value));
    }

    std::vector<std::string> where;
    if (tableName == RESOURCE_TABLE) {
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("res_id"), dict["res_id"]));
    } else if (tableName == METADATA_TABLE) {
        // relying on only one element when tableName is mt_metadata
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("property_name"), dict.begin()->second));
    } else {
        where.push_back(fmt::format("{} = {}", identifier("id"), obj->getID()));
    }

    return fmt::format("UPDATE {} SET {} WHERE {}", identifier(tableName), fmt::join(fields, ", "), fmt::join(where, " AND "));
}

std::string SQLDatabase::sqlForDelete(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    if (!addUpdateTable
        || (addUpdateTable->getTableName() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw_std_runtime_error("sqlForDelete called with invalid arguments");

    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::vector<std::string> where;
    if (tableName == RESOURCE_TABLE) {
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
        where.push_back(fmt::format("{} = {}", identifier("res_id"), dict["res_id"]));
    } else if (tableName == METADATA_TABLE) {
        // relying on only one element when tableName is mt_metadata
        where.push_back(fmt::format("{} = {}", identifier("item_id"), obj->getID()));
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
    int expectedConversionCount = res->nextRow()->col_int(0);
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    res = select(fmt::format("SELECT COUNT(*) FROM {}", identifier(METADATA_TABLE)));
    int metadataRowCount = res->nextRow()->col_int(0);
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
        migrateMetadata(row->col_int(0), row->col(1));
        ++objectsUpdated;
    }
    log_info("Migrated metadata - object count: {}", objectsUpdated);
    return true;
}

void SQLDatabase::migrateMetadata(int objectId, const std::string& metadataStr)
{
    std::map<std::string, std::string> dict;
    dictDecode(metadataStr, &dict);

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
        for (auto&& [key, val] : metadataSQLVals) {
            auto values = std::vector {
                fmt::format("{}", objectId),
                fmt::format("{}", key),
                fmt::format("{}", val),
            };
            insert(METADATA_TABLE, fields, values);
        }
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", objectId);
    }
}

void SQLDatabase::prepareResourceTable(const std::string& addColumnCmd)
{
    auto resourceAttributes = splitString(getInternalSetting("resource_attribute"), ',');
    bool addedAttribute = false;
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto&& resAttrib = MetadataHandler::getResAttrName(resAttrId);
        if (std::find_if(resourceAttributes.begin(), resourceAttributes.end(), [&](auto&& attr) { return attr == resAttrib; }) == resourceAttributes.end()) {
            _exec(fmt::format(addColumnCmd, resAttrib));
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
    int expectedConversionCount = res->nextRow()->col_int(0);
    log_debug("{} rows having resources: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    res = select(
        fmt::format("SELECT COUNT(*) FROM {}",
            identifier(RESOURCE_TABLE)));
    int resourceRowCount = res->nextRow()->col_int(0);
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
        migrateResources(row->col_int(0), row->col(1));
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
        size_t res_id = 0;
        for (auto&& resString : resources) {
            std::map<std::string, std::string> resourceSQLVals;
            auto&& resource = CdsResource::decode(resString);
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSQLVals[key] = quote(val);
            }
            auto fields = std::vector {
                identifier("item_id"),
                identifier("res_id"),
                identifier("handlerType"),
            };
            auto values = std::vector {
                fmt::format("{}", objectId),
                fmt::format("{}", res_id),
                fmt::format("{}", resource->getHandlerType()),
            };
            auto options = resource->getOptions();
            if (!options.empty()) {
                fields.push_back(identifier("options"));
                values.push_back(fmt::format("{}", quote(dictEncode(options))));
            }
            auto parameters = resource->getParameters();
            if (!parameters.empty()) {
                fields.push_back(identifier("parameters"));
                values.push_back(fmt::format("{}", quote(dictEncode(parameters))));
            }
            for (auto&& [key, val] : resourceSQLVals) {
                fields.push_back(identifier(key));
                values.push_back(fmt::format("{}", val));
            }
            insert(RESOURCE_TABLE, fields, values);
            res_id++;
        }
    } else {
        log_debug("Skipping migration - no resources for cds object {}", objectId);
    }
}

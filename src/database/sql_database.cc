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
#include <sstream>
#include <string>
#include <vector>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "config/config_setup.h"
#include "config/dynamic_content.h"
#include "content/autoscan.h"
#include "metadata/metadata_handler.h"
#include "search_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#define MAX_REMOVE_SIZE 1000
#define MAX_REMOVE_RECURSION 500

#define SQL_NULL "NULL"

#define RESOURCE_SEP '|'

/* table quote */
#define TQ(data) QTB << (data) << QTE
/* table quote with dot */
#define TQD(data1, data2) TQ(data1) << '.' << TQ(data2)
#define TQBM(data) browseColumnMapper->mapQuoted((data))
#define TQSM(data) searchColumnMapper->mapQuoted((data))

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

/// \brief Map browse column ids to column names
// map ensures entries are in correct order, each value of BrowseCol must be present
static const std::map<BrowseCol, std::pair<std::string, std::string>> browseColMap = {
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
static const std::map<SearchCol, std::pair<std::string, std::string>> searchColMap = {
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
static const std::map<MetadataCol, std::pair<std::string, std::string>> metaColMap = {
    { MetadataCol::id, { MTA_ALIAS, "id" } },
    { MetadataCol::item_id, { MTA_ALIAS, "item_id" } },
    { MetadataCol::property_name, { MTA_ALIAS, "property_name" } },
    { MetadataCol::property_value, { MTA_ALIAS, "property_value" } },
};

/// \brief Map browse sort keys to column ids
// entries are handled sequentially,
// duplicate entries are added to statement in same order if key is present in SortCriteria
static const std::vector<std::pair<std::string, BrowseCol>> browseSortMap = {
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
static const std::vector<std::pair<std::string, SearchCol>> searchSortMap = {
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
static const std::vector<std::pair<std::string, MetadataCol>> metaTagMap = {
    { "id", MetadataCol::id },
    { UPNP_SEARCH_ID, MetadataCol::item_id },
    { META_NAME, MetadataCol::property_name },
    { META_VALUE, MetadataCol::property_value },
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
static std::shared_ptr<EnumColumnMapper<int>> resourceColumnMapper;

SQLDatabase::SQLDatabase(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : Database(std::move(config))
    , mime(std::move(mime))
    , use_transaction(this->config->getBoolOption(CFG_SERVER_STORAGE_USE_TRANSACTIONS))
{
}

void SQLDatabase::init()
{
    if (table_quote_begin == '\0' || table_quote_end == '\0')
        throw_std_runtime_error("quote vars need to be overridden");

    /// \brief Map resource search keys to column ids
    // entries are handled sequentially,
    // duplicate entries are added to statement in same order if key is present in SortCriteria
    std::vector<std::pair<std::string, int>> resourceTagMap = {
        { "id", to_underlying(ResourceCol::id) },
        { UPNP_SEARCH_ID, to_underlying(ResourceCol::item_id) },
        { "res@id", to_underlying(ResourceCol::res_id) },
    };
    /// \brief Map resource column ids to column names
    // map ensures entries are in correct order, each value of ResourceCol must be present
    std::map<int, std::pair<std::string, std::string>> resourceColMap = {
        { to_underlying(ResourceCol::id), { RES_ALIAS, "id" } },
        { to_underlying(ResourceCol::item_id), { RES_ALIAS, "item_id" } },
        { to_underlying(ResourceCol::res_id), { RES_ALIAS, "res_id" } },
        { to_underlying(ResourceCol::handlerType), { RES_ALIAS, "handlerType" } },
        { to_underlying(ResourceCol::options), { RES_ALIAS, "options" } },
        { to_underlying(ResourceCol::parameters), { RES_ALIAS, "parameters" } },
    };
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = MetadataHandler::getResAttrName(resAttrId);
        resourceTagMap.emplace_back(std::pair<std::string, int>(fmt::format("res@{}", attrName), to_underlying(ResourceCol::attributes) + resAttrId));
        resourceColMap[to_underlying(ResourceCol::attributes) + resAttrId] = std::pair<std::string, std::string>(RES_ALIAS, attrName);
    }

    browseColumnMapper = std::make_shared<EnumColumnMapper<BrowseCol>>(table_quote_begin, table_quote_end, ITM_ALIAS, CDS_OBJECT_TABLE, browseSortMap, browseColMap);
    searchColumnMapper = std::make_shared<EnumColumnMapper<SearchCol>>(table_quote_begin, table_quote_end, SRC_ALIAS, CDS_OBJECT_TABLE, searchSortMap, searchColMap);
    metaColumnMapper = std::make_shared<EnumColumnMapper<MetadataCol>>(table_quote_begin, table_quote_end, MTA_ALIAS, METADATA_TABLE, metaTagMap, metaColMap);
    resourceColumnMapper = std::make_shared<EnumColumnMapper<int>>(table_quote_begin, table_quote_end, RES_ALIAS, RESOURCE_TABLE, resourceTagMap, resourceColMap);

    // Statement for UPnP browse
    {
        std::ostringstream buf;
        buf << "SELECT ";
        for (auto&& [key, col] : browseColMap) {
            if (key > BrowseCol::id) {
                buf << ", ";
            }
            buf << TQD(col.first, col.second);
        }
        buf << " FROM " << browseColumnMapper->tableQuoted();
        buf << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ(REF_ALIAS) << " ON " << TQBM(BrowseCol::ref_id) << "=" << TQD(REF_ALIAS, browseColMap.at(BrowseCol::id).second);
        buf << " LEFT JOIN " << TQ(AUTOSCAN_TABLE) << ' ' << TQ(AUS_ALIAS) << " ON " << TQD(AUS_ALIAS, "obj_id") << "=" << TQBM(BrowseCol::id);
        buf << " ";
        this->sql_browse_query = buf.str();
    }
    // Statement for UPnP search
    {
        std::ostringstream colBuf;
        for (auto&& [key, col] : searchColMap) {
            if (key > SearchCol::id) {
                colBuf << ", ";
            }
            colBuf << TQD(col.first, col.second);
        }
        this->sql_search_columns = colBuf.str();
        std::ostringstream buf;
        buf << searchColumnMapper->tableQuoted();
        buf << fmt::format(" INNER JOIN {} ON {} = {}", metaColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), metaColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        buf << fmt::format(" INNER JOIN {} ON {} = {}", resourceColumnMapper->tableQuoted(), searchColumnMapper->mapQuoted(UPNP_SEARCH_ID), resourceColumnMapper->mapQuoted(UPNP_SEARCH_ID));
        this->sql_search_query = buf.str();
    }
    // Statement for metadata
    {
        std::ostringstream buf;
        buf << "SELECT ";
        for (auto&& [key, col] : metaColMap) {
            if (key > MetadataCol::id) {
                buf << ", ";
            }
            buf << TQ(col.second); // currently no alias
        }
        buf << " ";
        this->sql_meta_query = buf.str();
    }
    // Statement for resource
    {
        std::ostringstream buf;
        buf << "SELECT ";
        for (auto&& [key, col] : resourceColMap) {
            if (key > to_underlying(ResourceCol::id)) {
                buf << ", ";
            }
            buf << TQ(col.second); // currently no alias
        }
        buf << " ";
        this->sql_resource_query = buf.str();
    }

    sqlEmitter = std::make_shared<DefaultSQLEmitter>(searchColumnMapper, metaColumnMapper, resourceColumnMapper);
}

void SQLDatabase::upgradeDatabase(std::string& dbVersion, const std::array<unsigned int, DBVERSION>& hashies, config_option_t upgradeOption, std::string_view updateVersionCommand, std::string_view addResourceColumnCmd)
{
    /* --- load database upgrades from config file --- */
    const fs::path& upgradeFile = config->getOption(upgradeOption);
    log_debug("db_version: {}", dbVersion);
    log_debug("Loading SQL Upgrades from: {}", upgradeFile.c_str());
    std::vector<std::unique_ptr<std::vector<std::pair<std::string, std::string>>>> dbUpdates;
    pugi::xml_document xmlDoc;
    pugi::xml_parse_result result = xmlDoc.load_file(upgradeFile.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    auto root = xmlDoc.document_element();
    if (root.name() != std::string_view("upgrade"))
        throw std::runtime_error("Error in upgrade file: <upgrade> tag not found");

    size_t version = 1;
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        const pugi::xml_node& versionNode = versionElement.node();
        auto versionCmds = std::make_unique<std::vector<std::pair<std::string, std::string>>>();
        std::ostringstream ostr;
        versionNode.print(ostr);
        auto&& myHash = stringHash(ostr.str());
        if (version < DBVERSION && myHash == hashies[version]) {
            for (auto&& script : versionNode.select_nodes("script")) {
                const pugi::xml_node& scriptNode = script.node();
                std::string migration = trimString(scriptNode.attribute("migration").as_string());
                versionCmds->push_back(std::pair(migration, trimString(scriptNode.text().as_string())));
            }
        } else {
            log_error("Wrong hash for version {}: {} != {}", version + 1, myHash, hashies[version]);
            throw_std_runtime_error("Wrong hash for version {}", version + 1);
        }
        dbUpdates.push_back(std::move(versionCmds));
        version++;
    }

    version = 1;
    static const std::map<std::string, bool (SQLDatabase::*)()> migActions = {
        { "metadata", &SQLDatabase::doMetadataMigration },
        { "resources", &SQLDatabase::doResourceMigration },
    };
    this->addResourceColumnCmd = addResourceColumnCmd;
    /* --- run database upgrades --- */
    for (auto&& upgrade : dbUpdates) {
        if (dbVersion == fmt::to_string(version)) {
            log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
            for (auto&& [migrationCmd, upgradeCmd] : *upgrade) {
                bool actionResult = true;
                if (!migrationCmd.empty() && migActions.find(migrationCmd) != migActions.end())
                    actionResult = (*this.*(migActions.at(migrationCmd)))();
                if (actionResult && !upgradeCmd.empty())
                    _exec(upgradeCmd.c_str());
            }
            _exec(fmt::format(updateVersionCommand, version + 1, version).c_str());
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
    auto sortKeys = std::vector<std::string>();
    for (auto&& [key, col] : browseSortMap) {
        if (std::find(sortKeys.begin(), sortKeys.end(), key) == sortKeys.end()) {
            sortKeys.emplace_back(key);
        }
    }
    return join(sortKeys, ',');
}

std::string SQLDatabase::getSearchCapabilities()
{
    auto searchKeys = std::vector<std::string> {
        MetadataHandler::getMetaFieldName(M_TITLE),
        UPNP_SEARCH_CLASS,
        MetadataHandler::getMetaFieldName(M_ARTIST),
        MetadataHandler::getMetaFieldName(M_ALBUM),
        MetadataHandler::getMetaFieldName(M_GENRE),
    };
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto attrName = MetadataHandler::getResAttrName(resAttrId);
        searchKeys.emplace_back(fmt::format("res@{}", attrName));
    }
    return join(searchKeys, ',');
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

    std::vector<std::shared_ptr<SQLDatabase::AddUpdateTable>> returnVal;

    // check for a duplicate (virtual) object
    if (hasReference && op != Operation::Update) {
        std::ostringstream q;
        q << "SELECT " << TQ("id")
          << " FROM " << TQ(CDS_OBJECT_TABLE)
          << " WHERE " << TQ("parent_id")
          << '=' << quote(obj->getParentID())
          << " AND " << TQ("ref_id")
          << '=' << quote(refObj->getID())
          << " AND " << TQ("dc_title")
          << '=' << quote(obj->getTitle())
          << " LIMIT 1";
        auto res = select(q);
        // if duplicate items is found - ignore
        if (res && (res->nextRow()))
            return returnVal;
    }

    if (obj->getParentID() == INVALID_OBJECT_ID) {
        throw_std_runtime_error("Tried to create or update an object {} with an illegal parent id {}", obj->getLocation().c_str(), obj->getParentID());
    }

    cdsObjectSql["parent_id"] = fmt::to_string(obj->getParentID());
    returnVal.push_back(std::make_shared<AddUpdateTable>(CDS_OBJECT_TABLE, cdsObjectSql, op));

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

    std::vector<std::shared_ptr<SQLDatabase::AddUpdateTable>> tables = _addUpdateObject(obj, Operation::Insert, changedContainer);

    beginTransaction("addObject");
    for (auto&& addUpdateTable : tables) {
        auto qb = sqlForInsert(obj, addUpdateTable);
        log_debug("Generated insert: {}", qb->str().c_str());

        if (addUpdateTable->getTableName() == CDS_OBJECT_TABLE) {
            int newId = exec(qb->str(), true);
            obj->setID(newId);
        } else {
            exec(qb->str(), false);
        }
    }
    commit("addObject");
}

void SQLDatabase::updateObject(std::shared_ptr<CdsObject> obj, int* changedContainer)
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
        Operation op = addUpdateTable->getOperation();
        std::unique_ptr<std::ostringstream> qb;

        switch (op) {
        case Operation::Insert:
            qb = sqlForInsert(obj, addUpdateTable);
            break;
        case Operation::Update:
            qb = sqlForUpdate(obj, addUpdateTable);
            break;
        case Operation::Delete:
            qb = sqlForDelete(obj, addUpdateTable);
            break;
        }

        log_debug("upd_query: {}", qb->str());
        exec(qb->str());
    }
    commit("updateObject");
}

std::shared_ptr<CdsObject> SQLDatabase::loadObject(int objectID)
{
    if (dynamicContainers.find(objectID) != dynamicContainers.end()) {
        return dynamicContainers.at(objectID);
    }

    std::ostringstream qb;

    qb << sql_browse_query << " WHERE " << TQBM(BrowseCol::id) << '=' << objectID;

    beginTransaction("loadObject");
    auto res = select(qb);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            auto result = createObjectFromRow(row);
            commit("loadObject");
            return result;
        }
    }
    log_debug("sql_query = {}", qb.str());
    commit("loadObject");
    throw ObjectNotFoundException(fmt::format("Object not found: {}", objectID));
}

std::shared_ptr<CdsObject> SQLDatabase::loadObjectByServiceID(const std::string& serviceID)
{
    std::ostringstream qb;
    qb << sql_browse_query << " WHERE " << TQBM(BrowseCol::service_id) << '=' << quote(serviceID);
    beginTransaction("loadObjectByServiceID");
    auto res = select(qb);
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

std::unique_ptr<std::vector<int>> SQLDatabase::getServiceObjectIDs(char servicePrefix)
{
    std::ostringstream qb;
    qb << "SELECT " << TQ("id")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("service_id")
       << " LIKE " << quote(std::string(1, servicePrefix) + '%');

    beginTransaction("getServiceObjectIDs");
    auto res = select(qb);
    commit("getServiceObjectIDs");
    if (!res)
        throw_std_runtime_error("db error");

    std::vector<int> objectIDs;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        objectIDs.push_back(std::stoi(row->col(0)));
    }

    return std::make_unique<std::vector<int>>(objectIDs);
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

    std::ostringstream qb;
    qb << sql_browse_query << " WHERE ";

    if (param->getFlag(BROWSE_DIRECT_CHILDREN) && parent->isContainer()) {
        int count = param->getRequestedCount();
        bool doLimit = true;
        if (!count) {
            if (param->getStartingIndex())
                count = std::numeric_limits<int>::max();
            else
                doLimit = false;
        }

        qb << TQBM(BrowseCol::parent_id) << '=' << parent->getID();

        if (parent->getID() == CDS_ID_ROOT && hideFsRoot)
            qb << " AND " << TQBM(BrowseCol::id) << "!=" << quote(CDS_ID_FS_ROOT);

        // order by code..
        auto orderByCode = [&]() {
            std::ostringstream orderQb;
            if (param->getFlag(BROWSE_TRACK_SORT)) {
                orderQb << TQBM(BrowseCol::part_number) << ',';
                orderQb << TQBM(BrowseCol::track_number);
            } else {
                SortParser sortParser(browseColumnMapper, param->getSortCriteria());
                orderQb << sortParser.parse();
            }
            if (orderQb.str().empty()) {
                orderQb << TQBM(BrowseCol::dc_title);
            }
            return orderQb.str();
        };

        if (!getContainers && !getItems) {
            qb << " AND 0=1";
        } else if (getContainers && !getItems) {
            qb << " AND " << TQBM(BrowseCol::object_type) << '='
               << quote(OBJECT_TYPE_CONTAINER)
               << " ORDER BY " << orderByCode();
        } else if (!getContainers && getItems) {
            qb << " AND (" << TQBM(BrowseCol::object_type) << " & "
               << quote(OBJECT_TYPE_ITEM) << ") = "
               << quote(OBJECT_TYPE_ITEM)
               << " ORDER BY " << orderByCode();
        } else {
            qb << " ORDER BY ("
               << TQBM(BrowseCol::object_type) << '=' << quote(OBJECT_TYPE_CONTAINER)
               << ") DESC, " << orderByCode();
        }
        if (doLimit)
            qb << " LIMIT " << count << " OFFSET " << param->getStartingIndex();
    } else { // metadata
        qb << TQBM(BrowseCol::id) << '=' << parent->getID() << " LIMIT 1";
    }
    log_debug("QUERY: {}", qb.str().c_str());
    beginTransaction("browse");
    std::shared_ptr<SQLResult> sqlResult = select(qb);
    commit("browse");

    std::vector<std::shared_ptr<CdsObject>> result;

    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        auto obj = createObjectFromRow(row);
        result.push_back(obj);
        row = nullptr; // clear out content from unique_ptr
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
                            dynFolder->addResource(move(resource));
                        }
                        dynamicContainers[dynId] = dynFolder;
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
    std::string countSQL(fmt::format("SELECT COUNT(*) FROM {} WHERE {}", sql_search_query, searchSQL));
    log_debug("Search count resolves to SQL [{}]", countSQL);
    auto sqlResult = select(countSQL.c_str());
    commit("search");
    auto countRow = sqlResult->nextRow();
    if (countRow) {
        *numMatches = std::stoi(countRow->col(0));
    }

    std::ostringstream retrievalSQL;
    retrievalSQL << fmt::format("SELECT DISTINCT {} FROM {} WHERE {}", sql_search_columns, sql_search_query, searchSQL);

    // order by code..
    auto orderByCode = [&]() {
        std::ostringstream orderQb;
        SortParser sortParser(searchColumnMapper, param->getSortCriteria());
        orderQb << sortParser.parse();
        if (orderQb.str().empty()) {
            orderQb << TQSM(SearchCol::id);
        }
        return orderQb.str();
    };

    retrievalSQL << " ORDER BY " << orderByCode();

    size_t startingIndex = param->getStartingIndex();
    size_t requestedCount = param->getRequestedCount();
    if (startingIndex > 0 || requestedCount > 0) {
        retrievalSQL << " LIMIT " << (requestedCount == 0 ? 10000000000 : requestedCount)
                     << " OFFSET " << startingIndex;
    }

    log_debug("Search resolves to SQL [{}]", retrievalSQL.str().c_str());
    beginTransaction("search 2");
    sqlResult = select(retrievalSQL);
    commit("search 2");

    std::vector<std::shared_ptr<CdsObject>> result;

    std::unique_ptr<SQLRow> row;
    while ((row = sqlResult->nextRow())) {
        auto obj = createObjectFromSearchRow(row);
        result.push_back(obj);
        row = nullptr; // clear out content from unique_ptr
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

    std::ostringstream qb;
    qb << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("parent_id") << '=' << contId;
    if (containers && !items)
        qb << " AND " << TQ("object_type") << '=' << OBJECT_TYPE_CONTAINER;
    else if (items && !containers)
        qb << " AND (" << TQ("object_type") << " & " << OBJECT_TYPE_ITEM
           << ") = " << OBJECT_TYPE_ITEM;
    if (contId == CDS_ID_ROOT && hideFsRoot) {
        qb << " AND " << TQ("id") << "!=" << quote(CDS_ID_FS_ROOT);
    }
    beginTransaction("getChildCount");
    auto res = select(qb);
    commit("getChildCount");

    if (res) {
        auto row = res->nextRow();
        if (row) {
            int childCount = std::stoi(row->col(0));
            return childCount;
        }
    }
    return 0;
}

std::vector<std::string> SQLDatabase::getMimeTypes()
{
    std::vector<std::string> arr;

    std::ostringstream qb;
    qb << "SELECT DISTINCT " << TQ("mime_type")
       << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("mime_type") << " IS NOT NULL ORDER BY "
       << TQ("mime_type");
    beginTransaction("getMimeTypes");
    auto res = select(qb);
    commit("getMimeTypes");
    if (!res)
        throw_std_runtime_error("db error");

    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        arr.push_back(std::string(row->col(0)));
    }

    return arr;
}

std::shared_ptr<CdsObject> SQLDatabase::findObjectByPath(fs::path fullpath, bool wasRegularFile)
{
    std::string dbLocation;
    std::error_code ec;
    if (isRegularFile(fullpath, ec) || wasRegularFile)
        dbLocation = addLocationPrefix(LOC_FILE_PREFIX, fullpath);
    else
        dbLocation = addLocationPrefix(LOC_DIR_PREFIX, fullpath);

    std::ostringstream qb;
    qb << sql_browse_query
       << " WHERE " << TQBM(BrowseCol::location_hash) << '=' << quote(stringHash(dbLocation))
       << " AND " << TQBM(BrowseCol::location) << '=' << quote(dbLocation)
       << " AND " << TQBM(BrowseCol::ref_id) << " IS NULL"
                                                " LIMIT 1";

    beginTransaction("findObjectByPath");
    auto res = select(qb);
    if (!res) {
        commit("findObjectByPath");
        throw_std_runtime_error("error while doing select: {}", qb.str());
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

int SQLDatabase::findObjectIDByPath(fs::path fullpath, bool wasRegularFile)
{
    auto obj = findObjectByPath(fullpath, wasRegularFile);
    if (!obj)
        return INVALID_OBJECT_ID;
    return obj->getID();
}

int SQLDatabase::ensurePathExistence(fs::path path, int* changedContainer)
{
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

    std::ostringstream qb;
    qb << "INSERT INTO "
       << TQ(CDS_OBJECT_TABLE)
       << " ("
       << TQ("parent_id") << ','
       << TQ("object_type") << ','
       << TQ("upnp_class") << ','
       << TQ("dc_title") << ','
       << TQ("location") << ','
       << TQ("location_hash") << ','
       << TQ("ref_id") << ") VALUES ("
       << parentID << ','
       << OBJECT_TYPE_CONTAINER << ','
       << (!upnpClass.empty() ? quote(upnpClass) : quote(UPNP_CLASS_CONTAINER)) << ','
       << quote(std::move(name)) << ','
       << quote(dbLocation) << ','
       << quote(stringHash(dbLocation)) << ',';
    if (refID > 0) {
        qb << refID;
    } else {
        qb << SQL_NULL;
    }
    qb << ')';

    beginTransaction("createContainer");
    int newId = exec(qb.str(), true); // true = get last id#
    log_debug("Created object row, id: {}", newId);

    if (!itemMetadata.empty()) {
        for (auto&& [key, val] : itemMetadata) {
            std::ostringstream ib;
            ib << "INSERT INTO "
               << TQ(METADATA_TABLE)
               << " ("
               << TQ("item_id") << ','
               << TQ("property_name") << ','
               << TQ("property_value") << ") VALUES ("
               << newId << ","
               << quote(key) << ","
               << quote(val)
               << ")";
            exec(ib.str());
        }
        log_debug("Wrote metadata for cds_object {}", newId);
    }
    commit("createContainer");

    return newId;
}

fs::path SQLDatabase::buildContainerPath(int parentID, const std::string& title)
{
    //title = escape(title, xxx);
    if (parentID == CDS_ID_ROOT)
        return fmt::format("{}{}", VIRTUAL_CONTAINER_SEPARATOR, title);

    std::ostringstream qb;
    qb << "SELECT " << TQ("location") << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("id") << '=' << parentID << " LIMIT 1";

    beginTransaction("buildContainerPath");
    auto res = select(qb);
    commit("buildContainerPath");
    if (!res)
        return "";

    auto row = res->nextRow();
    if (!row)
        return "";

    char prefix;
    auto path = stripLocationPrefix(fmt::format("{}{}{}", row->col(0), VIRTUAL_CONTAINER_SEPARATOR, title), &prefix);
    if (prefix != LOC_VIRT_PREFIX)
        throw_std_runtime_error("Tried to build a virtual container path with an non-virtual parentID");

    return path;
}

void SQLDatabase::addContainerChain(std::string virtualPath, const std::string& lastClass, int lastRefID, int* containerID, std::vector<int>& updateID, const std::map<std::string, std::string>& lastMetadata)
{
    log_debug("Adding container Chain for path: {}, lastRefId: {}, containerId: {}", virtualPath.c_str(), lastRefID, *containerID);

    reduceString(virtualPath, VIRTUAL_CONTAINER_SEPARATOR);
    if (virtualPath == std::string(1, VIRTUAL_CONTAINER_SEPARATOR)) {
        *containerID = CDS_ID_ROOT;
        return;
    }
    std::ostringstream qb;
    std::string dbLocation = addLocationPrefix(LOC_VIRT_PREFIX, virtualPath);
    qb << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE)
       << " WHERE " << TQ("location_hash") << '=' << quote(stringHash(dbLocation))
       << " AND " << TQ("location") << '=' << quote(dbLocation)
       << " LIMIT 1";

    beginTransaction("addContainerChain");
    auto res = select(qb);
    if (res) {
        auto row = res->nextRow();
        if (row) {
            if (containerID)
                *containerID = std::stoi(row->col(0));
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
    addContainerChain(newpath, classes.empty() ? "" : join(classes, '/'), INVALID_OBJECT_ID, &parentContainerID, updateID, std::map<std::string, std::string>());

    *containerID = createContainer(parentContainerID, container, virtualPath, true, newClass, lastRefID, lastMetadata);
    updateID.emplace(updateID.begin(), *containerID);
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
        return "";
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
    std::ostringstream qb;
    qb << sql_meta_query
       << " FROM " << TQ(METADATA_TABLE)
       << " WHERE " << TQ("item_id")
       << " = " << objectId;
    auto res = select(qb);
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
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE "
          << TQ("object_type") << " != " << quote(OBJECT_TYPE_CONTAINER);
    if (!mimeType.empty()) {
        query << " AND " << TQ("mime_type") << " LIKE " << quote(mimeType + "%");
    }
    if (!upnpClass.empty()) {
        query << " AND " << TQ("upnp_class") << " LIKE " << quote(upnpClass + "%");
    }
    if (isVirtual) {
        query << " AND " << TQ("location") << " LIKE " << quote(fmt::format("{}%", LOC_VIRT_PREFIX));
    } else {
        query << " AND " << TQ("location") << " LIKE " << quote(fmt::format("{}%", LOC_FILE_PREFIX));
    }
    //<< " AND is_virtual = 0";

    auto res = select(query);

    std::unique_ptr<SQLRow> row;
    if (res && (row = res->nextRow())) {
        return std::stoi(row->col(0));
    }
    return 0;
}

std::string SQLDatabase::incrementUpdateIDs(const std::unique_ptr<std::unordered_set<int>>& ids)
{
    if (ids->empty())
        return "";
    std::ostringstream inBuf;

    bool first = true;
    for (auto&& id : *ids) {
        if (first) {
            inBuf << "IN (" << id;
            first = false;
        } else {
            inBuf << ',' << id;
        }
    }
    inBuf << ')';

    std::ostringstream bufUpdate;
    bufUpdate << "UPDATE " << TQ(CDS_OBJECT_TABLE) << " SET " << TQ("update_id")
              << '=' << TQ("update_id") << " + 1 WHERE " << TQ("id") << ' ';
    bufUpdate << inBuf.str();
    beginTransaction("incrementUpdateIDs");
    exec(bufUpdate.str());

    std::ostringstream bufSelect;
    bufSelect << "SELECT " << TQ("id") << ',' << TQ("update_id") << " FROM "
              << TQ(CDS_OBJECT_TABLE) << " WHERE " << TQ("id") << ' ';
    bufSelect << inBuf.str();
    auto res = select(bufSelect);
    if (!res) {
        rollback("incrementUpdateIDs 2");
        throw_std_runtime_error("Error while fetching update ids");
    }
    commit("incrementUpdateIDs 2");

    std::unique_ptr<SQLRow> row;
    std::vector<std::string> rows;
    while ((row = res->nextRow())) {
        std::ostringstream s;
        s << row->col(0) << ',' << row->col(1);
        rows.emplace_back(s.str());
    }

    if (rows.empty())
        return "";
    return join(rows, ",");
}

std::unique_ptr<std::unordered_set<int>> SQLDatabase::getObjects(int parentID, bool withoutContainer)
{
    std::ostringstream q;
    q << "SELECT " << TQ("id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    if (withoutContainer)
        q << TQ("object_type") << " != " << OBJECT_TYPE_CONTAINER << " AND ";
    q << TQ("parent_id") << '=';
    q << parentID;
    auto res = select(q);
    if (!res)
        throw_std_runtime_error("db error");
    if (res->getNumRows() == 0)
        return nullptr;

    std::unordered_set<int> ret;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        ret.insert(std::stoi(row->col(0)));
    }
    return std::make_unique<std::unordered_set<int>>(std::move(ret));
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObjects(const std::unique_ptr<std::unordered_set<int>>& list, bool all)
{
    size_t count = list->size();
    if (count <= 0)
        return nullptr;

    auto it = std::find_if(list->begin(), list->end(), IS_FORBIDDEN_CDS_ID);
    if (it != list->end()) {
        throw_std_runtime_error("Tried to delete a forbidden ID ({})", *it);
    }

    std::ostringstream idsBuf;
    idsBuf << "SELECT " << TQ("id") << ',' << TQ("object_type")
           << " FROM " << TQ(CDS_OBJECT_TABLE)
           << " WHERE " << TQ("id") << " IN (" << join(*list, ",") << ")";

    auto res = select(idsBuf);
    if (!res)
        throw_std_runtime_error("sql error");

    std::vector<int32_t> items;
    std::vector<int32_t> containers;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        int objectType = std::stoi(row->col(1));
        if (IS_CDS_CONTAINER(objectType))
            containers.push_back(std::stol(row->col(0)));
        else
            items.push_back(std::stol(row->col(0)));
    }

    auto rr = _recursiveRemove(items, containers, all);
    return _purgeEmptyContainers(rr);
}

void SQLDatabase::_removeObjects(const std::vector<int32_t>& objectIDs)
{
    auto objectIdsStr = join(objectIDs, ',');
    std::ostringstream sel;
    sel << "SELECT " << TQD('a', "id") << ',' << TQD('a', "persistent")
        << ',' << TQD('o', "location")
        << " FROM " << TQ(AUTOSCAN_TABLE) << " a"
                                             " JOIN "
        << TQ(CDS_OBJECT_TABLE) << " o"
                                   " ON "
        << TQD('o', "id") << '=' << TQD('a', "obj_id")
        << " WHERE " << TQD('o', "id")
        << " IN (" << objectIdsStr << ')';

    log_debug("{}", sel.str());

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
                std::ostringstream u;
                u << "UPDATE " << TQ(AUTOSCAN_TABLE)
                  << " SET " << TQ("obj_id") << "=" SQL_NULL
                  << ',' << TQ("location") << '=' << quote(location.string())
                  << " WHERE " << TQ("id") << '=' << quote(row->col(0));
                exec(u.str());
            } else {
                delete_as.emplace_back(row->col_c_str(0));
            }
            log_debug("relevant autoscan: {}; persistent: {}", row->col_c_str(0), persistent);
        }

        if (!delete_as.empty()) {
            std::ostringstream delAutoscan;
            delAutoscan << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
                        << " WHERE " << TQ("id") << " IN ("
                        << join(delete_as, ',')
                        << ')';
            exec(delAutoscan.str());
            log_debug("deleting autoscans: {}", delAutoscan.str().c_str());
        }
    }

    std::ostringstream qObject;
    qObject << "DELETE FROM " << TQ(CDS_OBJECT_TABLE)
            << " WHERE " << TQ("id")
            << " IN (" << objectIdsStr << ')';
    exec(qObject.str());
    commit("_removeObjects");
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::removeObject(int objectID, bool all)
{
    std::ostringstream q;
    q << "SELECT " << TQ("object_type") << ',' << TQ("ref_id")
      << " FROM " << TQ(CDS_OBJECT_TABLE)
      << " WHERE " << TQ("id") << '=' << quote(objectID) << " LIMIT 1";
    auto res = select(q);
    if (!res)
        return nullptr;

    auto row = res->nextRow();
    if (!row)
        return nullptr;

    int objectType = std::stoi(row->col(0));
    bool isContainer = IS_CDS_CONTAINER(objectType);
    if (all && !isContainer) {
        std::string ref_id_str = row->col(1);
        int ref_id;
        if (!ref_id_str.empty()) {
            ref_id = std::stoi(ref_id_str);
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
    // select statements
    std::ostringstream itemsSql;
    itemsSql << "SELECT DISTINCT " << TQ("id") << ',' << TQ("parent_id")
             << " FROM " << TQ(CDS_OBJECT_TABLE)
             << " WHERE " << TQ("ref_id") << " IN (";

    std::ostringstream containersSql;
    containersSql << "SELECT DISTINCT " << TQ("id") << ',' << TQ("object_type");
    if (all) {
        containersSql << ',' << TQ("ref_id");
    }
    containersSql << " FROM " << TQ(CDS_OBJECT_TABLE)
                  << " WHERE " << TQ("parent_id") << " IN (";

    std::ostringstream parentsSql;
    parentsSql << "SELECT DISTINCT " << TQ("parent_id")
               << " FROM " << TQ(CDS_OBJECT_TABLE)
               << " WHERE " << TQ("id") << " IN (";

    ChangedContainers changedContainers;

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

    std::vector<int32_t> itemIds(items);
    std::vector<int32_t> containerIds(containers);
    std::vector<int32_t> parentIds(items);
    std::vector<int32_t> removeIds(containers);

    // collect container for update signals
    if (!containers.empty()) {
        parentIds.insert(parentIds.end(), containers.begin(), containers.end());
        std::ostringstream sql;
        sql << parentsSql.str() << join(parentIds, ',') << ')';
        res = select(sql);
        if (!res)
            throw DatabaseException("", fmt::format("Sql error: {}", sql.str()));
        parentIds.clear();
        while ((row = res->nextRow())) {
            changedContainers.ui.push_back(std::stoi(row->col(0)));
        }
    }

    int count = 0;
    while (!itemIds.empty() || !parentIds.empty() || !containerIds.empty()) {
        // collect child entries
        if (!parentIds.empty()) {
            // add ids to remove
            removeIds.insert(removeIds.end(), parentIds.begin(), parentIds.end());
            std::ostringstream sql;
            sql << parentsSql.str() << join(parentIds, ',') << ')';
            res = select(sql);
            if (!res)
                throw DatabaseException("", fmt::format("Sql error: {}", sql.str()));
            parentIds.clear();
            while ((row = res->nextRow())) {
                changedContainers.upnp.push_back(std::stoi(row->col(0)));
            }
        }

        // collect entries
        if (!itemIds.empty()) {
            std::ostringstream sql;
            sql << itemsSql.str() << join(itemIds, ',') << ')';
            res = select(sql);
            if (!res)
                throw DatabaseException("", fmt::format("Sql error: {}", sql.str()));
            itemIds.clear();
            while ((row = res->nextRow())) {
                removeIds.push_back(std::stoi(row->col(0)));
                changedContainers.upnp.push_back(std::stoi(row->col(1)));
            }
        }

        // collect entries in container
        if (!containerIds.empty()) {
            std::ostringstream sql;
            sql << containersSql.str() << join(containerIds, ',') << ')';
            res = select(sql);
            if (!res)
                throw DatabaseException("", fmt::format("Sql error: {}", sql.str()));
            containerIds.clear();
            while ((row = res->nextRow())) {
                int objectType = std::stoi(row->col(1));
                if (IS_CDS_CONTAINER(objectType)) {
                    containerIds.push_back(std::stoi(row->col(0)));
                    removeIds.push_back(std::stoi(row->col(0)));
                } else {
                    if (all) {
                        std::string refId = row->col(2);
                        if (!refId.empty()) {
                            parentIds.push_back(std::stoi(row->col(2)));
                            itemIds.push_back(std::stoi(row->col(2)));
                            removeIds.push_back(std::stoi(row->col(2)));
                        } else {
                            removeIds.push_back(std::stoi(row->col(0)));
                            itemIds.push_back(std::stoi(row->col(0)));
                        }
                    } else {
                        removeIds.push_back(std::stoi(row->col(0)));
                        itemIds.push_back(std::stoi(row->col(0)));
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

std::string SQLDatabase::toCSV(const std::vector<int>& input)
{
    return join(input, ",");
}

std::unique_ptr<Database::ChangedContainers> SQLDatabase::_purgeEmptyContainers(const std::unique_ptr<ChangedContainers>& maybeEmpty)
{
    log_debug("start upnp: {}; ui: {}",
        join(maybeEmpty->upnp, ',').c_str(),
        join(maybeEmpty->ui, ',').c_str());
    if (maybeEmpty->upnp.empty() && maybeEmpty->ui.empty())
        return nullptr;

    std::ostringstream selectSql;
    selectSql << "SELECT " << TQD('a', "id")
              << ", COUNT(" << TQD('b', "parent_id")
              << ")," << TQD('a', "parent_id") << ',' << TQD('a', "flags")
              << " FROM " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('a')
              << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('b')
              << " ON " << TQD('a', "id") << '=' << TQD('b', "parent_id")
              << " WHERE " << TQD('a', "object_type") << '=' << quote(1)
              << " AND " << TQD('a', "id") << " IN ("; //(a.flags & " << OBJECT_FLAG_PERSISTENT_CONTAINER << ") = 0 AND
    std::string strSel2(") GROUP BY a.id"); // HAVING COUNT(b.parent_id)=0");

    std::ostringstream bufSelUpnp;
    bufSelUpnp << selectSql.str();

    std::vector<int32_t> del;

    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;

    std::vector<int32_t> selUi;
    std::vector<int32_t> selUpnp;

    ChangedContainers changedContainers;

    auto& uiV = maybeEmpty->ui;
    selUi.insert(selUi.end(), uiV.begin(), uiV.end());
    auto& upnpV = maybeEmpty->upnp;
    selUpnp.insert(selUpnp.end(), upnpV.begin(), upnpV.end());

    bool again;
    int count = 0;
    do {
        again = false;

        if (!selUpnp.empty()) {
            std::ostringstream sql;
            sql << selectSql.str() << join(selUpnp, ',') << strSel2;
            log_debug("upnp-sql: {}", sql.str().c_str());
            res = select(sql.str());
            selUpnp.clear();
            if (!res)
                throw_std_runtime_error("db error");
            while ((row = res->nextRow())) {
                int flags = std::stoi(row->col(3));
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER)
                    changedContainers.upnp.push_back(std::stoi(row->col(0)));
                else if (row->col(1) == "0") {
                    del.push_back(std::stoi(row->col(0)));
                    selUi.push_back(std::stoi(row->col(2)));
                } else {
                    selUpnp.push_back(std::stoi(row->col(0)));
                }
            }
        }

        if (!selUi.empty()) {
            std::ostringstream sql;
            sql << selectSql.str() << join(selUi, ',') << strSel2;
            log_debug("ui-sql: {}", sql.str().c_str());
            res = select(sql.str());
            selUi.clear();
            if (!res)
                throw_std_runtime_error("db error");
            while ((row = res->nextRow())) {
                int flags = std::stoi(row->col(3));
                if (flags & OBJECT_FLAG_PERSISTENT_CONTAINER) {
                    changedContainers.ui.push_back(std::stoi(row->col(0)));
                    changedContainers.upnp.push_back(std::stoi(row->col(0)));
                } else if (row->col(1) == "0") {
                    del.push_back(std::stoi(row->col(0)));
                    selUi.push_back(std::stoi(row->col(2)));
                } else {
                    selUi.push_back(std::stoi(row->col(0)));
                }
            }
        }

        //log_debug("selecting: {}; removing: {}", bufSel->c_str(), join(del, ',').c_str());
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
        changedUi.insert(changedUi.end(), selUi.begin(), selUi.end());
        changedUpnp.insert(changedUpnp.end(), selUi.begin(), selUi.end());
    }
    if (!selUpnp.empty()) {
        changedUpnp.insert(changedUpnp.end(), selUpnp.begin(), selUpnp.end());
    }
    // log_debug("end; changedContainers (upnp): {}", join(changedUpnp, ',').c_str());
    // log_debug("end; changedContainers (ui): {}", join(changedUi, ',').c_str());
    log_debug("end; changedContainers (upnp): {}", changedUpnp.size());
    log_debug("end; changedContainers (ui): {}", changedUi.size());

    return std::make_unique<ChangedContainers>(changedContainers);
}

std::string SQLDatabase::getInternalSetting(const std::string& key)
{
    std::ostringstream q;
    q << "SELECT " << TQ("value") << " FROM " << TQ(INTERNAL_SETTINGS_TABLE) << " WHERE " << TQ("key") << '='
      << quote(key) << " LIMIT 1";
    auto res = select(q);
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
    std::ostringstream query;
    query << "SELECT DISTINCT "
          << TQ("item") << ','
          << TQ("key") << ','
          << TQ("item_value") << ','
          << TQ("status")
          << " FROM "
          << TQ(CONFIG_VALUE_TABLE);
    auto res = select(query);
    if (!res)
        return {};

    std::vector<ConfigValue> result;
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
    std::ostringstream del;
    del << "DELETE FROM " << TQ(CONFIG_VALUE_TABLE);
    if (item != "*") {
        del << " WHERE " << TQ("item") << '=' << quote(item);
    }
    log_info("deleting {} item", item);
    exec(del.str());
}

void SQLDatabase::updateConfigValue(const std::string& key, const std::string& item, const std::string& value, const std::string& status)
{
    std::ostringstream query;
    query << "SELECT "
          << TQ("item")
          << " FROM "
          << TQ(CONFIG_VALUE_TABLE)
          << " WHERE "
          << TQ("item") << '=' << quote(item)
          << " LIMIT 1";
    auto res = select(query);
    if (!res || !res->nextRow()) {
        std::ostringstream insert;
        insert << "INSERT INTO "
               << TQ(CONFIG_VALUE_TABLE)
               << " ("
               << TQ("item") << ','
               << TQ("key") << ','
               << TQ("item_value") << ','
               << TQ("status")
               << ") VALUES ("
               << quote(item) << ','
               << quote(key) << ','
               << quote(value) << ','
               << quote(status)
               << ')';
        exec(insert.str());
        log_debug("inserted for {} as {} {}", key, item, value);
    } else {
        std::ostringstream update;
        update << "UPDATE "
               << TQ(CONFIG_VALUE_TABLE)
               << " SET "
               << TQ("item_value") << '=' << quote(value)
               << " WHERE "
               << TQ("item") << '=' << quote(item);
        exec(update.str());
        log_debug("updated for {} as {} {}", key, item, value);
    }
}

void SQLDatabase::updateAutoscanList(ScanMode scanmode, std::shared_ptr<AutoscanList> list)
{
    log_debug("setting persistent autoscans untouched - scanmode: {};", AutoscanDirectory::mapScanmode(scanmode));
    std::ostringstream update;
    update << "UPDATE " << TQ(AUTOSCAN_TABLE)
           << " SET " << TQ("touched") << '=' << mapBool(false)
           << " WHERE "
           << TQ("persistent") << '=' << mapBool(true)
           << " AND " << TQ("scan_mode") << '='
           << quote(AutoscanDirectory::mapScanmode(scanmode));
    beginTransaction("updateAutoscanList");
    exec(update.str());
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

        std::ostringstream q;
        q << "SELECT " << TQ("id") << " FROM " << TQ(AUTOSCAN_TABLE)
          << " WHERE ";
        int objectID = findObjectIDByPath(location);
        log_debug("objectID = {}", objectID);
        if (objectID == INVALID_OBJECT_ID)
            q << TQ("location") << '=' << quote(location);
        else
            q << TQ("obj_id") << '=' << quote(objectID);
        q << " LIMIT 1";
        beginTransaction("updateAutoscanList x");
        auto res = select(q);
        if (!res) {
            rollback("updateAutoscanList x");
            throw DatabaseException("", "query error while selecting from autoscan list");
        }
        commit("updateAutoscanList x");

        auto row = res->nextRow();
        if (row) {
            ad->setDatabaseID(std::stoi(row->col(0)));
            updateAutoscanDirectory(ad);
        } else
            addAutoscanDirectory(ad);
    }

    std::ostringstream del;
    del << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
        << " WHERE " << TQ("touched") << '=' << mapBool(false)
        << " AND " << TQ("scan_mode") << '='
        << quote(AutoscanDirectory::mapScanmode(scanmode));
    beginTransaction("updateAutoscanList delete");
    exec(del.str());
    commit("updateAutoscanList delete");
}

std::shared_ptr<AutoscanList> SQLDatabase::getAutoscanList(ScanMode scanmode)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
      << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
      << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
      << " ON " FLD("obj_id") '=' << TQD('t', "id")
      << " WHERE " FLD("scan_mode") '=' << quote(AutoscanDirectory::mapScanmode(scanmode));
    auto res = select(q);
    if (!res)
        throw DatabaseException("", "query error while fetching autoscan list");

    auto self = getSelf();
    auto ret = std::make_shared<AutoscanList>(self);
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto adir = _fillAutoscanDirectory(row);
        if (!adir)
            _removeAutoscanDirectory(std::stoi(row->col(0)));
        else
            ret->add(adir);
    }
    return ret;
}

std::shared_ptr<AutoscanDirectory> SQLDatabase::getAutoscanDirectory(int objectID)
{
#define FLD(field) << TQD('a', field) <<
    std::ostringstream q;
    q << "SELECT " FLD("id") ',' FLD("obj_id") ',' FLD("scan_level") ',' FLD("scan_mode") ',' FLD("recursive") ',' FLD("hidden") ',' FLD("interval") ',' FLD("last_modified") ',' FLD("persistent") ',' FLD("location") ',' << TQD('t', "location")
      << " FROM " << TQ(AUTOSCAN_TABLE) << ' ' << TQ('a')
      << " LEFT JOIN " << TQ(CDS_OBJECT_TABLE) << ' ' << TQ('t')
      << " ON " FLD("obj_id") '=' << TQD('t', "id")
      << " WHERE " << TQD('t', "id") << '=' << quote(objectID);
    auto res = select(q);
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
    std::string objectIDstr = row->col(1);
    if (!objectIDstr.empty())
        objectID = std::stoi(objectIDstr);
    int databaseID = std::stoi(row->col(0));

    fs::path location;
    if (objectID == INVALID_OBJECT_ID) {
        location = row->col(9);
    } else {
        char prefix;
        location = stripLocationPrefix(row->col(10), &prefix);
        if (prefix != LOC_DIR_PREFIX)
            return nullptr;
    }

    ScanMode mode = AutoscanDirectory::remapScanmode(row->col(3));
    bool recursive = remapBool(row->col(4));
    bool hidden = remapBool(row->col(5));
    bool persistent = remapBool(row->col(8));
    int interval = 0;
    if (mode == ScanMode::Timed)
        interval = std::stoi(row->col(6));
    auto last_modified = std::chrono::seconds(std::stol(row->col(7)));

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

    std::ostringstream q;
    q << "INSERT INTO " << TQ(AUTOSCAN_TABLE)
      << " (" << TQ("obj_id") << ','
      << TQ("scan_level") << ','
      << TQ("scan_mode") << ','
      << TQ("recursive") << ','
      << TQ("hidden") << ','
      << TQ("interval") << ','
      << TQ("last_modified") << ','
      << TQ("persistent") << ','
      << TQ("location") << ','
      << TQ("path_ids")
      << ") VALUES ("
      << (objectID >= 0 ? quote(objectID) : SQL_NULL) << ','
      << quote("full") << ','
      << quote(AutoscanDirectory::mapScanmode(adir->getScanMode())) << ','
      << mapBool(adir->getRecursive()) << ','
      << mapBool(adir->getHidden()) << ','
      << quote(adir->getInterval().count()) << ','
      << quote(adir->getPreviousLMT().count()) << ','
      << mapBool(adir->persistent()) << ','
      << (objectID >= 0 ? SQL_NULL : quote(adir->getLocation())) << ','
      << (!pathIds ? SQL_NULL : quote("," + toCSV(*pathIds) + ','))
      << ')';
    adir->setDatabaseID(exec(q.str(), true));
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
    std::ostringstream q;
    q << "UPDATE " << TQ(AUTOSCAN_TABLE)
      << " SET " << TQ("obj_id") << '=' << (objectID >= 0 ? quote(objectID) : SQL_NULL)
      << ',' << TQ("scan_level") << '='
      << quote("full")
      << ',' << TQ("scan_mode") << '='
      << quote(AutoscanDirectory::mapScanmode(adir->getScanMode()))
      << ',' << TQ("recursive") << '=' << mapBool(adir->getRecursive())
      << ',' << TQ("hidden") << '=' << mapBool(adir->getHidden())
      << ',' << TQ("interval") << '=' << quote(adir->getInterval().count());
    if (adir->getPreviousLMT() > std::chrono::seconds::zero())
        q << ',' << TQ("last_modified") << '=' << quote(adir->getPreviousLMT().count());
    q << ',' << TQ("persistent") << '=' << mapBool(adir->persistent())
      << ',' << TQ("location") << '=' << (objectID >= 0 ? SQL_NULL : quote(adir->getLocation()))
      << ',' << TQ("path_ids") << '=' << (!pathIds ? SQL_NULL : quote("," + toCSV(*pathIds) + ','))
      << ',' << TQ("touched") << '=' << mapBool(true)
      << " WHERE " << TQ("id") << '=' << quote(adir->getDatabaseID());
    exec(q.str());
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
    std::ostringstream q;
    q << "DELETE FROM " << TQ(AUTOSCAN_TABLE)
      << " WHERE " << TQ("id") << '=' << quote(autoscanID);
    exec(q.str());
    if (objectID != INVALID_OBJECT_ID)
        _autoscanChangePersistentFlag(objectID, false);
}

int SQLDatabase::_getAutoscanObjectID(int autoscanID)
{
    std::ostringstream q;
    q << "SELECT " << TQ("obj_id") << " FROM " << TQ(AUTOSCAN_TABLE)
      << " WHERE " << TQ("id") << '=' << quote(autoscanID)
      << " LIMIT 1";
    auto res = select(q);
    if (!res)
        throw DatabaseException("", "error while doing select on ");
    auto row = res->nextRow();
    if (row && !row->col(0).empty())
        return std::stoi(row->col(0));
    return INVALID_OBJECT_ID;
}

void SQLDatabase::_autoscanChangePersistentFlag(int objectID, bool persistent)
{
    if (objectID == INVALID_OBJECT_ID || objectID == INVALID_OBJECT_ID_2)
        return;

    std::ostringstream q;
    q << "UPDATE " << TQ(CDS_OBJECT_TABLE)
      << " SET " << TQ("flags") << " = (" << TQ("flags")
      << (persistent ? " | " : " & ~")
      << OBJECT_FLAG_PERSISTENT_CONTAINER
      << ") WHERE " << TQ("id") << '=' << quote(objectID);
    exec(q.str());
}

void SQLDatabase::checkOverlappingAutoscans(std::shared_ptr<AutoscanDirectory> adir)
{
    (void)_checkOverlappingAutoscans(adir);
}

std::unique_ptr<std::vector<int>> SQLDatabase::_checkOverlappingAutoscans(const std::shared_ptr<AutoscanDirectory>& adir)
{
    if (!adir)
        throw_std_runtime_error("_checkOverlappingAutoscans called with adir==nullptr");
    int checkObjectID = adir->getObjectID();
    if (checkObjectID == INVALID_OBJECT_ID)
        return nullptr;
    int databaseID = adir->getDatabaseID();

    std::unique_ptr<SQLRow> row;
    {
        std::ostringstream qAs;
        qAs << "SELECT " << TQ("id")
            << " FROM " << TQ(AUTOSCAN_TABLE)
            << " WHERE " << TQ("obj_id") << " = "
            << quote(checkObjectID);
        if (databaseID >= 0)
            qAs << " AND " << TQ("id") << " != " << quote(databaseID);

        auto res = select(qAs);
        if (!res)
            throw_std_runtime_error("SQL error");

        if ((row = res->nextRow())) {
            auto obj = loadObject(checkObjectID);
            if (!obj)
                throw_std_runtime_error("Referenced object (by Autoscan) not found.");
            log_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
            throw_std_runtime_error("There is already an Autoscan set on {}", obj->getLocation().c_str());
        }
    }

    if (adir->getRecursive()) {
        std::ostringstream qRec;
        qRec << "SELECT " << TQ("obj_id")
             << " FROM " << TQ(AUTOSCAN_TABLE)
             << " WHERE " << TQ("path_ids") << " LIKE "
             << quote(fmt::format("%,{},%", checkObjectID));
        if (databaseID >= 0)
            qRec << " AND " << TQ("id") << " != " << quote(databaseID);
        qRec << " LIMIT 1";

        log_debug("------------ {}", qRec.str().c_str());

        auto res = select(qRec);
        if (!res)
            throw_std_runtime_error("SQL error");
        if ((row = res->nextRow())) {
            int objectID = std::stoi(row->col(0));
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
        if (!pathIDs)
            throw_std_runtime_error("getPathIDs returned nullptr");
        std::ostringstream qPath;
        qPath << "SELECT " << TQ("obj_id")
              << " FROM " << TQ(AUTOSCAN_TABLE)
              << " WHERE " << TQ("obj_id") << " IN ("
              << toCSV(*pathIDs)
              << ") AND " << TQ("recursive") << '=' << mapBool(true);
        if (databaseID >= 0)
            qPath << " AND " << TQ("id") << " != " << quote(databaseID);
        qPath << " LIMIT 1";

        auto res = select(qPath);
        if (!res)
            throw_std_runtime_error("SQL error");
        if (!(row = res->nextRow()))
            return pathIDs;
    }

    int objectID = std::stoi(row->col(0));
    auto obj = loadObject(objectID);
    if (!obj) {
        throw_std_runtime_error("Referenced object (by Autoscan) not found.");
    }
    log_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
    throw_std_runtime_error("Overlapping Autoscans are not allowed. There is already a recursive Autoscan set on {}", obj->getLocation().c_str());
}

std::unique_ptr<std::vector<int>> SQLDatabase::getPathIDs(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return nullptr;

    std::ostringstream sel;
    sel << "SELECT " << TQ("parent_id") << " FROM " << TQ(CDS_OBJECT_TABLE) << " WHERE ";
    sel << TQ("id") << '=';

    std::vector<int> pathIDs;
    std::shared_ptr<SQLResult> res;
    std::unique_ptr<SQLRow> row;
    while (objectID != CDS_ID_ROOT) {
        pathIDs.push_back(objectID);
        std::ostringstream q;
        q << sel.str() << quote(objectID) << " LIMIT 1";
        res = select(q);
        if (!res || !(row = res->nextRow()))
            break;
        objectID = std::stoi(row->col(0));
    }
    return std::make_unique<std::vector<int>>(pathIDs);
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
    std::ostringstream qb;
    qb << "UPDATE "
       << TQ(CDS_OBJECT_TABLE)
       << " SET "
       << TQ("flags")
       << " = ("
       << TQ("flags")
       << "&~" << flag
       << ") WHERE "
       << TQ("flags")
       << "&" << flag;
    exec(qb.str());
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
    auto&& res = select(rsql.c_str());

    if (!res)
        return {};

    std::vector<std::shared_ptr<CdsResource>> resources;
    std::unique_ptr<SQLRow> row;
    while ((row = res->nextRow())) {
        auto resource = std::make_shared<CdsResource>(std::stoi(getCol(row, ResourceCol::handlerType)));
        resource->decode(getCol(row, ResourceCol::options), getCol(row, ResourceCol::parameters));
        for (auto&& resAttrId : ResourceAttributeIterator()) {
            auto index = to_underlying(ResourceCol::attributes) + to_underlying(resAttrId);
            auto value = row->col_c_str(index);
            if (value) {
                resource->addAttribute(resAttrId, value);
            }
        }
        resources.push_back(resource);
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

std::unique_ptr<std::ostringstream> SQLDatabase::sqlForInsert(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::ostringstream fields;
    std::ostringstream values;

    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin()) {
            fields << ',';
            values << ',';
        }
        fields << TQ(it->first);
        values << it->second;
    }

    if (tableName == CDS_OBJECT_TABLE && obj->getID() != INVALID_OBJECT_ID) {
        throw_std_runtime_error("Attempted to insert new object with ID!");
    }

    if (tableName == METADATA_TABLE || tableName == RESOURCE_TABLE) {
        fields << "," << TQ("item_id");
        values << "," << obj->getID();
    }

    std::ostringstream qb;
    qb << "INSERT INTO " << TQ(tableName) << " (" << fields.str() << ") VALUES (" << values.str() << ')';

    return std::make_unique<std::ostringstream>(std::move(qb));
}

std::unique_ptr<std::ostringstream> SQLDatabase::sqlForUpdate(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    if (!addUpdateTable
        || (addUpdateTable->getTableName() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw_std_runtime_error("sqlForUpdate called with invalid arguments");

    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::ostringstream qb;
    qb << "UPDATE " << TQ(tableName) << " SET ";
    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin())
            qb << ',';
        qb << TQ(it->first) << '='
           << it->second;
    }

    // relying on only one element when tableName is mt_metadata
    if (tableName == RESOURCE_TABLE) {
        qb << " WHERE " << TQ("item_id") << " = " << obj->getID();
        qb << " AND " << TQ("res_id") << " = " << dict["res_id"];
    } else if (tableName == METADATA_TABLE) {
        qb << " WHERE " << TQ("item_id") << " = " << obj->getID();
        qb << " AND " << TQ("property_name") << " = " << dict.begin()->second;
    } else {
        qb << " WHERE " << TQ("id") << " = " << obj->getID();
    }

    return std::make_unique<std::ostringstream>(std::move(qb));
}

std::unique_ptr<std::ostringstream> SQLDatabase::sqlForDelete(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<AddUpdateTable>& addUpdateTable) const
{
    if (!addUpdateTable
        || (addUpdateTable->getTableName() == METADATA_TABLE && addUpdateTable->getDict().size() != 2))
        throw_std_runtime_error("sqlForDelete called with invalid arguments");

    std::string tableName = addUpdateTable->getTableName();
    auto dict = addUpdateTable->getDict();

    std::ostringstream qb;
    qb << "DELETE FROM " << TQ(tableName);

    // relying on only one element when tableName is mt_metadata
    if (tableName == METADATA_TABLE) {
        qb << " WHERE " << TQ("item_id") << " = " << obj->getID();
        qb << " AND " << TQ("property_name") << " = " << dict.begin()->second;
    } else {
        qb << " WHERE " << TQ("id") << " = " << obj->getID();
    }

    return std::make_unique<std::ostringstream>(std::move(qb));
}

// column metadata is dropped in DBVERSION 12
bool SQLDatabase::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required");
    std::ostringstream qbCountNotNull;
    qbCountNotNull << "SELECT COUNT(*)"
                   << " FROM " << TQ(CDS_OBJECT_TABLE)
                   << " WHERE " << TQ("metadata")
                   << " is not null";
    auto res = select(qbCountNotNull);
    int expectedConversionCount = std::stoi(res->nextRow()->col(0));
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    std::ostringstream qbCountMetadata;
    qbCountMetadata << "SELECT COUNT(*)"
                    << " FROM " << TQ(METADATA_TABLE);
    res = select(qbCountMetadata);
    int metadataRowCount = std::stoi(res->nextRow()->col(0));
    log_debug("mt_metadata rows having metadata: {}", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required");
        return true;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata");

    std::ostringstream qbRetrieveIDs;
    qbRetrieveIDs << "SELECT " << TQ("id") << ", " << TQ("metadata")
                  << " FROM " << TQ(CDS_OBJECT_TABLE)
                  << " WHERE " << TQ("metadata")
                  << " is not null";
    auto resIds = select(qbRetrieveIDs);
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow())) {
        migrateMetadata(std::stoi(row->col(0)), row->col(1));
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

        for (auto&& [key, val] : metadataSQLVals) {
            std::ostringstream fields, values;
            fields << TQ("item_id") << ','
                   << TQ("property_name") << ','
                   << TQ("property_value");
            values << objectId << ','
                   << key << ',' << val;
            std::ostringstream qb;
            qb << "INSERT INTO " << TQ(METADATA_TABLE)
               << " (" << fields.str()
               << ") VALUES (" << values.str() << ')';

            exec(qb.str());
        }
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", objectId);
    }
}

void SQLDatabase::prepareResourceTable(std::string_view addColumnCmd)
{
    auto resourceAttributes = splitString(getInternalSetting("resource_attribute"), ',');
    bool addedAttribute = false;
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto&& resAttrib = MetadataHandler::getResAttrName(resAttrId);
        if (std::find_if(resourceAttributes.begin(), resourceAttributes.end(), [&](auto&& attr) { return attr == resAttrib; }) == resourceAttributes.end()) {
            _exec(fmt::format(addColumnCmd, resAttrib).c_str());
            resourceAttributes.push_back(resAttrib);
            addedAttribute = true;
        }
    }
    if (addedAttribute)
        storeInternalSetting("resource_attribute", join(resourceAttributes, ','));
}

// column resources is dropped in DBVERSION 13
bool SQLDatabase::doResourceMigration()
{
    if (!addResourceColumnCmd.empty())
        prepareResourceTable(addResourceColumnCmd);

    log_debug("Checking if resources migration is required");
    auto res = select(
        fmt::format("SELECT COUNT(*) FROM {0}{2}{1} WHERE {0}resources{1} is not null",
            table_quote_begin, table_quote_end, CDS_OBJECT_TABLE)
            .c_str());
    int expectedConversionCount = std::stoi(res->nextRow()->col(0));
    log_debug("{} rows having resources: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    res = select(
        fmt::format("SELECT COUNT(*) FROM {0}{2}{1}",
            table_quote_begin, table_quote_end, RESOURCE_TABLE)
            .c_str());
    int resourceRowCount = std::stoi(res->nextRow()->col(0));
    log_debug("{} rows having entries: {}", RESOURCE_TABLE, resourceRowCount);

    if (expectedConversionCount > 0 && resourceRowCount > 0) {
        log_info("No resources migration required");
        return true;
    }

    log_info("About to migrate resources from {} to {}", CDS_OBJECT_TABLE, RESOURCE_TABLE);

    auto resIds = select(
        fmt::format("SELECT {0}id{1}, {0}resources{1} FROM {0}{2}{1} WHERE {0}resources{1} is not null",
            table_quote_begin, table_quote_end, CDS_OBJECT_TABLE)
            .c_str());
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow())) {
        migrateResources(std::stoi(row->col(0)), row->col(1));
        ++objectsUpdated;
    }
    log_info("Migrated resources - object count: {}", objectsUpdated);
    return true;
}

void SQLDatabase::migrateResources(int objectId, const std::string& resourcesStr)
{
    if (!resourcesStr.empty()) {
        log_debug("Migrating resources for cds object {}", objectId);
        std::vector<std::string> resources = splitString(resourcesStr, RESOURCE_SEP);
        size_t res_id = 0;
        for (auto&& resString : resources) {
            std::map<std::string, std::string> resourceSQLVals;
            auto&& resource = CdsResource::decode(resString);
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceSQLVals[key] = quote(val);
            }
            std::ostringstream fields, values;
            fields << TQ("item_id") << ", " << TQ("res_id") << ", " << TQ("handlerType");
            values << objectId << ", " << res_id << ", " << resource->getHandlerType();
            auto options = resource->getOptions();
            if (!options.empty()) {
                fields << ", " << TQ("options");
                values << ", " << quote(dictEncode(options));
            }
            auto parameters = resource->getParameters();
            if (!parameters.empty()) {
                fields << ", " << TQ("parameters");
                values << ", " << quote(dictEncode(parameters));
            }
            for (auto&& [key, val] : resourceSQLVals) {
                fields << ',' << TQ(key);
                values << ',' << val;
            }
            exec(fmt::format("INSERT INTO {}{}{} ({}) VALUES ({})", table_quote_begin, RESOURCE_TABLE, table_quote_end, fields.str(), values.str()).c_str());
            res_id++;
        }
    } else {
        log_debug("Skipping migration - no resources for cds object {}", objectId);
    }
}

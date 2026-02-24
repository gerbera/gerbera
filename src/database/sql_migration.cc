/*GRB*
  Gerbera - https://gerbera.io/

  sql_migration.cc - this file is part of Gerbera.

  Copyright (C) 2026 Gerbera Contributors

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

/// @file database/sql_migration.cc
#define GRB_LOG_FAC GrbLogFacility::sqldatabase

#include "sql_migration.h"

#include "cds/cds_objects.h"
#include "search_handler.h"
#include "sql_result.h"
#include "sql_table.h"
#include "upnp/xml_builder.h"
#include "util/logger.h"
#include "util/url_utils.h"

#include <fmt/core.h>
#include <pugixml.hpp>

void SQLMigration::upgradeDatabase(
    unsigned int dbVersion,
    const std::array<unsigned int, DBVERSION + 1>& hashies,
    const fs::path& upgradeFile,
    std::string_view updateVersionCommand,
    std::size_t firstDBVersion,
    unsigned int stringLimit)
{
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

    std::size_t version = firstDBVersion;
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        auto versionNode = versionElement.node();
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
        throw DatabaseException(
            fmt::format("The database upgrade file {} seems to be from another Gerbera version. Expected {}, actual {}, found {}",
                upgradeFile.c_str(), DBVERSION, dbVersion, version),
            LINE_MESSAGE);

    version = firstDBVersion;
    static const std::map<std::string, bool (SQLMigration::*)()> migActions {
        { "metadata", &SQLMigration::doMetadataMigration },
        { "resources", &SQLMigration::doResourceMigration },
        { "location", &SQLMigration::doLocationMigration },
    };

    /* --- run database upgrades --- */
    for (auto&& upgrade : dbUpdates) {
        if (dbVersion == version) {
            log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
            for (auto&& [migrationCmd, upgradeCmd] : upgrade) {
                bool actionResult = true;
                replaceAllString(upgradeCmd, STRING_LIMIT, fmt::to_string(stringLimit));
                if (!migrationCmd.empty() && migActions.find(migrationCmd) != migActions.end())
                    actionResult = (*this.*(migActions.at(migrationCmd)))();
                if (actionResult && !upgradeCmd.empty())
                    database->_exec(upgradeCmd);
            }
            log_debug("upgrade {}", fmt::format(updateVersionCommand, version + 1, version));
            database->_exec(fmt::format(updateVersionCommand, version + 1, version));
            dbVersion = version + 1;
            log_info("Database upgrade to version {} successful.", dbVersion);
        }
        version++;
    }

    if (dbVersion != DBVERSION)
        throw DatabaseException(fmt::format("The database seems to be from another Gerbera version. Expected {}, actual {}", DBVERSION, dbVersion), LINE_MESSAGE);

    prepareResourceTable();
}

// column metadata is dropped in DBVERSION 12
bool SQLMigration::doMetadataMigration()
{
    log_debug("Checking if metadata migration is required");
    auto res = database->select(fmt::format("SELECT COUNT(*) FROM {} WHERE {} IS NOT NULL",
        browseColumnMapper->getTableName(),
        database->identifier("metadata"))); // metadata column is removed!
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_cds_object rows having metadata: {}", expectedConversionCount);

    res = database->select(fmt::format("SELECT COUNT(*) FROM {}", metaColumnMapper->getTableName()));
    int metadataRowCount = res->nextRow()->col_int(0, 0);
    log_debug("mt_metadata rows having metadata: {}", metadataRowCount);

    if (expectedConversionCount > 0 && metadataRowCount > 0) {
        log_info("No metadata migration required");
        return true;
    }

    log_info("About to migrate metadata from mt_cds_object to mt_metadata");

    auto resIds = database->select(fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
        browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
        database->identifier("metadata"),
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

void SQLMigration::migrateMetadata(int objectId, const std::string& metadataStr)
{
    std::map<std::string, std::string> itemMetadata = URLUtils::dictDecode(metadataStr);

    if (!itemMetadata.empty()) {
        log_debug("Migrating metadata for cds object {}", objectId);
        std::vector<std::map<MetadataColumn, std::string>> multiDict;
        multiDict.reserve(itemMetadata.size());
        for (auto&& [key, val] : itemMetadata) {
            auto mDict = std::map<MetadataColumn, std::string> {
                { MetadataColumn::ItemId, database->quote(objectId) },
                { MetadataColumn::PropertyName, database->quote(key) },
                { MetadataColumn::PropertyValue, database->quote(val) },
            };
            multiDict.push_back(std::move(mDict));
        }
        Metadata2Table mt(std::move(multiDict), metaColumnMapper);
        database->execOnTable(METADATA_TABLE, mt.sqlForMultiInsert(nullptr), objectId);
    } else {
        log_debug("Skipping migration - no metadata for cds object {}", objectId);
    }
}

void SQLMigration::prepareResourceTable()
{
    auto resourceAttributes = splitString(database->getInternalSetting("resource_attribute"), ',');
    bool addedAttribute = false;
    for (auto&& resAttrId : ResourceAttributeIterator()) {
        auto&& resAttrib = EnumMapper::getAttributeName(resAttrId);
        if (std::find(resourceAttributes.begin(), resourceAttributes.end(), resAttrib) == resourceAttributes.end()) {
            auto addColumnCmd = addResourceColumnCmd.at(EnumMapper::getAttributeType(resAttrId));
            database->_exec(fmt::format(addColumnCmd, resAttrib));
            log_info("'{}': Adding column '{}'", RESOURCE_TABLE, resAttrib);
            resourceAttributes.push_back(resAttrib);
            addedAttribute = true;
        }
    }
    if (addedAttribute)
        database->storeInternalSetting("resource_attribute", fmt::format("{}", fmt::join(resourceAttributes, ",")));
}

// column resources is dropped in DBVERSION 13
bool SQLMigration::doResourceMigration()
{
    if (!addResourceColumnCmd.empty())
        prepareResourceTable();

    log_debug("Checking if resources migration is required");
    auto res = database->select(
        fmt::format("SELECT COUNT(*) FROM {} WHERE {} IS NOT NULL",
            browseColumnMapper->getTableName(),
            database->identifier("resources"))); // resources column is removed!
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having resources: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    res = database->select(
        fmt::format("SELECT COUNT(*) FROM {}", resColumnMapper->getTableName()));
    int resourceRowCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having entries: {}", RESOURCE_TABLE, resourceRowCount);

    if (expectedConversionCount > 0 && resourceRowCount > 0) {
        log_info("No resources migration required");
        return true;
    }

    log_info("About to migrate resources from {} to {}", CDS_OBJECT_TABLE, RESOURCE_TABLE);

    auto resIds = database->select(
        fmt::format("SELECT {0}, {1} FROM {2} WHERE {1} IS NOT NULL",
            browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
            database->identifier("resources"),
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

void SQLMigration::migrateResources(int objectId, const std::string& resourcesStr)
{
    if (!resourcesStr.empty()) {
        log_debug("Migrating resources for cds object {}", objectId);
        auto resources = splitString(resourcesStr, RESOURCE_SEP);
        std::size_t resId = 0;
        for (auto&& resString : resources) {
            std::map<ResourceAttribute, std::string> resourceVals;
            auto&& resource = CdsResource::decode(resString);
            for (auto&& [key, val] : resource->getAttributes()) {
                resourceVals[key] = database->quote(val);
            }
            auto dict = std::map<ResourceColumn, std::string> {
                { ResourceColumn::ItemId, database->quote(objectId) },
                { ResourceColumn::ResId, database->quote(resId) },
                { ResourceColumn::HandlerType, database->quote(to_underlying(resource->getHandlerType())) },
                { ResourceColumn::Purpose, database->quote(to_underlying(resource->getPurpose())) },
            };
            auto&& options = resource->getOptions();
            if (!options.empty()) {
                dict[ResourceColumn::Options] = database->quote(URLUtils::dictEncode(options));
            }
            auto&& parameters = resource->getParameters();
            if (!parameters.empty()) {
                dict[ResourceColumn::Parameters] = database->quote(URLUtils::dictEncode(parameters));
            }
            Resource2Table rt(std::move(dict), std::move(resourceVals), Operation::Insert, resColumnMapper);
            database->execOnTable(RESOURCE_TABLE, rt.sqlForInsert(nullptr), objectId);
            resId++;
        }
    } else {
        log_debug("Skipping migration - no resources for cds object {}", objectId);
    }
}

// PREFIX is removed from location in DBVERSION 27
bool SQLMigration::doLocationMigration()
{
    log_debug("Checking if resources migration is required");
    auto res = database->select(
        fmt::format("SELECT COUNT(*) FROM {0} WHERE {1} LIKE {2} OR {1} LIKE {3} OR {1} LIKE {4}",
            browseColumnMapper->getTableName(),
            browseColumnMapper->mapQuoted(BrowseColumn::Location, true),
            database->quote("F%"),
            database->quote("D%"),
            database->quote("V%")));
    int expectedConversionCount = res->nextRow()->col_int(0, 0);
    log_debug("{} rows having location: {}", CDS_OBJECT_TABLE, expectedConversionCount);

    log_info("About to migrate location values in {}", CDS_OBJECT_TABLE);

    auto resIds = database->select(
        fmt::format("SELECT {0}, {1}, {2} FROM {3} WHERE {1} LIKE {4} OR {1} LIKE {5} OR {1} LIKE {6}",
            browseColumnMapper->mapQuoted(BrowseColumn::Id, true),
            browseColumnMapper->mapQuoted(BrowseColumn::Location, true),
            browseColumnMapper->mapQuoted(BrowseColumn::LocationHash, true),
            browseColumnMapper->getTableName(),
            database->quote("F%"),
            database->quote("D%"),
            database->quote("V%")));
    std::unique_ptr<SQLRow> row;

    int objectsUpdated = 0;
    while ((row = resIds->nextRow())) {
        migrateLocation(row->col_int(0, INVALID_OBJECT_ID), row->col(1));
        ++objectsUpdated;
    }
    log_info("Migrated location - object count: {}", objectsUpdated);
    return true;
}

void SQLMigration::migrateLocation(int objectId, const std::string& location)
{
    if (!location.empty()) {
        auto dbLocation = location.substr(1);
        log_debug("Migrating location for cds object {} to {} without {}", objectId, dbLocation, location.at(0));
        auto dict = std::map<BrowseColumn, std::string> {
            { BrowseColumn::Id, database->quote(objectId) },
            { BrowseColumn::Location, database->quote(dbLocation) },
            { BrowseColumn::LocationHash, database->quote(stringHash(dbLocation)) },
        };
        Object2Table ot(std::move(dict), Operation::Update, browseColumnMapper);
        database->execOnTable(CDS_OBJECT_TABLE, ot.sqlForUpdate(nullptr), objectId);
    } else {
        log_debug("Skipping migration - no location for cds object {}", objectId);
    }
}
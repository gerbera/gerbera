/*GRB*
  Gerbera - https://gerbera.io/

  sql_migration.h - this file is part of Gerbera.

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

/// @file database/sql_migration.h

#ifndef __SQL_MIGRATION_H__
#define __SQL_MIGRATION_H__

#include "sql_database.h"

template <class Col>
class EnumColumnMapper;
enum class BrowseColumn;
enum class MetadataColumn;
enum class ResourceColumn;
enum class ResourceDataType;

#define RESOURCE_SEP '|'

// @brief utility class for database migrations
class SQLMigration {
private:
    /// @brief Back reference to database
    std::shared_ptr<SQLDatabase> database;
    /// @brief handling of cds_object columns
    std::shared_ptr<EnumColumnMapper<BrowseColumn>> browseColumnMapper;
    /// @brief handling of meta_data columns
    std::shared_ptr<EnumColumnMapper<MetadataColumn>> metaColumnMapper;
    /// @brief handling of resource columns
    std::shared_ptr<EnumColumnMapper<ResourceColumn>> resColumnMapper;
    /// @brief generator commands for resources
    std::map<ResourceDataType, std::string_view> addResourceColumnCmd;

public:
    /// @brief constructor
    SQLMigration(
        std::shared_ptr<SQLDatabase> database,
        std::shared_ptr<EnumColumnMapper<BrowseColumn>> browseColumnMapper,
        std::shared_ptr<EnumColumnMapper<MetadataColumn>> metaColumnMapper,
        std::shared_ptr<EnumColumnMapper<ResourceColumn>> resColumnMapper,
        std::map<ResourceDataType, std::string_view> addResourceColumnCmd)
        : database(std::move(database))
        , browseColumnMapper(std::move(browseColumnMapper))
        , metaColumnMapper(std::move(metaColumnMapper))
        , resColumnMapper(std::move(resColumnMapper))
        , addResourceColumnCmd(std::move(addResourceColumnCmd))
    {
    }

    /// @brief upgrade database version by applying migration commands
    void upgradeDatabase(
        unsigned int dbVersion,
        const std::array<unsigned int, DBVERSION + 1>& hashies,
        const fs::path& upgradeFile,
        std::string_view updateVersionCommand,
        std::size_t firstDBVersion,
        unsigned int stringLimit);

private:
    /// @brief migrate metadata from mt_cds_objects to mt_metadata before removing the column (DBVERSION 12)
    bool doMetadataMigration();

    /// @brief migrate metadata for cdsObject
    void migrateMetadata(int objectId, const std::string& metadataStr);

    /// @brief Add a column to resource table for each defined resource attribute
    void prepareResourceTable();

    /// @brief migrate resources from mt_cds_objects to grb_resource before removing the column (DBVERSION 13)
    bool doResourceMigration();

    /// @brief migrate resource data for cdsObject
    void migrateResources(int objectId, const std::string& resourcesStr);

    /// @brief drop prefix from location
    bool doLocationMigration();

    /// @brief migrate a location for cdsObject
    void migrateLocation(int objectId, const std::string& location);
};

#endif
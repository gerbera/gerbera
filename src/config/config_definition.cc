/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

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

/// @file config/config_definition.cc
/// @brief Definitions of default values and setup for configuration.
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_definition.h" // API

#include "cds/cds_enums.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/result/client_config.h"
#include "config/result/transcoding.h"
#include "config/setup/config_setup_array.h"
#include "config/setup/config_setup_autoscan.h"
#include "config/setup/config_setup_bool.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_client.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_dynamic.h"
#include "config/setup/config_setup_enum.h"
#include "config/setup/config_setup_int.h"
#include "config/setup/config_setup_path.h"
#include "config/setup/config_setup_string.h"
#include "config/setup/config_setup_time.h"
#include "config/setup/config_setup_transcoding.h"
#include "config/setup/config_setup_tweak.h"
#include "config/setup/config_setup_vector.h"
#include "config_options.h"
#include "config_setup.h"
#include "config_val.h"
#include "content/content.h"
#include "database/sqlite3/sqlite_config.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"

#include <algorithm>
#include <upnpconfig.h>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

// default values

#define DEFAULT_FOLLOW_SYMLINKS_VALUE YES
#define DEFAULT_RESOURCES_CASE_SENSITIVE YES
#define DEFAULT_UPNP_STRING_LIMIT (-1)

#define DEFAULT_LIBOPTS_ENTRY_SEPARATOR "; "

/// @brief Options listed here will be reported as deprecated when used
static const std::vector<ConfigVal> deprecatedOptions {};

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getServerOptions()
{
    /// @brief default values for ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS
    static const std::map<std::string, std::string> upnpContainerDefaultPropDefaults {
        { "@childContainerCount", "0" },
        { "@childCount", "0" },
        { "@id", "0" },
        { "@parentID", "0" },
        { "@restricted", "1" },
        { "@searchable", "1" },
        { "upnp:storageFree", "0" },
        { "upnp:storageMaxPartition", "0" },
        { "upnp:storageMedium", "UNKNOWN" },
        { "upnp:storageTotal", "0" },
        { "upnp:storageUsed", "0" },
    };

    /// @brief default values for ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS
    static const std::map<std::string, std::string> upnpObjectDefaultPropDefaults {
        { "@id", "0" },
        { "@parentID", "0" },
        { "dc:creator", "UNKNOWN" },
        { "dc:date", "1900-01-01T00:00:00" },
        { "upnp:actor", "UNKNOWN" },
        { "upnp:album", "UNKNOWN" },
        { "upnp:artist", "UNKNOWN" },
        { "upnp:artistDiscographyURI", "" },
        { "upnp:author", "UNKNOWN" },
        { "upnp:contributor", "UNKNOWN" },
        { "upnp:director", "UNKNOWN" },
        { "upnp:genre", "UNKNOWN" },
        { "upnp:language", "en-US" },
        { "upnp:lastPlaybackPosition", "0" },
        { "upnp:lastPlaybackTime", "1900-01-01T00:00:00" },
        { "upnp:lyricsURI", "" },
        { "upnp:originalTrackNumber", "0" },
        { "upnp:playbackCount", "0" },
        { "upnp:playlist", "UNKNOWN" },
        { "upnp:producer", "UNKNOWN" },
        { "upnp:publisher", "UNKNOWN" },
        { "upnp:rating", "0" },
        { "upnp:recordable", "1" },
        { "upnp:relation", "UNKNOWN" },
    };

    /// @brief default values for ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS
    static const std::map<std::string, std::string> upnpResourceDefaultPropDefaults {
        { "@allowedUse", "UNKNOWN" },
        { "@bitrate", "0" },
        { "@bitsPerSample", "0" },
        { "@colorDepth", "1" },
        { "@contentInfoURI", "" },
        { "@daylightSaving", "UNKNOWN" },
        { "@duration", "0" },
        { "@framerate", "30p" },
        { "@importUri", "" },
        { "@nrAudioChannels", "0" },
        { "@protection", "UNKNOWN" },
        { "@protocolInfo", "http:*:*:*" },
        { "@recordQuality", "" },
        { "@reliability", "100" },
        { "@remainingTime", "" },
        { "@resolution", "1x1" },
        { "@rightsInfoURI", "" },
        { "@sampleFrequency", "0" },
        { "@size", "0" },
        { "@tspec", "UNKNOWN" },
        { "@usageInfo", "This file is hosted by Gerbera" },
        { "@validityEnd", "1900-01-01T00:00:00" },
        { "@validityStart", "1900-01-01T00:00:00" },
    };

    /// @brief default values for ConfigVal::UPNP_SEARCH_SEGMENTS
    static const std::vector<std::string> upnpSearchSegmentDefaults {
        "M_ARTIST",
        "M_TITLE",
    };

    /// @brief default values for ConfigVal::UPNP_ALBUM_PROPERTIES
    static const std::map<std::string, std::string> upnpAlbumPropDefaults {
        { "dc:creator", "M_ALBUMARTIST" },
        { UPNP_SEARCH_ARTIST, "M_ALBUMARTIST" },
        { "upnp:albumArtist", "M_ALBUMARTIST" },
        { "upnp:composer", "M_COMPOSER" },
        { "upnp:conductor", "M_CONDUCTOR" },
        { "upnp:orchestra", "M_ORCHESTRA" },
        { "upnp:date", "M_UPNP_DATE" },
        { DC_DATE, "M_UPNP_DATE" },
        { "upnp:producer", "M_PRODUCER" },
        { "dc:publisher", "M_PUBLISHER" },
        { UPNP_SEARCH_GENRE, "M_GENRE" },
    };

    /// @brief default values for ConfigVal::UPNP_ARTIST_PROPERTIES
    static const std::map<std::string, std::string> upnpArtistPropDefaults {
        { UPNP_SEARCH_ARTIST, "M_ALBUMARTIST" },
        { "upnp:albumArtist", "M_ALBUMARTIST" },
        { UPNP_SEARCH_GENRE, "M_GENRE" },
    };

    /// @brief default values for ConfigVal::UPNP_GENRE_PROPERTIES
    static const std::map<std::string, std::string> upnpGenrePropDefaults {
        { UPNP_SEARCH_GENRE, "M_GENRE" },
    };

    /// @brief default values for ConfigVal::UPNP_PLAYLIST_PROPERTIES
    static const std::map<std::string, std::string> upnpPlaylistPropDefaults {
        { DC_DATE, "M_UPNP_DATE" },
    };

    /// @brief default values for ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING
    static const std::map<std::string, std::string> uiExtMtDefaults {
        { "css", "text/css" },
        { "html", "text/html" },
        { "js", "application/javascript" },
        { "json", "application/json" },
    };

    /// @brief default values for ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN
    static const std::vector<std::string> defaultItemsPerPage {
        "10",
        "25",
        "50",
        "100",
    };

    return {
        // Core Server options
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_PORT,
            "/server/port", "config-server.html#confval-port",
            0, CheckPortValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_IP,
            "/server/ip", "config-server.html#confval-ip",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_NETWORK_INTERFACE,
            "/server/interface", "config-server.html#confval-interface",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_NAME,
            "/server/name", "config-server.html#confval-server-name",
            "Gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MANUFACTURER,
            "/server/manufacturer", "config-server.html#confval-manufacturer",
            "Gerbera Contributors"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MANUFACTURER_URL,
            "/server/manufacturerURL", "config-server.html#confval-manufacturerURL",
            DESC_MANUFACTURER_URL),
        std::make_shared<ConfigStringSetup>(ConfigVal::VIRTUAL_URL,
            "/server/virtualURL", "config-server.html#confval-virtualURL",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::EXTERNAL_URL,
            "/server/externalURL", "config-server.html#confval-externalURL",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_NAME,
            "/server/modelName", "config-server.html#confval-modelName",
            "Gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_DESCRIPTION,
            "/server/modelDescription", "config-server.html#modeldescription",
            "Free UPnP AV MediaServer, GNU GPL"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_NUMBER,
            "/server/modelNumber", "config-server.html#confval-modelNumber",
            GERBERA_VERSION),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_MODEL_URL,
            "/server/modelURL", "config-server.html#confval-modelURL",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_SERIAL_NUMBER,
            "/server/serialNumber", "config-server.html#confval-serialNumber",
            "1"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_PRESENTATION_URL,
            "/server/presentationURL", "config-server.html#confval-presentationURL", ""),
        std::make_shared<ConfigEnumSetup<UrlAppendMode>>(ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO,
            "/server/presentationURL/attribute::append-to", "config-server.html#confval-append-to",
            UrlAppendMode::none,
            std::map<std::string, UrlAppendMode>({ { "none", UrlAppendMode::none }, { "ip", UrlAppendMode::ip }, { "port", UrlAppendMode::port } })),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UDN,
            "/server/udn", "config-server.html#confval-udn", GRB_UDN_AUTO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_HOME,
            "/server/home", "config-server.html#confval-home"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_HOME_OVERRIDE,
            "/server/home/attribute::override", "config-server.html#confval-override",
            NO),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_TMPDIR,
            "/server/tmpdir", "config-server.html#confval-tmpdir",
            "/tmp/"),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_WEBROOT,
            "/server/webroot", "config-server.html#confval-webroot"),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_ALIVE_INTERVAL,
            "/server/alive", "config-server.html#confval-alive",
            180, ALIVE_INTERVAL_MIN, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_HIDE_PC_DIRECTORY,
            "/server/pc-directory/attribute::upnp-hide", "config-server.html#confval-upnp-hide",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_HIDE_PC_DIRECTORY_WEB,
            "/server/pc-directory/attribute::web-hide", "config-server.html#confval-web-hide",
            NO),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_BOOKMARK_FILE,
            "/server/bookmark", "config-server.html#confval-bookmark",
            "gerbera.html", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
            "/server/upnp-string-limit", "config-server.html#confval-upnp-string-limit",
            DEFAULT_UPNP_STRING_LIMIT, CheckUpnpStringLimitValue),

        // Database setup
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE,
            "/server/storage", "config-server.html#storage",
            true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS,
            "/server/storage/attribute::use-transactions", "config-server.html#confval-use-transactions",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SORT_KEY_ENABLED,
            "/server/storage/attribute::enable-sort-key", "config-server.html#confval-enable-sort-key",
            YES),
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_STORAGE_STRING_LIMIT,
            "/server/storage/attribute::string-limit", "config-server.html#confval-string-limit",
            255),

        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_DRIVER,
            "/server/storage/driver", "config-server.html#storage"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_SQLITE,
            "/server/storage/sqlite3", "config-server.html#confval-sqlite3"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_ENABLED,
            "/server/storage/sqlite3/attribute::enabled", "config-server.html#confval-sqlite3-enabled",
            YES),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_SQLITE_SHUTDOWN_ATTEMPTS,
            "/server/storage/sqlite3/attribute::shutdown-attempts", "config-server.html#confval-shutdown-attempts",
            5, 2, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE,
            "/server/storage/sqlite3/database-file", "config-server.html#confval-database-file",
            "gerbera.db", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS,
            "/server/storage/sqlite3/synchronous", "config-server.html#confval-synchronous",
            "off", SqliteConfig::CheckSqlLiteSyncValue, SqliteConfig::PrintSqlLiteSyncValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE,
            "/server/storage/sqlite3/journal-mode", "config-server.html#confval-journal-mode",
            "WAL", StringCheckFunction(SqliteConfig::CheckSqlJournalMode)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_RESTORE,
            "/server/storage/sqlite3/on-error", "config-server.html#confval-on-error",
            "restore", StringCheckFunction(SqliteConfig::CheckSqlLiteRestoreValue)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
            "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#confval-backup-enabled",
            YES),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
            "/server/storage/sqlite3/backup/attribute::interval", "config-server.html#confval-backup-interval",
            GrbTimeType::Seconds, 600, 1),

        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE,
            "/server/storage/sqlite3/init-sql-file", "config-server.html#confval-sqlite3-init-sql-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / sqlite3.sql"
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE,
            "/server/storage/sqlite3/upgrade-file", "config-server.html#confval-sqlite3-upgrade-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE,
            "/server/storage/sqlite3/drop-file", "config-server.html#confval-sqlite3-drop-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),

        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL,
            "/server/storage/mysql", "config-server.html#confval-mysql"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED,
            "/server/storage/mysql/attribute::enabled", "config-server.html#confval-mysql-enabled",
            NO),
#ifdef HAVE_MYSQL
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_HOST,
            "/server/storage/mysql/host", "config-server.html#confval-mysql-host",
            "localhost"),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_MYSQL_PORT,
            "/server/storage/mysql/port", "config-server.html#confval-mysql-port",
            0, CheckPortValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_USERNAME,
            "/server/storage/mysql/username", "config-server.html#confval-mysql-username",
            "gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_SOCKET,
            "/server/storage/mysql/socket", "config-server.html#confval-mysql-socket",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD,
            "/server/storage/mysql/password", "config-server.html#confval-mysql-password",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_DATABASE,
            "/server/storage/mysql/database", "config-server.html#confval-mysql-database",
            "gerbera"),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE,
            "/server/storage/mysql/init-sql-file", "config-server.html#confval-mysql-init-sql-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / mysql.sql"
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE,
            "/server/storage/mysql/upgrade-file", "config-server.html#confval-mysql-upgrade-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE,
            "/server/storage/mysql/drop-file", "config-server.html#confval-mysql-drop-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_ENGINE,
            "/server/storage/mysql/engine", "config-server.html#confval-mysql-engine",
            "MyISAM"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_CHARSET,
            "/server/storage/mysql/charset", "config-server.html#confval-mysql-charset",
            "utf8"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_MYSQL_COLLATION,
            "/server/storage/mysql/collation", "config-server.html#confval-mysql-collation",
            "utf8_general_ci"),
#endif

        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL,
            "/server/storage/postgres", "config-server.html#confval-postgres"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_STORAGE_PGSQL_ENABLED,
            "/server/storage/postgres/attribute::enabled", "config-server.html#confval-postgres-enabled",
            NO),
#ifdef HAVE_PGSQL
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL_HOST,
            "/server/storage/postgres/host", "config-server.html#confval-postgres-host",
            "localhost"),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_STORAGE_PGSQL_PORT,
            "/server/storage/postgres/port", "config-server.html#confval-postgres-port",
            0, CheckPortValue),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL_USERNAME,
            "/server/storage/postgres/username", "config-server.html#confval-postgres-username",
            "gerbera"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL_SOCKET,
            "/server/storage/postgres/socket", "config-server.html#confval-postgres-socket",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL_PASSWORD,
            "/server/storage/postgres/password", "config-server.html#confval-postgres-password",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_STORAGE_PGSQL_DATABASE,
            "/server/storage/postgres/database", "config-server.html#confval-postgres-database",
            "gerbera"),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE,
            "/server/storage/postgres/init-sql-file", "config-server.html#confval-postgres-init-sql-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / postgres.sql"
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE,
            "/server/storage/postgres/upgrade-file", "config-server.html#confval-postgres-upgrade-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE,
            "/server/storage/postgres/drop-file", "config-server.html#confval-postgres-drop-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
#endif

        // Web User Interface
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLED,
            "/server/ui/attribute::enabled", "config-server.html#confval-ui-enabled",
            YES),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_UI_POLL_INTERVAL,
            "/server/ui/attribute::poll-interval", "config-server.html#confval-poll-intervall",
            GrbTimeType::Seconds, 2, 1),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_POLL_WHEN_IDLE,
            "/server/ui/attribute::poll-when-idle", "config-server.html#confval-poll-when-idle",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_SHOW_TOOLTIPS,
            "/server/ui/attribute::show-tooltips", "config-server.html#confval-show-tooltips",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_NUMBERING,
            "/server/ui/attribute::show-numbering", "config-server.html#confval-show-numbering",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_THUMBNAIL,
            "/server/ui/attribute::show-thumbnail", "config-server.html#confval-show-thumbnail",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ENABLE_VIDEO,
            "/server/ui/attribute::show-video", "config-server.html#confval-enable-video",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_FS_SUPPORT_ADD_ITEM,
            "/server/ui/attribute::fs-add-item", "config-server.html#confval-fs-add-item",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_EDIT_SORTKEY,
            "/server/ui/attribute::edit-sortkey", "config-server.html#confval-edit-sort-key",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED,
            "/server/ui/accounts/attribute::enabled", "config-server.html#confval-accounts-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::SERVER_UI_ACCOUNT_LIST,
            "/server/ui/accounts", "config-server.html#confval-accounts",
            ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD),
        std::make_shared<ConfigTimeSetup>(ConfigVal::SERVER_UI_SESSION_TIMEOUT,
            "/server/ui/accounts/attribute::session-timeout", "config-server.html#confval-session-timeout",
            GrbTimeType::Minutes, 30, 1),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
            "/server/ui/items-per-page/attribute::default", "config-server.html#confval-items-per-page-default",
            25, 1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigArraySetup>(ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
            "/server/ui/items-per-page", "config-server.html#confval-items-per-page-option",
            ConfigVal::A_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, ConfigArraySetup::InitItemsPerPage, true,
            defaultItemsPerPage),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_CONTENT_SECURITY_POLICY,
            "/server/ui/content-security-policy", "config-server.html#confval-content-security-policy",
            "default-src %HOSTS% 'unsafe-eval' 'unsafe-inline'; img-src *; media-src *; child-src 'none';",
            ConfigStringSetup::MergeContentSecurityPolicy),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
            "/server/ui/extension-mimetype", "config-import.html#confval-ui-extension-mimetype",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, uiExtMtDefaults),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_DEFAULT,
            "/server/ui/extension-mimetype/attribute::default", "config-server.html#confval-ui-extension-mimetype-default",
            "application/octet-stream"),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_DOCUMENTATION_SOURCE,
            "/server/ui/source-docs-link", "config-server.html#confval-source-docs-link",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_UI_DOCUMENTATION_USER,
            "/server/ui/user-docs-link", "config-server.html#confval-user-docs-link",
#ifdef GERBERA_RELEASE
            "https://docs.gerbera.io/en/stable/"
#else
            "https://docs.gerbera.io/en/latest/"
#endif
            ),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD,
            "attribute::password", "config-server.html#confval-account-password",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER,
            "attribute::user", "config-server.html#confval-account-user",
            ""),

    // Logging and debugging
#ifdef GRBDEBUG
        std::make_shared<ConfigULongSetup>(ConfigVal::SERVER_LOG_DEBUG_MODE,
            "/server/attribute::debug-mode", "config-server.html#confval-debug-mode",
            0, GrbLogger::makeFacility, GrbLogger::printFacility),
#endif
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_LOG_ROTATE_SIZE,
            "/server/logging/attribute::rotate-file-size", "config-server.html#confval-rotate-file-size",
            1024 * 1024 * 5),
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_LOG_ROTATE_COUNT,
            "/server/logging/attribute::rotate-file-count", "config-server.html#confval-rotate-file-count",
            10),
#ifdef UPNP_HAVE_TOOLS
        std::make_shared<ConfigUIntSetup>(ConfigVal::SERVER_UPNP_MAXJOBS,
            "/server/attribute::upnp-max-jobs", "config-server.html#confval-upnp-max-jobs",
            500),
#endif

        // UPNP control
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_LITERAL_HOST_REDIRECTION,
            "/server/upnp/attribute::literal-host-redirection", "config-server.html#confval-literal-host-redirection",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_DYNAMIC_DESCRIPTION,
            "/server/upnp/attribute::dynamic-descriptions", "config-server.html#confval-dynamic-descriptions",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_MULTI_VALUES_ENABLED,
            "/server/upnp/attribute::multi-value", "config-server.html#confval-multi-value",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_SEARCH_CONTAINER_FLAG,
            "/server/upnp/attribute::searchable-container-flag", "config-server.html#confval-searchable-container-flag",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::UPNP_SEARCH_SEPARATOR,
            "/server/upnp/attribute::search-result-separator", "config-server.html#confval-search-result-separator",
            " - "),
        std::make_shared<ConfigBoolSetup>(ConfigVal::UPNP_SEARCH_FILENAME,
            "/server/upnp/attribute::search-filename", "config-server.html#confval-search-filename",
            NO),
        std::make_shared<ConfigIntSetup>(ConfigVal::UPNP_CAPTION_COUNT,
            "/server/upnp/attribute::caption-info-count", "config-server.html#confval-caption-info-count",
            -1, -1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigArraySetup>(ConfigVal::UPNP_SEARCH_ITEM_SEGMENTS,
            "/server/upnp/search-item-result", "config-server.html#confval-search-item-result",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA, ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false, upnpSearchSegmentDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ALBUM_PROPERTIES,
            "/server/upnp/album-properties", "config-server.html#confval-album-properties",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpAlbumPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ARTIST_PROPERTIES,
            "/server/upnp/artist-properties", "config-server.html#confval-artist-properties",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpArtistPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_GENRE_PROPERTIES,
            "/server/upnp/genre-properties", "config-server.html#confval-genre-properties",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpGenrePropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_PLAYLIST_PROPERTIES,
            "/server/upnp/playlist-properties", "config-server.html#confval-playlist-properties",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, true, upnpPlaylistPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_TITLE_PROPERTIES,
            "/server/upnp/title-properties", "config-server.html#confval-title-properties",
            ConfigVal::A_UPNP_PROPERTIES_PROPERTY, ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, ConfigVal::A_UPNP_PROPERTIES_METADATA,
            false, false, false),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_PROPERTIES_PROPERTY,
            "upnp-property", "config-server.html#confval-upnp-property"),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_NAMESPACE_PROPERTY,
            "upnp-namespace", "config-server.html#confval-upnp-namespace"),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ALBUM_NAMESPACES,
            "/server/upnp/album-properties", "config-server.html#confval-album-properties",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_ARTIST_NAMESPACES,
            "/server/upnp/artist-properties", "config-server.html#confval-artist-properties",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_GENRE_NAMESPACES,
            "/server/upnp/genre-properties", "config-server.html#confval-genre-properties",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_PLAYLIST_NAMESPACES,
            "/server/upnp/playlist-properties", "config-server.html#confval-playlist-properties",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_TITLE_NAMESPACES,
            "/server/upnp/title-properties", "config-server.html#confval-title-properties",
            ConfigVal::A_UPNP_NAMESPACE_PROPERTY, ConfigVal::A_UPNP_NAMESPACE_KEY, ConfigVal::A_UPNP_NAMESPACE_URI,
            false, false, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_NAMESPACE_KEY,
            "attribute::xmlns", "config-server.html#confval-xmlns"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_NAMESPACE_URI,
            "attribute::uri", "config-server.html#confval-uri"),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_PROPERTIES_UPNPTAG,
            "attribute::upnp-tag", "config-server.html#confval-upnp-tag"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_PROPERTIES_METADATA,
            "attribute::meta-data", "config-server.html#confval-meta-data"),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS,
            "/server/upnp/resource-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpResourceDefaultPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS,
            "/server/upnp/object-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpObjectDefaultPropDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS,
            "/server/upnp/container-defaults", "config-server.html#upnp",
            ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            false, false, false, upnpContainerDefaultPropDefaults),
        std::make_shared<ConfigSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY,
            "property-default", "config-server.html#confval-property-default"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG,
            "attribute::tag", "config-server.html#confval-property-default-tag"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE,
            "attribute::value", "config-server.html#confval-proptery-default-value"),

        // Dynamic (aka search) containers
        std::make_shared<ConfigDynamicContentSetup>(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
            "/server/containers", "config-server.html#confval-containers"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST_ENABLED,
            "/server/containers/attribute::enabled", "config-server.html#confval-containers-enabled",
            true, false),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION,
            "attribute::location", "config-server.html#confval-containers-container-location",
            "/Auto", ConfigPathArguments::isFile | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE,
            "attribute::image", "config-server.html#confval-containers-container-image",
            "", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE,
            "attribute::title", "config-server.html#confval-containers-container-title",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER,
            "filter", "config-server.html#confval-container-filter",
            true, "last_updated > \"@last31\""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT,
            "attribute::upnp-shortcut", "config-server.html#confval-containers-container-upnp-shortcut",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT,
            "attribute::sort", "config-server.html#confval-containers-container-sort",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT,
            "attribute::max-count", "config-server.html#confval-containers-container-max-count",
            500, 0, ConfigIntSetup::CheckMinValue),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getServerExtendedOptions()
{
    return {
    // Thumbnails for images and videos
#ifdef HAVE_FFMPEGTHUMBNAILER
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled", "config-extended.html#confval-ffmpegthumbnailer-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::video-enabled", "config-extended.html#confval-ffmpegthumbnailer-video-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/attribute::image-enabled", "config-extended.html#confval-ffmpegthumbnailer-image-enabled",
            YES),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
            "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size", "config-extended.html#confval-ffmpegthumbnailer-thumbnail-size",
            160, 1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
            "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage", "config-extended.html#confval-ffmpegthumbnailer-seek-percentage",
            5, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ROTATE,
            "/server/extended-runtime-options/ffmpegthumbnailer/rotate", "config-extended.html#confval-ffmpegthumbnailer-rotate",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
            "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay", "config-extended.html#confval-ffmpegthumbnailer-filmstrip-overlay",
            NO),
        std::make_shared<ConfigIntSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
            "/server/extended-runtime-options/ffmpegthumbnailer/image-quality", "config-extended.html#confval-ffmpegthumbnailer-image-quality",
            8, CheckImageQualityValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
            "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled", "config-extended.html#confval-ffmpegthumbnailer-cache-dir-enabled",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, // ConfigPathSetup
            "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir", "config-extended.html#confval-ffmpegthumbnailer-cache-dir",
            ""),
#endif

        // Playmarks
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
            "/server/extended-runtime-options/mark-played-items/attribute::enabled", "config-extended.html#confval-mark-played-items-enabled",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
            "/server/extended-runtime-options/mark-played-items/string/attribute::mode", "config-extended.html#confval-mark-played-items-mode",
            DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE, StringCheckFunction(ConfigBoolSetup::CheckMarkPlayedValue)),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
            "/server/extended-runtime-options/mark-played-items/string", "config-extended.html#confval-mark-played-items-string",
            false, "*", true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
            "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates", "config-extended.html#confval-mark-played-items-cds-updates",
            YES),
        std::make_shared<ConfigArraySetup>(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
            "/server/extended-runtime-options/mark-played-items/mark", "config-extended.html#confval-mark-played-items-mark",
            ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, ConfigArraySetup::InitPlayedItemsMark),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT,
            "content", "config-extended.html#confval-mark-played-items-content"),

#ifdef HAVE_LASTFM
        std::make_shared<ConfigBoolSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED,
            "/server/extended-runtime-options/lastfm/attribute::enabled", "config-extended.html#confval-lastfm-enabled",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME,
            "/server/extended-runtime-options/lastfm/username", "config-extended.html#confval-lastfm-username",
            false, "lastfmuser", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD,
            "/server/extended-runtime-options/lastfm/password", "config-extended.html#confval-lastfm-password",
            false, "lastfmpass", true),
#ifndef HAVE_LASTFMLIB
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_SESSIONKEY,
            "/server/extended-runtime-options/lastfm/sessionkey", "config-extended.html#confval-lastfm-sessionkey",
            false, "sessionKey", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_AUTHURL,
            "/server/extended-runtime-options/lastfm/auth-url", "config-extended.html#confval-lastfm-auth-url",
            false, "https://www.last.fm/api/auth/", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::SERVER_EXTOPTS_LASTFM_SCROBBLEURL,
            "/server/extended-runtime-options/lastfm/scrobble-url", "config-extended.html#confval-lastfm-scrobble-url",
            false, "https://ws.audioscrobbler.com/2.0/", true),
#endif
#endif

#ifdef HAVE_CURL
        std::make_shared<ConfigIntSetup>(ConfigVal::URL_REQUEST_CURL_BUFFER_SIZE,
            "/server/online-content/attribute::fetch-buffer-size", "config-online.html#online-content",
            1024 * 1024, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::URL_REQUEST_CURL_FILL_SIZE,
            "/server/online-content/attribute::fetch-buffer-fill-size", "config-online.html#online-content",
            0, 0, ConfigIntSetup::CheckMinValue),
#endif // HAVE_CURL
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getClientOptions()
{
    return {
        std::make_shared<ConfigClientSetup>(ConfigVal::CLIENTS_LIST,
            "/clients", "config-clients.html#confval-clients"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::CLIENTS_LIST_ENABLED,
            "/clients/attribute::enabled", "config-clients.html#confval-clients-enabled",
            NO),
        std::make_shared<ConfigTimeSetup>(ConfigVal::CLIENTS_CACHE_THRESHOLD,
            "/clients/attribute::cache-threshold", "config-clients.html#confval-cache-threshold",
            GrbTimeType::Hours, 6, 1),
        std::make_shared<ConfigTimeSetup>(ConfigVal::CLIENTS_BOOKMARK_OFFSET,
            "/clients/attribute::bookmark-offset", "config-clients.html#confval-bookmark-offset",
            GrbTimeType::Seconds, 10, 0),

        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_CLIENT,
            "/clients/client", "config-clients.html#confval-client",
            ""),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_CLIENTS_CLIENT_FLAGS,
            "flags", "config-clients.html#confval-flags",
            0U, ClientConfig::makeFlags, ClientConfig::mapFlags),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_IP,
            "ip", "config-clients.html#confval-client-ip",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_GROUP,
            "group", "config-clients.html#confval-client-group",
            DEFAULT_CLIENT_GROUP),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_USERAGENT,
            "userAgent", "config-clients.html#confval-client-userAgent",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED,
            "allowed", "config-clients.html#confval-client-allowed",
            YES),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE,
            "/clients/client", "config-import.html#confval-client-map",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM,
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, false),
        std::make_shared<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
            "/clients/client", "config-import.html#confval-client-dlna",
            ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO },
            false, false, true),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT,
            "attribute::caption-info-count", "config-clients.html#confval-client-caption-info-count",
            -1, -1, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT,
            "attribute::upnp-string-limit", "config-clients.html#confval-client-upnp-string-limit",
            DEFAULT_UPNP_STRING_LIMIT, CheckUpnpStringLimitValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE,
            "attribute::multi-value", "config-clients.html#confval-client-multi-value",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_FILTER_FULL,
            "attribute::full-filter", "config-clients.html#confval-full-filter",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS,
            "/clients/client", "config-import.html#confval-client-header",
            ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER,
            ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY,
            ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE,
            false, false, false),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY,
            "attribute::key", "config-import.html#headers", ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE,
            "attribute::value", "config-import.html#headers", ""),

        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_GROUP,
            "/clients/group", "config-clients.html#confval-group"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_CLIENTS_GROUP_NAME,
            "name", "config-clients.html#confval-group-name",
            DEFAULT_CLIENT_GROUP),
        std::make_shared<ConfigArraySetup>(ConfigVal::A_CLIENTS_GROUP_HIDDEN_LIST,
            "/clients/group", "config-clients.html#confval-group-hide",
            ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION,
            false, false),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_GROUP_HIDE,
            "hide", "config-clients.html#confval-group-hide"),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_CLIENTS_GROUP_LOCATION,
            "attribute::location", "config-clients.html#confval-hide-location",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_CLIENTS_GROUP_ALLOWED,
            "attribute::allowed", "config-clients.html#confval-group-allowed",
            YES),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getImportOptions()
{
    /// @brief default values for ConfigVal::BOXLAYOUT_BOX
    static const std::vector<BoxLayout> boxLayoutDefaults {
        BoxLayout(BoxKeys::root, "Root", UPNP_CLASS_CONTAINER, "", "00000"),
        BoxLayout(BoxKeys::pcDirectory, "PC Directory", UPNP_CLASS_CONTAINER, "", "00000"),
        // FOLDER_STRUCTURE

        BoxLayout(BoxKeys::audioAllAlbums, "Albums", UPNP_CLASS_CONTAINER, "MUSIC_ALBUMS"),
        BoxLayout(BoxKeys::audioAllArtists, "Artists", UPNP_CLASS_CONTAINER, "MUSIC_ARTISTS"),
        BoxLayout(BoxKeys::audioAll, "All Audio"),
        BoxLayout(BoxKeys::audioAllComposers, "Composers"),
        BoxLayout(BoxKeys::audioAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "MUSIC_FOLDER_STRUCTURE"),
        BoxLayout(BoxKeys::audioAllGenres, "Genres", UPNP_CLASS_CONTAINER, "MUSIC_GENRES"),
        BoxLayout(BoxKeys::audioAllSongs, "All Songs"),
        BoxLayout(BoxKeys::audioAllTracks, "All - full name", UPNP_CLASS_CONTAINER, "MUSIC_ALL"),
        BoxLayout(BoxKeys::audioAllYears, "Year"),
        BoxLayout(BoxKeys::audioRoot, "Audio"),
        BoxLayout(BoxKeys::audioArtistChronology, "Album Chronology"),

        BoxLayout(BoxKeys::audioInitialAbc, "ABC"),
        BoxLayout(BoxKeys::audioInitialAllArtistTracks, "000 All"),
        BoxLayout(BoxKeys::audioInitialAllBooks, "Books"),
        BoxLayout(BoxKeys::audioInitialAudioBookRoot, "AudioBooks", UPNP_CLASS_CONTAINER, "MUSIC_AUDIOBOOKS"),

        BoxLayout(BoxKeys::audioStructuredAllAlbums, "-Album-", UPNP_CLASS_CONTAINER, "MUSIC_ALBUMS", "", true, true, 6),
        BoxLayout(BoxKeys::audioStructuredAllArtistTracks, "all"),
        BoxLayout(BoxKeys::audioStructuredAllArtists, "-Artist-", UPNP_CLASS_CONTAINER, "MUSIC_ARTISTS", "", true, true, 9),
        BoxLayout(BoxKeys::audioStructuredAllGenres, "-Genre-", UPNP_CLASS_CONTAINER, "MUSIC_GENRES", "", true, true, 6),
        BoxLayout(BoxKeys::audioStructuredAllTracks, "-Track-", UPNP_CLASS_CONTAINER, "MUSIC_ALL", "", true, true, 6),
        BoxLayout(BoxKeys::audioStructuredAllYears, "-Year-"),

        BoxLayout(BoxKeys::videoAllDates, "Date", UPNP_CLASS_CONTAINER, "VIDEOS_YEARS_MONTH"),
        BoxLayout(BoxKeys::videoAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "VIDEOS_FOLDER_STRUCTURE"),
        BoxLayout(BoxKeys::videoAll, "All Video", UPNP_CLASS_CONTAINER, "VIDEOS_ALL"),
        BoxLayout(BoxKeys::videoAllYears, "Year", UPNP_CLASS_CONTAINER, "VIDEOS_YEARS"),
        BoxLayout(BoxKeys::videoUnknown, "Unknown"),
        BoxLayout(BoxKeys::videoRoot, "Video", UPNP_CLASS_CONTAINER, "VIDEOS"),
        // VIDEOS_GENRES
        // VIDEOS_ALBUM
        // VIDEOS_RECENTLY_ADDED
        // VIDEOS_LAST_PLAYED
        // VIDEOS_RECORDINGS

        // IMAGES_ALBUM
        // IMAGES_SLIDESHOWS
        // IMAGES_RECENTLY_ADDED
        // IMAGES_LAST_WATCHED
        BoxLayout(BoxKeys::imageAllDates, "Date", UPNP_CLASS_CONTAINER, "IMAGES_YEARS_MONTH"),
        BoxLayout(BoxKeys::imageAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "IMAGES_FOLDER_STRUCTURE"),
        BoxLayout(BoxKeys::imageAll, "All Photos", UPNP_CLASS_CONTAINER, "IMAGES_ALL"),
        BoxLayout(BoxKeys::imageAllYears, "Year", UPNP_CLASS_CONTAINER, "IMAGES_YEARS"),
        BoxLayout(BoxKeys::imageRoot, "Photos", UPNP_CLASS_CONTAINER, "IMAGES"),
        BoxLayout(BoxKeys::imageUnknown, "Unknown"),
        BoxLayout(BoxKeys::imageAllModels, "All Models", UPNP_CLASS_CONTAINER),
        BoxLayout(BoxKeys::imageYearMonth, "Year+Month", UPNP_CLASS_CONTAINER),
        BoxLayout(BoxKeys::imageYearDate, "Year+Date", UPNP_CLASS_CONTAINER),
        BoxLayout(BoxKeys::topicRoot, "Topics", UPNP_CLASS_CONTAINER),
        BoxLayout(BoxKeys::topic, "Topic", UPNP_CLASS_CONTAINER),
        BoxLayout(BoxKeys::topicExtra, "Extra", UPNP_CLASS_CONTAINER),

#ifdef ONLINE_SERVICES
        BoxLayout(BoxKeys::trailerRoot, "Online Services"),
        BoxLayout(BoxKeys::trailerAll, "All Trailers"),
        BoxLayout(BoxKeys::trailerAllGenres, "Genres"),
        BoxLayout(BoxKeys::trailerRelDate, "Release Date"),
        BoxLayout(BoxKeys::trailerPostDate, "Post Date"),
        BoxLayout(BoxKeys::trailerUnknown, "Unknown"),
#endif
#ifdef HAVE_JS
        BoxLayout(BoxKeys::playlistRoot, "Playlists"),
        BoxLayout(BoxKeys::playlistAll, "All Playlists"),
        BoxLayout(BoxKeys::playlistAllDirectories, "Directories", UPNP_CLASS_CONTAINER, "MUSIC_PLAYLISTS", "", true, true, 1),
#endif
    };

    /// @brief default values for ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS
    static const std::vector<std::vector<std::pair<std::string, std::string>>> virtualDirectoryKeys {
        { { "metadata", "M_ALBUMARTIST" }, { "class", UPNP_CLASS_MUSIC_ALBUM } },
        { { "metadata", "M_UPNP_DATE" }, { "class", UPNP_CLASS_MUSIC_ALBUM } },
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS
    static const std::vector<std::string> ignoreDefaults {
        "part",
        "tmp",
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST
    static const std::map<std::string, std::string> mtCtDefaults {
        { "application/ogg", CONTENT_TYPE_OGG },
        { "audio/L16", CONTENT_TYPE_PCM },
        { "audio/flac", CONTENT_TYPE_FLAC },
        { "audio/mp4", CONTENT_TYPE_MP4 },
        { "audio/mpeg", CONTENT_TYPE_MP3 },
        { "audio/ogg", CONTENT_TYPE_OGG },
        { "audio/x-dsd", CONTENT_TYPE_DSD },
        { "audio/x-flac", CONTENT_TYPE_FLAC },
        { "audio/x-matroska", CONTENT_TYPE_MKA },
        { "audio/x-mpegurl", CONTENT_TYPE_PLAYLIST },
        { "audio/x-ms-wma", CONTENT_TYPE_WMA },
        { "audio/x-scpls", CONTENT_TYPE_PLAYLIST },
        { "audio/x-wav", CONTENT_TYPE_PCM },
        { "audio/x-wavpack", CONTENT_TYPE_WAVPACK },
        { "image/jpeg", CONTENT_TYPE_JPG },
        { "image/png", CONTENT_TYPE_PNG },
        { "video/mkv", CONTENT_TYPE_MKV },
        { "video/mp4", CONTENT_TYPE_MP4 },
        { "video/mpeg", CONTENT_TYPE_MPEG },
        { "video/x-matroska", CONTENT_TYPE_MKV },
        { "video/x-mkv", CONTENT_TYPE_MKV },
        { "video/x-ms-asf", CONTENT_TYPE_ASF },
        { MIME_TYPE_ASX_PLAYLIST, CONTENT_TYPE_PLAYLIST },
        { "video/x-msvideo", CONTENT_TYPE_AVI },
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST
    static const std::vector<std::vector<std::pair<std::string, std::string>>> ctDlnaDefaults {
        { { "from", CONTENT_TYPE_ASF }, { "to", "VC_ASF_AP_L2_WMA" } },
        { { "from", CONTENT_TYPE_AVI }, { "to", "AVI" } },
        { { "from", CONTENT_TYPE_DSD }, { "to", "DSF" } },
        { { "from", CONTENT_TYPE_FLAC }, { "to", "FLAC" } },
        { { "from", CONTENT_TYPE_JPG }, { "to", "JPEG_LRG" } },
        { { "from", CONTENT_TYPE_MKA }, { "to", "MKV" } },
        { { "from", CONTENT_TYPE_MKV }, { "to", "MKV" } },
        { { "from", CONTENT_TYPE_MP3 }, { "to", "MP3" } },
        { { "from", CONTENT_TYPE_MP4 }, { "to", "AVC_MP4_EU" } },
        { { "from", CONTENT_TYPE_MPEG }, { "to", "MPEG_PS_PAL" } },
        { { "from", CONTENT_TYPE_OGG }, { "to", "OGG" } },
        { { "from", CONTENT_TYPE_PCM }, { "to", "LPCM" } },
        { { "from", CONTENT_TYPE_PNG }, { "to", "PNG_LRG" } },
        { { "from", CONTENT_TYPE_WMA }, { "to", "WMAFULL" } },
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST
    static const std::map<std::string, std::string> mtUpnpDefaults {
        { "application/ogg", UPNP_CLASS_MUSIC_TRACK },
        { "audio/*", UPNP_CLASS_MUSIC_TRACK },
        { "image/*", UPNP_CLASS_IMAGE_ITEM },
        { "video/*", UPNP_CLASS_VIDEO_ITEM },
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST
    static const std::map<std::string, std::string> mtTransferDefaults {
        { "application/ogg", UPNP_DLNA_TRANSFER_MODE_STREAMING },
        { "audio/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
        { "image/*", UPNP_DLNA_TRANSFER_MODE_INTERACTIVE },
        { "video/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
        { "text/*", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
        { MIME_TYPE_SRT_SUBTITLE, UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
        { "srt", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
    };

    /// @brief default values for ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST
    static const std::map<std::string, std::string> extMtDefaults {
        { "asf", "video/x-ms-asf" },
        { "asx", MIME_TYPE_ASX_PLAYLIST }, // tweak to handle asx as playlist
        { "dff", "audio/x-dff" },
        { "dsd", "audio/x-dsd" },
        { "dsf", "audio/x-dsf" },
        { "flv", "video/x-flv" },
        { "m2ts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
        { "m3u", "audio/x-mpegurl" },
        { "m3u8", "audio/x-mpegurl" },
        { "m4a", "audio/mp4" }, // LibMagic identifies this as audio/x-m4a, preventing TagLib from parsing it
        { "mka", "audio/x-matroska" },
        { "mkv", "video/x-matroska" },
        { "mp3", "audio/mpeg" },
        { "mts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
        { "oga", "audio/ogg" },
        { "ogg", "audio/ogg" },
        { "ogm", "video/ogg" },
        { "ogv", "video/ogg" },
        { "ogx", "application/ogg" },
        { "pls", "audio/x-scpls" },
        { "srt", MIME_TYPE_SRT_SUBTITLE },
        { "ts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
        { "tsa", "audio/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
        { "tsv", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
        { "wax", "audio/x-ms-wax" },
        { "wm", "video/x-ms-wm" },
        { "wma", "audio/x-ms-wma" },
        { "wmv", "video/x-ms-wmv" },
        { "wmx", "video/x-ms-wmx" },
        { "wv", "audio/x-wavpack" },
        { "wvx", "video/x-ms-wvx" },
    };

    /// @brief default values for ConfigVal::IMPORT_SYSTEM_DIRECTORIES
    static const std::vector<std::string> excludesFullpath {
        "/bin",
        "/boot",
        "/dev",
        "/etc",
        "/lib",
        "/lib32",
        "/lib64",
        "/libx32",
        "/proc",
        "/run",
        "/sbin",
        "/sys",
        "/tmp",
        "/usr",
        "/var",
    };

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST
    static const std::vector<std::string> defaultFanArtFile {
        // "%title%.jpg",
        // "%filename%.jpg",
        // "folder.jpg",
        // "poster.jpg",
        // "cover.jpg",
        // "albumartsmall.jpg",
        // "%album%.jpg",
    };

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST
    static const std::vector<std::vector<std::pair<std::string, std::string>>> defaultFanArtDirectory {
        { { "name", "." }, { "pattern", "%filename%" }, { "mime", "image/*" } },
    };

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST
    static const std::vector<std::string> defaultContainerArtFile {};

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST
    static const std::vector<std::string> defaultSubtitleFile {};

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST
    static const std::vector<std::vector<std::pair<std::string, std::string>>> defaultSubtitleDirectory {
        { { "name", "." }, { "pattern", "%filename%" }, { "mime", MIME_TYPE_SRT_SUBTITLE } },
    };

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST
    static const std::vector<std::string> defaultMetadataFile {};

    /// @brief default values for ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST
    static const std::vector<std::string> defaultResourceFile {};

    return {
        // boxlayout provides modification options like translation of tree items
        std::make_shared<ConfigBoxLayoutSetup>(ConfigVal::BOXLAYOUT_LIST,
            "/import/scripting/virtual-layout/boxlayout", "config-import.html#confval-boxlayout",
            boxLayoutDefaults),
        std::make_shared<ConfigSetup>(ConfigVal::A_BOXLAYOUT_CHAIN,
            "chain", "config-import.html#confval-chain"),
        std::make_shared<ConfigEnumSetup<AutoscanMediaMode>>(ConfigVal::A_BOXLAYOUT_CHAIN_TYPE,
            "attribute::type", "config-import.html#boxlayout",
            std::map<std::string, AutoscanMediaMode>(
                {
                    { "audio", AutoscanMediaMode::Audio },
                    { "image", AutoscanMediaMode::Image },
                    { "video", AutoscanMediaMode::Video },
                })),
        std::make_shared<ConfigVectorSetup>(ConfigVal::A_BOXLAYOUT_CHAIN_LINKS,
            "links", "config-import.html#confval-chain-link",
            ConfigVal::A_BOXLAYOUT_CHAIN_LINK,
            std::vector<ConfigVal> { ConfigVal::A_BOXLAYOUT_BOX_KEY },
            false, false, false),
        std::make_shared<ConfigSetup>(ConfigVal::A_BOXLAYOUT_BOX,
            "box", "config-import.html#confval-box"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY,
            "attribute::key", "config-import.html#confval-box-key",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE,
            "attribute::title", "config-import.html#confval-box-title",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS,
            "attribute::class", "config-import.html#confval-box-class",
            UPNP_CLASS_CONTAINER),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT,
            "attribute::upnp-shortcut", "config-import.html#confval-upnp-shortcut",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE,
            "attribute::size", "config-import.html#confval-box-size",
            1, -10, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED,
            "attribute::enabled", "config-import.html#confval-box-enabled",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY,
            "attribute::sort-key", "config-import.html#confval-sort-key",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_SEARCHABLE,
            "attribute::searchable", "config-import.html#confval-box-searchable",
            YES),

        // default file settings, can be overwritten in autoscan
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_HIDDEN_FILES,
            "/import/attribute::hidden-files", "config-import.html#confval-import-hidden-files",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_FOLLOW_SYMLINKS,
            "/import/attribute::follow-symlinks", "config-import.html#confval-import-follow-symlinks",
            DEFAULT_FOLLOW_SYMLINKS_VALUE),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_DEFAULT_DATE,
            "/import/attribute::default-date", "config-import.html#confval-default-date",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_NOMEDIA_FILE,
            "/import/attribute::nomedia-file", "config-import.html#confval-nomedia-file",
            ".nomedia"),
        std::make_shared<ConfigEnumSetup<ImportMode>>(ConfigVal::IMPORT_LAYOUT_MODE,
            "/import/attribute::import-mode", "config-import.html#confval-import-mode",
            ImportMode::MediaTomb,
            std::map<std::string, ImportMode>({ { "mt", ImportMode::MediaTomb }, { "grb", ImportMode::Gerbera } })),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_READABLE_NAMES,
            "/import/attribute::readable-names", "config-import.html#confval-readable-names",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_CASE_SENSITIVE_TAGS,
            "/import/attribute::case-sensitive-tags", "config-import.html#confval-case-sensitive-tags",
            YES),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS,
            "/import/virtual-directories", "config-import.html#confval-virtual-directories",
            ConfigVal::A_IMPORT_VIRT_DIR_KEY,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_VIRT_DIR_METADATA },
            false, false, true,
            virtualDirectoryKeys),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_VIRT_DIR_KEY,
            "key", "config-import.html#confval-virtual-directories-key",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_VIRT_DIR_METADATA,
            "metadata", "config-import.html#confval-virtual-directories-key-metadata",
            ""),

        // mappings extension -> mimetype -> upnpclass / contenttype / transfermode -> dlnaprofile
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
            "/import/mappings/extension-mimetype", "config-import.html#confval-extension-mimetype",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, extMtDefaults),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
            "/import/mappings/extension-mimetype/attribute::ignore-unknown", "config-import.html#confval-extension-mimetype-ignore-unknown",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
            "/import/mappings/extension-mimetype/attribute::case-sensitive", "config-import.html#confval-extension-mimetype-case-sensitive",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
            "/import/mappings/mimetype-upnpclass", "config-import.html#mimetype-upnpclass",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, mtUpnpDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
            "/import/mappings/mimetype-dlnatransfermode", "config-import.html#confval-mimetype-dlnatransfermode",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            false, false, true, mtTransferDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
            "/import/mappings/mimetype-contenttype", "config-import.html#confval-mimetype-contenttype",
            ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
            false, false, true, mtCtDefaults),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
            "/import/mappings/contenttype-dlnaprofile", "config-import.html#confval-contenttype-dlnaprofile",
            ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO },
            false, false, true,
            ctDlnaDefaults),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS,
            "/import/mappings/ignore-extensions", "config-import.html#confval-ignore-extensions",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, ignoreDefaults),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LAYOUT_MAPPING,
            "/import/layout", "config-import.html#confval-layout",
            ConfigVal::A_IMPORT_LAYOUT_MAPPING_PATH, ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LAYOUT_PARENT_PATH,
            "/import/layout/attribute::parent-path", "config-import.html#confval-parent-path",
            NO),

    // Scripting
#ifdef HAVE_JS
        std::make_shared<ConfigEnumSetup<AutoscanScanMode>>(ConfigVal::IMPORT_SCRIPTING_SCAN_MODE,
            "/import/scripting/attribute::scan-mode", "config-import.html#confval-scripting-scan-mode",
            AutoscanScanMode::Manual,
            std::map<std::string, AutoscanScanMode>(
                {
                    { AUTOSCAN_MANUAL, AutoscanScanMode::Manual },
                    { AUTOSCAN_TIMED, AutoscanScanMode::Timed },
#ifdef HAVE_INOTIFY
                    { AUTOSCAN_INOTIFY, AutoscanScanMode::INotify },
#endif
                })),
        std::make_shared<ConfigTimeSetup>(ConfigVal::IMPORT_SCRIPTING_SCAN_INTERVAL,
            "/import/scripting/attribute::scan-interval", "config-import.html#confval-scripting-scan-interval",
            GrbTimeType::Minutes, 48 * 60, 0),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_CHARSET,
            "/import/scripting/attribute::script-charset", "config-import.html#confval-script-charset",
            "UTF-8", ConfigStringSetup::CheckCharset),

        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER,
            "/import/scripting/script-folder/common", "config-import.html#confval-script-folder-common",
            "", ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER,
            "/import/scripting/script-folder/custom", "config-import.html#confval-script-folder-custom",
            "", ConfigPathArguments::none),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST,
            "/import/scripting/import-function/playlist", "config-import.html#confval-playlist",
            "importPlaylist"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS,
            "/import/scripting/import-function/playlist/attribute::create-link", "config-import.html#confval-playlist-create-link",
            YES),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE,
            "/import/scripting/import-function/meta-file", "config-import.html#confval-meta-file",
            "importMetadata"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE,
            "/import/scripting/import-function/audio-file", "config-import.html#confval-audio-file",
            "importAudio"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE,
            "/import/scripting/import-function/video-file", "config-import.html#confval-video-file",
            "importVideo"),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE,
            "/import/scripting/import-function/image-file", "config-import.html#confval-image-file",
            "importImage"),
#ifdef ONLINE_SERVICES
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER,
            "/import/scripting/import-function/online-item", "config-import.html#confval-trailer",
            "importOnlineItem"),
#endif

        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
            "/import/scripting/virtual-layout/script-options", "config-import.html#confval-script-options",
            ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION, ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE),

        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS,
            "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "config-import.html#confval-skip-chars",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR,
            "/import/scripting/virtual-layout/structured-layout/attribute::div-char", "config-import.html#confval-div-char",
            ""),
#endif // HAVE_JS

        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
            "/import/scripting/virtual-layout/genre-map", "config-import.html#confval-genre-map",
            ConfigVal::A_IMPORT_LAYOUT_GENRE, ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO),

        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_MODEL_MAP,
            "/import/scripting/virtual-layout/model-map", "config-import.html#confval-model-map",
            ConfigVal::A_IMPORT_LAYOUT_MODEL, ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO),

        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_SCRIPTING_IMPORT_HEADLINE_MAP,
            "/import/scripting/virtual-layout/headline-map", "config-import.html#confval-headline-map",
            ConfigVal::A_IMPORT_LAYOUT_HEADLINE,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO, ConfigVal::A_IMPORT_LAYOUT_HEADLINE_TYPE },
            false, false, false),

        // character conversion
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_FILESYSTEM_CHARSET,
            "/import/filesystem-charset", "config-import.html#confval-filesystem-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_METADATA_CHARSET,
            "/import/metadata-charset", "config-import.html#confval-metadata-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_PLAYLIST_CHARSET,
            "/import/playlist-charset", "config-import.html#confval-playlist-charset",
            DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigEnumSetup<LayoutType>>(ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
            "/import/scripting/virtual-layout/attribute::type", "config-import.html#confval-virtual-layout-type",
#ifdef HAVE_JS
            LayoutType::Js,
#else
            LayoutType::Builtin,
#endif
            std::map<std::string, LayoutType>({ { "js", LayoutType::Js }, { "builtin", LayoutType::Builtin }, { "disabled", LayoutType::Disabled } })),

        // tweaks
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP,
            "/import/library-options/attribute::multi-value-separator", "config-import.html#confval-multi-value-separator",
            DEFAULT_LIBOPTS_ENTRY_SEPARATOR),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP,
            "/import/library-options/attribute::legacy-value-separator", "config-import.html#confval-legacy-value-separator",
            ""),

        // resources
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE,
            "/import/resources/attribute::case-sensitive", "config-import.html#confval-case-sensitive",
            DEFAULT_RESOURCES_CASE_SENSITIVE),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST,
            "/import/resources/fanart", "config-import.html#confval-fanart",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultFanArtFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
            "/import/resources/fanart", "config-import.html#confval-fanart",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT, ConfigVal::A_IMPORT_RESOURCES_MIME },
            false, false, false,
            defaultFanArtDirectory),

        // container art
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
            "/import/resources/container", "config-import.html#confval-container",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultContainerArtFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
            "/import/resources/container", "config-import.html#confval-container",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT, ConfigVal::A_IMPORT_RESOURCES_MIME },
            false, false, false),
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION,
            "/import/resources/container/attribute::location", "config-import.html#confval-resource-container-location",
            ""),
        std::make_shared<ConfigIntSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT,
            "/import/resources/container/attribute::parentCount", "config-import.html#confval-parentCount",
            2, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_MINDEPTH,
            "/import/resources/container/attribute::minDepth", "config-import.html#confval-minDepth",
            2, 0, ConfigIntSetup::CheckMinValue),

        // subtitles
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
            "/import/resources/subtitle", "config-import.html#confval-subtitle",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultSubtitleFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
            "/import/resources/subtitle", "config-import.html#confval-subtitle",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT, ConfigVal::A_IMPORT_RESOURCES_MIME },
            false, false, false,
            defaultSubtitleDirectory),

        // meta files
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST,
            "/import/resources/metafile", "config-import.html#confval-metafile",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultMetadataFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
            "/import/resources/metafile", "config-import.html#confval-metafile",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT, ConfigVal::A_IMPORT_RESOURCES_MIME },
            false, false, false),

        // resource reverse
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST,
            "/import/resources/resource", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_FILE, ConfigVal::A_IMPORT_RESOURCES_NAME,
            false, false, defaultResourceFile),
        std::make_shared<ConfigVectorSetup>(ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
            "/import/resources/resource", "config-import.html#resources",
            ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            std::vector<ConfigVal> { ConfigVal::A_IMPORT_RESOURCES_NAME, ConfigVal::A_IMPORT_RESOURCES_PTT, ConfigVal::A_IMPORT_RESOURCES_EXT, ConfigVal::A_IMPORT_RESOURCES_MIME },
            false, false, false),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_ORDER,
            "/import/resources/order", "config-import.html#confval-order",
            ConfigVal::A_IMPORT_RESOURCES_HANDLER, ConfigVal::A_IMPORT_RESOURCES_NAME,
            EnumMapper::checkContentHandler),

        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_SYSTEM_DIRECTORIES,
            "/import/system-directories", "config-import.html#confval-system-directories",
            ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH, ConfigVal::A_IMPORT_RESOURCES_NAME, true, true,
            excludesFullpath),
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_VISIBLE_DIRECTORIES,
            "/import/visible-directories", "config-import.html#confval-visible-directories",
            ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH, ConfigVal::A_IMPORT_RESOURCES_NAME, false, false),

    // Autoscan settings
#ifdef HAVE_INOTIFY
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY,
            "/import/autoscan/attribute::use-inotify", "config-import.html#confval-use-inotify",
            "auto", StringCheckFunction(ConfigBoolSetup::CheckInotifyValue)),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_ATTRIB,
            "/import/autoscan/attribute::inotify-attrib", "config-import.html#confval-inotify-attrib",
            NO),
        std::make_shared<ConfigAutoscanSetup>(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
            "/import/autoscan", "config-import.html#confval-autoscan",
            AutoscanScanMode::INotify),
#endif
        std::make_shared<ConfigSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY,
            "directory", "config-import.html#confval-directory",
            ""),
        std::make_shared<ConfigAutoscanSetup>(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
            "/import/autoscan", "config-import.html#confval-autoscan",
            AutoscanScanMode::Timed),
        std::make_shared<ConfigAutoscanSetup>(ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
            "/import/autoscan", "config-import.html#confval-autoscan",
            AutoscanScanMode::Manual),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION,
            "attribute::location", "config-import.html#confval-directory-location",
            "", ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigEnumSetup<AutoscanScanMode>>(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE,
            "attribute::mode", "config-import.html#confval-directory-mode",
            std::map<std::string, AutoscanScanMode>(
                {
                    { AUTOSCAN_TIMED, AutoscanScanMode::Timed },
#ifdef HAVE_INOTIFY
                    { AUTOSCAN_INOTIFY, AutoscanScanMode::INotify },
#endif
                    { AUTOSCAN_MANUAL, AutoscanScanMode::Manual },
                })),
        std::make_shared<ConfigTimeSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL,
            "attribute::interval", "config-import.html#confval-directory-interval",
            GrbTimeType::Seconds, -1, 0),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE,
            "attribute::recursive", "config-import.html#confval-directory-recursive",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES,
            "attribute::dirtypes", "config-import.html#confval-dirtypes",
            true, false),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE,
            "attribute::media-type", "config-import.html#confval-media-type",
            to_underlying(AutoscanDirectory::MediaType::Any), AutoscanDirectory::makeMediaType),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES,
            "attribute::hidden-files", "config-import.html#confval-directory-hide"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS,
            "attribute::follow-symlinks", "config-import.html#confval-directory-follow-symlinks"),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_TASKCOUNT,
            "attribute::task-count", "config-import.html#confval-autoscan"),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT,
            "attribute::scan-count", "config-import.html#confval-autoscan"),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT,
            "attribute::retry-count", "config-import.html#confval-directoy-autoscan",
            0),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_LMT,
            "attribute::last-modified", "config-import.html#confval-autoscan"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FORCE_REREAD_UNKNOWN,
            "attribute::force-reread-unknown", "config-import.html#confval-force-reread-unknown",
            NO),

        // Directory Tweaks
        std::make_shared<ConfigDirectorySetup>(ConfigVal::IMPORT_DIRECTORIES_LIST,
            "/import/directories", "config-import.html#confval-autoscan"),
        std::make_shared<ConfigSetup>(ConfigVal::A_DIRECTORIES_TWEAK,
            "/import/directories/tweak", "config-import.html#confval-autoscan",
            ""),
        std::make_shared<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION,
            "attribute::location", "config-import.html#confval-autoscan",
            "", ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT,
            "attribute::inherit", "config-import.html#confval-autoscan",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE,
            "attribute::recursive", "config-import.html#confval-autoscan",
            false, true),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN,
            "attribute::hidden-files", "config-import.html#confval-autoscan"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE,
            "attribute::case-sensitive", "config-import.html#confval-autoscan",
            DEFAULT_RESOURCES_CASE_SENSITIVE),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS,
            "attribute::follow-symlinks", "config-import.html#confval-autoscan",
            DEFAULT_FOLLOW_SYMLINKS_VALUE),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET,
            "attribute::meta-charset", "config-import.html#confval-charset",
            "", ConfigStringSetup::CheckCharset),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE,
            "attribute::fanart-file", "config-import.html#confval-resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE,
            "attribute::subtitle-file", "config-import.html#confval-resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE,
            "attribute::metadata-file", "config-import.html#confval-resources",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE,
            "attribute::resource-file", "config-import.html#confval-resources",
            ""),

        // mapping attributes
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT,
            "treat", "config-import.html#confval-mimetype-contenttype-treat",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE,
            "attribute::mimetype", "config-import.html#confval-mimetype-contenttype-treat-mimetype",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
            "attribute::as", "config-import.html#confval-mimetype-contenttype-treat-as",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM,
            "attribute::from", "config-import.html#confval-contenttype-dlnaprofile-from",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO,
            "attribute::to", "config-import.html#confval-contenttype-dlnaprofile-to",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_NAME,
            "attribute::name", "config-import.html#confval-resource-add-file-name",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_PTT,
            "attribute::pattern", "config-import.html#confval-resource-add-dir-pattern",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_EXT,
            "attribute::ext", "config-import.html#confval-resource-add-file-ext",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_RESOURCES_MIME,
            "attribute::mime", "config-import.html#confval-resource-add-file-mime",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM,
            "attribute::from", "config-import.html#confval-layout-path-from",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO,
            "attribute::to", "config-import.html#confval-layout-path-to",
            ""),

        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO,
            "container-type-audio", "config-import.html#confval-container-type-audio",
            UPNP_CLASS_MUSIC_ALBUM),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE,
            "container-type-image", "config-import.html#confval-container-type-image",
            UPNP_CLASS_PHOTO_ALBUM),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO,
            "container-type-video", "config-import.html#confval-container-type-video",
            UPNP_CLASS_CONTAINER),

        // library attributes
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            "attribute::tag", "config-import.html#confval-auxdata-add-data-tag",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            "attribute::key", "config-import.html#confval-metadata-add-data-key",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            "attribute::tag", "config-import.html#confval-library-comment-tag",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            "attribute::label", "config-import.html#confval-library-comment-label",
            ""),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getLibraryOptions()
{
#ifdef HAVE_EXIV2
    /// @brief default values for ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST
    static const std::map<std::string, std::string> exiv2CommentDefaults {
        { "Taken with", "Exif.Image.Model" },
        { "Flash setting", "Exif.Photo.Flash" },
        { "Focal length", "Exif.Photo.FocalLength" },
        { "Focal length 35 mm equivalent", "Exif.Photo.FocalLengthIn35mmFilm" },
    };
#endif

#ifdef HAVE_TAGLIB
    /// @brief default values for ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST
    static const std::map<std::string, std::string> id3SpecialPropertyMap {
        { "PERFORMER", UPNP_SEARCH_ARTIST "@role[Performer]" },
    };
#endif

#ifdef HAVE_FFMPEG
    /// @brief default values for ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST
    static const std::map<std::string, std::string> ffmpegSpecialPropertyMap {
        { "performer", UPNP_SEARCH_ARTIST "@role[Performer]" },
    };
#endif

    return {
#ifdef HAVE_LIBEXIF
        // options for libexif
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
            "/import/library-options/libexif/auxdata", "config-import.html#confval-libexif",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
            "/import/library-options/libexif/metadata", "config-import.html#confval-libexif",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET,
            "/import/library-options/libexif/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_ENABLED,
            "/import/library-options/libexif/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_ENABLED,
            "/import/library-options/libexif/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
            "/import/library-options/libexif/comment", "config-import.html#confval-libexif",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_EXIV2
        // options for exiv2
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
            "/import/library-options/exiv2/auxdata", "config-import.html#confval-exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
            "/import/library-options/exiv2/metadata", "config-import.html#confval-exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET,
            "/import/library-options/exiv2/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_ENABLED,
            "/import/library-options/exiv2/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_ENABLED,
            "/import/library-options/exiv2/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            YES),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
            "/import/library-options/exiv2/comment", "config-import.html#confval-exiv2",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false, exiv2CommentDefaults),
#endif
#ifdef HAVE_TAGLIB
        // options for taglib (id3)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
            "/import/library-options/id3/auxdata", "config-import.html#confval-id3",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
            "/import/library-options/id3/metadata", "config-import.html#confval-id3",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false, id3SpecialPropertyMap),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET,
            "/import/library-options/id3/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED,
            "/import/library-options/id3/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_ENABLED,
            "/import/library-options/id3/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
            "/import/library-options/id3/comment", "config-import.html#confval-id3",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_FFMPEG
        // options for ffmpeg (libav)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
            "/import/library-options/ffmpeg/auxdata", "config-import.html#confval-ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
            "/import/library-options/ffmpeg/metadata", "config-import.html#confval-ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false, ffmpegSpecialPropertyMap),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET,
            "/import/library-options/ffmpeg/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED,
            "/import/library-options/ffmpeg/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_ENABLED,
            "/import/library-options/ffmpeg/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            NO),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_ARTWORK_ENABLED,
            "/import/library-options/ffmpeg/attribute::artwork-enabled", "config-import.html#confval-artwork-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
            "/import/library-options/ffmpeg/comment", "config-import.html#confval-ffmpeg",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
        std::make_shared<ConfigUIntSetup>(ConfigVal::IMPORT_LIBOPTS_FFMPEG_SUBTITLE_SEEK_SIZE,
            "import/library-options/ffmpeg/attribute::subtitle-seek-size", "config-import.html#confval-subtitle-seek-size",
            2048),
#endif
#ifdef HAVE_MATROSKA
        // options for matroska (mkv)
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_AUXDATA_TAGS_LIST,
            "/import/library-options/mkv/auxdata", "config-import.html#confval-mkv",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
            "/import/library-options/mkv/metadata", "config-import.html#confval-mkv",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET,
            "/import/library-options/mkv/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_ENABLED,
            "/import/library-options/mkv/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_ENABLED,
            "/import/library-options/mkv/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
            "/import/library-options/mkv/comment", "config-import.html#confval-mkv",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_WAVPACK
        // options for wavpack
        std::make_shared<ConfigArraySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST,
            "/import/library-options/wavpack/auxdata", "config-import.html#confval-wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            false, false),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
            "/import/library-options/wavpack/metadata", "config-import.html#confval-wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG,
            ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY,
            false, true, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET,
            "/import/library-options/wavpack/attribute::charset", "config-import.html#confval-library-charset",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_ENABLED,
            "/import/library-options/wavpack/attribute::enabled", "config-import.html#confval-library-enabled",
            YES),
        std::make_shared<ConfigBoolSetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_ENABLED,
            "/import/library-options/wavpack/comment/attribute::enabled", "config-import.html#confval-comment-enabled",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
            "/import/library-options/wavpack/comment", "config-import.html#confval-wavpack",
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL,
            ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG,
            false, false, false),
#endif
#ifdef HAVE_MAGIC
        std::make_shared<ConfigPathSetup>(ConfigVal::IMPORT_MAGIC_FILE,
            "/import/magic-file", "config-import.html#confval-magic-file",
            "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
#endif
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getTranscodingOptions()
{
    /// @brief default values for ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP
    static const std::map<std::string, std::string> trMtDefaults {
        { "video/x-flv", "vlcmpeg" },
        { "application/ogg", "vlcmpeg" },
        { "audio/ogg", "ogg2mp3" },
    };

    return {
        std::make_shared<ConfigTranscodingSetup>(ConfigVal::TRANSCODING_PROFILE_LIST,
            "/transcoding", "config-transcode.html#transcoding"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_TRANSCODING_ENABLED,
            "/transcoding/attribute::enabled", "config-transcode.html#confval-transcoding-enabled",
            NO),

#ifdef HAVE_CURL
        std::make_shared<ConfigIntSetup>(ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
            "/transcoding/attribute::fetch-buffer-size", "config-transcode.html#confval-fetch-buffer-size",
            262144, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
            "/transcoding/attribute::fetch-buffer-fill-size", "config-transcode.html#confval-fetch-buffer-fill-size",
            0, 0, ConfigIntSetup::CheckMinValue),
#endif // HAVE_CURL

        // mimetype identification and media filtering
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
            "/transcoding/mimetype-profile-mappings/attribute::allow-unused", "config-transcode.html#confval-mapping-allow-unused",
            NO),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP,
            "/transcoding/mimetype-profile-mappings", "config-transcode.html#confval-mimetype-profile-mappings",
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
            ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING,
            false, false, false, trMtDefaults),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_FILTER,
            "/transcoding/mimetype-profile-mappings/transcode", "config-transcode.html#confval-transcode",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_USING,
            "attribute::using", "config-transcode.html#confval-using",
            ""),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS,
            "attribute::client-flags", "config-transcode.html#confval-client-flags",
            0U, ClientConfig::makeFlags, ClientConfig::mapFlags),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTWITHOUT,
            "attribute::client-without-flag", "config-transcode.html#confval-profile-client-without-flag",
            NO),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SRCDLNA,
            "attribute::source-profile", "config-transcode.html#confval-source-profile",
            ""),

        // Transcoding Profiles
        std::make_shared<ConfigBoolSetup>(ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
            "/transcoding/profiles/attribute::allow-unused", "config-transcode.html#confval-profiles-allow-unused",
            NO),
        std::make_shared<ConfigEnumSetup<TranscodingType>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE,
            "attribute::type", "config-transcode.html#confval-profile-type",
            std::map<std::string, TranscodingType>({ { "none", TranscodingType::None },
                { "external", TranscodingType::External },
                /* for the future...{"remote", TranscodingType::Remote}*/ })),
        std::make_shared<ConfigEnumSetup<AviFourccListmode>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
            "attribute::mode", "config-transcode.html#confval-avi-fourcc-list-mode",
            std::map<std::string, AviFourccListmode>({ { "ignore", AviFourccListmode::Ignore }, { "process", AviFourccListmode::Process }, { "disabled", AviFourccListmode::None } })),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
            "fourcc", "config-transcode.html#confval-4cc-fourcc",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF,
            "dlna-profile", "config-transcode.html#confval-profile-dlna-profile",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING,
            "attribute::no-transcoding", "config-transcode.html#confval-no-transcoding",
            ""),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED,
            "attribute::enabled", "config-transcode.html#confval-profile-enabled"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL,
            "accept-url", "config-transcode.html#confval-accept-url"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG,
            "hide-original-resource", "config-transcode.html#confval-hide-original-resource"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB,
            "thumbnail", "config-transcode.html#confval-thumbnail"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST,
            "first-resource", "config-transcode.html#confval-first-resource"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG,
            "accept-ogg-theora", "config-transcode.html#confval-accept-ogg-theora"),
        std::make_shared<ConfigArraySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC,
            "avi-fourcc-list", "config-transcode.html#confval-fourcc-list",
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, ConfigVal::MAX,
            true, true),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ,
            "sample-frequency", "config-transcode.html#confval-sample-frequency",
            GRB_STRINGIZE(SOURCE), CheckProfileNumberValue),
        std::make_shared<ConfigIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN,
            "audio-channels", "config-transcode.html#confval-audio-channels",
            GRB_STRINGIZE(SOURCE), CheckProfileNumberValue),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE,
            "/transcoding/profiles/profile", "config-transcode.html#confval-profile",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME,
            "attribute::name", "config-transcode.html#confval-profile-name",
            true, "", true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
            "mimetype", "config-transcode.html#confval-profile-mimetype",
            true, "", true),

        // Buffer
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE,
            "attribute::size", "config-transcode.html#confval-buffer-size",
            0, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK,
            "attribute::chunk-size", "config-transcode.html#confval-buffer-chunk-size",
            0, 0, ConfigIntSetup::CheckMinValue),
        std::make_shared<ConfigUIntSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL,
            "attribute::fill-size", "config-transcode.html#confval-buffer-fill-size",
            0, 0, ConfigIntSetup::CheckMinValue),

        // Agent
        std::make_shared<ConfigPathSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
            "attribute::command", "config-transcode.html#confval-command",
            "",
            ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::isExe | ConfigPathArguments::resolveEmpty),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
            "attribute::arguments", "config-transcode.html#confval-arguments",
            true, "", true),
        std::make_shared<ConfigDictionarySetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON,
            "agent", "config-transcode.html#profiles",
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
            ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
            false, false, false),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_RESOLUTION,
            "resolution", "config-transcode.html#confval-resolution",
            false),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT,
            "agent", "config-transcode.html#confval-agent",
            true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER,
            "buffer", "config-transcode.html#confval-buffer",
            true),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
            "attribute::mimetype", "config-transcode.html#confval-profile-mimetype",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_VALUE,
            "attribute::value", "config-transcode.html#confval-profile-mimetype",
            ""),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES,
            "mime-property", "config-transcode.html#confval-profile-mimetype",
            ""),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_KEY,
            "attribute::key", "config-transcode.html#confval-profile-mimetype-key",
            ""),
        std::make_shared<ConfigEnumSetup<ResourceAttribute>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_RESOURCE,
            "attribute::resource", "config-transcode.html#confval-profile-mimetype-resource",
            ResourceAttribute::MAX,
            EnumMapper::mapAttributeName, EnumMapper::getAttributeName),
        std::make_shared<ConfigEnumSetup<MetadataFields>>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE_PROPERTIES_METADATA,
            "attribute::metadata", "config-transcode.html#confval-profile-mimetype-metadata",
            MetadataFields::M_MAX,
            MetaEnumMapper::remapMetaDataField, MetaEnumMapper::getMetaFieldName),
    };
}

std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::getSimpleOptions()
{
    return {
        std::make_shared<ConfigPathSetup>(ConfigVal::A_LOAD_SECTION_FROM_FILE,
            "attribute::from-file", "config.html#confval-from-file"),

        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
            "environ", "config-transcode.html#confval-environ"),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
            "name", "config-transcode.html#confval-environ-name"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
            "value", "config-transcode.html#confval-environ-value"),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION,
            "option", "config-server.html#confval-items-per-page-option"),
        std::make_shared<ConfigSetup>(ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT,
            "account", "config-server.html#confval-account"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP,
            "map", "config-import.html#confval-extension-mimetype-map"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_ADD_FILE,
            "add-file", "config-import.html#confval-resource-add-file"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_ADD_DIR,
            "add-dir", "config-import.html#confval-resource-add-dir"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_DATA,
            "add-data", "config-import.html#confval-auxdata-add-data"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LIBOPTS_COMMENT_DATA,
            "add-comment", "config-import.html#confval-library-comment"),
        std::make_shared<ConfigSetup>(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
            "transcode", "config-transcode.html#confval-transcode"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_MAPPING_PATH,
            "path", "config-import.html#confval-layout-path"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION,
            "script-option", "config-import.html#confval-script-option"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME,
            "name", "config-import.html#confval-script-option-name"),
        std::make_shared<ConfigStringSetup>(ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE,
            "value", "config-import.html#confval-script-option-value"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_GENRE,
            "genre", "config-import.html#confval-genre"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_MODEL,
            "model", "config-import.html#confval-model-map-model"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_HEADLINE,
            "headline", "config-import.html#confval-headline-map-headline"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_LAYOUT_HEADLINE_TYPE,
            "type", "config-import.html#confval-headline-map-headline-type"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_SYSTEM_DIR_ADD_PATH,
            "add-path", "config-import.html#confval-system-directories-add-path"),
        std::make_shared<ConfigSetup>(ConfigVal::A_DYNAMIC_CONTAINER,
            "container", "config-server.html#confval-containers-container"),
        std::make_shared<ConfigSetup>(ConfigVal::A_IMPORT_RESOURCES_HANDLER,
            "handler", "config-import.html#confval-handler"),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER,
            "header", "config-clients.html#confval-client-header"),
        std::make_shared<ConfigSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE,
            "dlna", "config-clients.html#confval-client-dlna"),
        std::make_shared<ConfigSetup>(ConfigVal::A_BOXLAYOUT_CHAIN_LINK,
            "link", "config-import.html#confval-link"),
        std::make_shared<ConfigBoolSetup>(ConfigVal::A_LIST_EXTEND,
            "attribute::extend", "config-general.html#confval-extend",
            NO),
    };
}

void ConfigDefinition::initOptions(const std::shared_ptr<ConfigDefinition>& self)
{
    auto serverOptions = getServerOptions();
    auto serverExtOptions = getServerExtendedOptions();
    auto clientOptions = getClientOptions();
    auto importOptions = getImportOptions();
    auto libraryOptions = getLibraryOptions();
    auto transcodingOptions = getTranscodingOptions();
    auto simpleOptions = getSimpleOptions();

    for (auto list : { serverOptions, serverExtOptions, clientOptions, importOptions, libraryOptions, transcodingOptions, simpleOptions }) {
        complexOptions.reserve(complexOptions.size() + list.size());
        complexOptions.insert(complexOptions.end(), list.begin(), list.end());
    }

    for (auto&& option : complexOptions) {
        option->setDefinition(self);
    }
}

/// @brief define parent options for path search
void ConfigDefinition::initHierarchy()
{
    parentOptions = {
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, {
                                                                ConfigVal::TRANSCODING_PROFILE_LIST,
                                                            } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, {
                                                                    ConfigVal::TRANSCODING_PROFILE_LIST,
                                                                } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, {
                                                               ConfigVal::TRANSCODING_PROFILE_LIST,
                                                           } },
        { ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, {
                                                             ConfigVal::TRANSCODING_PROFILE_LIST,
                                                         } },

        { ConfigVal::A_CLIENTS_UPNP_FILTER_FULL, {
                                                     ConfigVal::A_CLIENTS_CLIENT,
                                                     ConfigVal::CLIENTS_LIST,
                                                 } },
        { ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE, {
                                                     ConfigVal::A_CLIENTS_CLIENT,
                                                     ConfigVal::CLIENTS_LIST,
                                                 } },

        { ConfigVal::A_UPNP_PROPERTIES_PROPERTY, {
                                                     ConfigVal::UPNP_ALBUM_PROPERTIES,
                                                     ConfigVal::UPNP_GENRE_PROPERTIES,
                                                     ConfigVal::UPNP_ARTIST_PROPERTIES,
                                                     ConfigVal::UPNP_TITLE_PROPERTIES,
                                                     ConfigVal::UPNP_PLAYLIST_PROPERTIES,
                                                 } },
        { ConfigVal::A_UPNP_PROPERTIES_UPNPTAG, {
                                                    ConfigVal::UPNP_ALBUM_PROPERTIES,
                                                    ConfigVal::UPNP_GENRE_PROPERTIES,
                                                    ConfigVal::UPNP_ARTIST_PROPERTIES,
                                                    ConfigVal::UPNP_TITLE_PROPERTIES,
                                                    ConfigVal::UPNP_PLAYLIST_PROPERTIES,
                                                } },
        { ConfigVal::A_UPNP_PROPERTIES_METADATA, {
                                                     ConfigVal::UPNP_ALBUM_PROPERTIES,
                                                     ConfigVal::UPNP_GENRE_PROPERTIES,
                                                     ConfigVal::UPNP_ARTIST_PROPERTIES,
                                                     ConfigVal::UPNP_TITLE_PROPERTIES,
                                                     ConfigVal::UPNP_PLAYLIST_PROPERTIES,
                                                 } },

        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_PROPERTY, {
                                                           ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS,
                                                           ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS,
                                                           ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS,
                                                       } },
        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_TAG, {
                                                      ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS,
                                                      ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS,
                                                      ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS,
                                                  } },
        { ConfigVal::A_UPNP_DEFAULT_PROPERTY_VALUE, {
                                                        ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS,
                                                        ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS,
                                                        ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS,
                                                    } },

        { ConfigVal::A_UPNP_NAMESPACE_PROPERTY, {
                                                    ConfigVal::UPNP_ALBUM_NAMESPACES,
                                                    ConfigVal::UPNP_GENRE_NAMESPACES,
                                                    ConfigVal::UPNP_ARTIST_NAMESPACES,
                                                    ConfigVal::UPNP_TITLE_NAMESPACES,
                                                    ConfigVal::UPNP_PLAYLIST_NAMESPACES,
                                                } },
        { ConfigVal::A_UPNP_NAMESPACE_KEY, {
                                               ConfigVal::UPNP_ALBUM_NAMESPACES,
                                               ConfigVal::UPNP_GENRE_NAMESPACES,
                                               ConfigVal::UPNP_ARTIST_NAMESPACES,
                                               ConfigVal::UPNP_TITLE_NAMESPACES,
                                               ConfigVal::UPNP_PLAYLIST_NAMESPACES,
                                           } },
        { ConfigVal::A_UPNP_NAMESPACE_URI, {
                                               ConfigVal::UPNP_ALBUM_NAMESPACES,
                                               ConfigVal::UPNP_GENRE_NAMESPACES,
                                               ConfigVal::UPNP_ARTIST_NAMESPACES,
                                               ConfigVal::UPNP_TITLE_NAMESPACES,
                                               ConfigVal::UPNP_PLAYLIST_NAMESPACES,
                                           } },

        { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, {
                                                        ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                        ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                        ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                    } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, {
                                                    ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                    ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                    ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, {
                                                         ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                         ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, {
                                                         ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                         ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, {
                                                           ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                           ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                           ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                       } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS, {
                                                              ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                              ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                              ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                          } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_SCANCOUNT, {
                                                         ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                         ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                         ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                     } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT, {
                                                          ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                          ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                          ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                                      } },
        { ConfigVal::A_AUTOSCAN_DIRECTORY_LMT, {
                                                   ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST,
                                                   ConfigVal::IMPORT_AUTOSCAN_MANUAL_LIST,
#ifdef HAVE_INOTIFY
                                                   ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST,
#endif
                                               } },

        { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION, {
                                                       ConfigVal::IMPORT_DIRECTORIES_LIST,
                                                   } },
        { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE, {
                                                        ConfigVal::IMPORT_DIRECTORIES_LIST,
                                                    } },
        { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN, {
                                                     ConfigVal::IMPORT_DIRECTORIES_LIST,
                                                 } },
        { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE, {
                                                             ConfigVal::IMPORT_DIRECTORIES_LIST,
                                                         } },
        { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS, {
                                                              ConfigVal::IMPORT_DIRECTORIES_LIST,
                                                          } },

        { ConfigVal::A_DYNAMIC_CONTAINER, {
                                              ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                          } },
        { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, {
                                                       ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                                   } },
        { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE, {
                                                    ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                                } },
        { ConfigVal::A_DYNAMIC_CONTAINER_TITLE, {
                                                    ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                                } },
        { ConfigVal::A_DYNAMIC_CONTAINER_FILTER, {
                                                     ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                                 } },
        { ConfigVal::A_DYNAMIC_CONTAINER_SORT, {
                                                   ConfigVal::SERVER_DYNAMIC_CONTENT_LIST,
                                               } },

        { ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, {} },

        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, {
                                                                  ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                                  ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                              } },
        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, {
                                                               ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                               ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                           } },
        { ConfigVal::A_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, {
                                                            ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                            ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                        } },
        { ConfigVal::A_IMPORT_LIBOPTS_COMMENT_LABEL, {
#ifdef HAVE_LIBEXIF
                                                         ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
#endif
#ifdef HAVE_EXIV2
                                                         ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                         ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                         ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                         ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                         ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
#endif
                                                     } },
        { ConfigVal::A_IMPORT_LIBOPTS_COMMENT_TAG, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_TAG, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_AUXDATA_TAGS_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_LIBOPTS_AUXDATA_KEY, {
#ifdef HAVE_LIBEXIF
                                                       ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_EXIV2
                                                       ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_TAGLIB
                                                       ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_FFMPEG
                                                       ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_MATROSKA
                                                       ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST,
#endif
#ifdef HAVE_WAVPACK
                                                       ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
#endif
                                                   } },
        { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM, {
                                                          ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                                          ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                          ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
                                                          ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
                                                      } },
        { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO, {
                                                        ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                                        ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                                        ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING,
                                                        ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE,
                                                    } },

        { ConfigVal::A_IMPORT_RESOURCES_NAME, {
                                                  ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
                                                  ConfigVal::IMPORT_SYSTEM_DIRECTORIES,
                                                  ConfigVal::IMPORT_RESOURCES_ORDER,
                                              } },
        { ConfigVal::A_IMPORT_RESOURCES_EXT, {
                                                 ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
                                             } },
        { ConfigVal::A_IMPORT_RESOURCES_PTT, {
                                                 ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
                                                 ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
                                             } },
        { ConfigVal::A_IMPORT_RESOURCES_MIME, {
                                                  ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
                                                  ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST,
                                              } },

        { ConfigVal::A_IMPORT_LAYOUT_MAPPING_FROM, {
                                                       ConfigVal::IMPORT_LAYOUT_MAPPING,
                                                       ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
                                                       ConfigVal::IMPORT_SCRIPTING_IMPORT_MODEL_MAP,
                                                       ConfigVal::IMPORT_SCRIPTING_IMPORT_HEADLINE_MAP,
                                                   } },
        { ConfigVal::A_IMPORT_LAYOUT_MAPPING_TO, {
                                                     ConfigVal::IMPORT_LAYOUT_MAPPING,
                                                     ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
                                                     ConfigVal::IMPORT_SCRIPTING_IMPORT_MODEL_MAP,
                                                     ConfigVal::IMPORT_SCRIPTING_IMPORT_HEADLINE_MAP,
                                                 } },
#ifdef HAVE_JS
        { ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, {
                                                             ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
                                                         } },
        { ConfigVal::A_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE, {
                                                              ConfigVal::IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
                                                          } },
#endif
    };
}

/// @brief define option dependencies for automatic loading
void ConfigDefinition::initDependencies()
{
    dependencyMap = {
        { ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_RESTORE, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED },
        { ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE, ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE },
        { ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE, ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE },
        { ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE, ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE },
#ifdef HAVE_MYSQL
        { ConfigVal::SERVER_STORAGE_MYSQL_HOST, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_DATABASE, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_USERNAME, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_PORT, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_SOCKET, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_ENGINE, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_CHARSET, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_COLLATION, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE, ConfigVal::SERVER_STORAGE_MYSQL_DATABASE },
        { ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE, ConfigVal::SERVER_STORAGE_MYSQL_DATABASE },
        { ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE, ConfigVal::SERVER_STORAGE_MYSQL_DATABASE },
#endif
#ifdef HAVE_PGSQL
        { ConfigVal::SERVER_STORAGE_PGSQL_HOST, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_DATABASE, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_USERNAME, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_PORT, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_SOCKET, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_PASSWORD, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED },
        { ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE, ConfigVal::SERVER_STORAGE_PGSQL_DATABASE },
        { ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE, ConfigVal::SERVER_STORAGE_PGSQL_DATABASE },
        { ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE, ConfigVal::SERVER_STORAGE_PGSQL_DATABASE },
#endif
#ifdef HAVE_CURL
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, ConfigVal::TRANSCODING_TRANSCODING_ENABLED },
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE, ConfigVal::TRANSCODING_TRANSCODING_ENABLED },
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
#endif
#ifdef HAVE_LASTFM
        { ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
#ifndef HAVE_LASTFMLIB
        { ConfigVal::SERVER_EXTOPTS_LASTFM_SESSIONKEY, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_AUTHURL, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_SCROBBLEURL, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED },
#endif
#endif
    };
}

void ConfigDefinition::markDeprecated() const
{
    for (auto&& opt : deprecatedOptions) {
        auto co = findConfigSetup(opt);
        if (co)
            co->setDeprecated();
    }
}

const char* ConfigDefinition::mapConfigOption(ConfigVal option) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& c) { return c->option == option; });
    if (co != complexOptions.end()) {
        return (*co)->xpath;
    }
    return "";
}

bool ConfigDefinition::isDependent(ConfigVal option) const
{
    return (dependencyMap.find(option) != dependencyMap.end());
}

std::vector<ConfigVal> ConfigDefinition::getDependencies(ConfigVal option) const
{
    std::vector<ConfigVal> result;
    for (auto&& [key, val] : dependencyMap) {
        if (val == option)
            result.push_back(key);
    }
    return result;
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetup(ConfigVal option, bool save) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (save)
        return nullptr;

    throw_std_runtime_error("Error in config code: {} tag not found. ConfigVal::MAX={}", option, ConfigVal::MAX);
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetupByPath(
    const std::string& key,
    bool save,
    const std::shared_ptr<ConfigSetup>& parent) const
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) { return s->getUniquePath() == key; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (parent) {
        auto attrKey = key.substr(parent->getUniquePath().length());
        if (attrKey.find_first_of(']') != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find_first_of(']') + 1);
        }
        if (attrKey.find("attribute::") != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find("attribute::") + 11);
        }

        co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) {
            return s->getUniquePath() == attrKey //
                && (parentOptions.find(s->option) == parentOptions.end() //
                    || parentOptions.at(s->option).end() != std::find(parentOptions.at(s->option).begin(), parentOptions.at(s->option).end(), s->option));
        });
        if (co != complexOptions.end()) {
            log_debug("Config: attribute option found: '{}'", (*co)->xpath);
            return *co;
        }
    }

    if (save) {
        co = std::find_if(complexOptions.begin(), complexOptions.end(),
            [&](auto&& s) {
                auto uPath = s->getUniquePath();
                auto len = std::min(uPath.length(), key.length());
                return key.substr(0, len) == uPath.substr(0, len);
            });
        return (co != complexOptions.end()) ? *co : nullptr;
    }

    throw_std_runtime_error("Error in config code: {} tag not found", key);
}

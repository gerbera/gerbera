/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file config_definition.cc
///\brief Definitions of default values and setup for configuration.

#include "config_definition.h" // API

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "client_config.h"
#include "config_options.h"
#include "config_setup.h"
#include "metadata/metadata_handler.h"

// device description defaults

#define DESC_FRIENDLY_NAME "Gerbera"
#define DESC_MANUFACTURER "Gerbera Contributors"
#define DESC_MODEL_DESCRIPTION "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME "Gerbera"
#define DESC_MODEL_NUMBER GERBERA_VERSION
#define DESC_SERIAL_NUMBER "1"

// default values
#define DEFAULT_JS_CHARSET "UTF-8"

#define DEFAULT_TMPDIR "/tmp/"
#define DEFAULT_UI_EN_VALUE YES
#define DEFAULT_UI_SHOW_TOOLTIPS_VALUE YES
#define DEFAULT_POLL_WHEN_IDLE_VALUE NO
#define DEFAULT_POLL_INTERVAL 2
#define DEFAULT_ACCOUNTS_EN_VALUE NO
#define DEFAULT_ALIVE_INTERVAL 1800 // seconds
#define DEFAULT_BOOKMARK_FILE "gerbera.html"
#define DEFAULT_IGNORE_UNKNOWN_EXTENSIONS NO
#define DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS NO
#define DEFAULT_IMPORT_LAYOUT_PARENT_PATH NO
#define DEFAULT_PLAYLIST_CREATE_LINK YES
#define DEFAULT_HIDDEN_FILES_VALUE NO
#define DEFAULT_FOLLOW_SYMLINKS_VALUE YES
#define DEFAULT_RESOURCES_CASE_SENSITIVE YES
#define DEFAULT_UPNP_STRING_LIMIT (-1)
#define DEFAULT_SESSION_TIMEOUT 30
#define DEFAULT_PRES_URL_APPENDTO_ATTR "none"
#define DEFAULT_ITEMS_PER_PAGE 25
#define DEFAULT_LAYOUT_TYPE "builtin"
#define DEFAULT_HIDE_PC_DIRECTORY NO
#define DEFAULT_CLIENTS_EN_VALUE NO

#ifdef SOPCAST
#define DEFAULT_SOPCAST_ENABLED NO
#define DEFAULT_SOPCAST_UPDATE_AT_START NO
#endif

#ifdef ATRAILERS
#define DEFAULT_ATRAILERS_ENABLED NO
#define DEFAULT_ATRAILERS_UPDATE_AT_START NO
#define DEFAULT_ATRAILERS_REFRESH 43200
#define DEFAULT_ATRAILERS_RESOLUTION 640
#endif

#define DEFAULT_TRANSCODING_ENABLED NO

#define DEFAULT_SQLITE_SYNC "off"
#define DEFAULT_SQLITE_RESTORE "restore"
#define DEFAULT_SQLITE_BACKUP_ENABLED NO
#define DEFAULT_SQLITE_BACKUP_INTERVAL 600
#define DEFAULT_SQLITE_ENABLED YES

#ifdef HAVE_MYSQL
#define DEFAULT_MYSQL_HOST "localhost"
#define DEFAULT_MYSQL_DB "gerbera"
#define DEFAULT_MYSQL_USER "gerbera"
#define DEFAULT_MYSQL_ENABLED NO

#else //HAVE_MYSQL
#define DEFAULT_MYSQL_ENABLED NO
#endif

#define DEFAULT_SQLITE3_DB_FILENAME "gerbera.db"
#define DEFAULT_LIBOPTS_ENTRY_SEPARATOR "; "

#ifdef HAVE_CURL
#define DEFAULT_CURL_BUFFER_SIZE 262144
#define DEFAULT_CURL_INITIAL_FILL_SIZE 0
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
#define DEFAULT_FFMPEGTHUMBNAILER_ENABLED NO
#define DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE 128
#define DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE 5
#define DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY YES
#define DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS NO
#define DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY 8
#define DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED YES
#define DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR ""
#endif

#if defined(HAVE_LASTFMLIB)
#define DEFAULT_LASTFM_ENABLED NO
#define DEFAULT_LASTFM_USERNAME "lastfmuser"
#define DEFAULT_LASTFM_PASSWORD "lastfmpass"
#endif

#define DEFAULT_MARK_PLAYED_ITEMS_ENABLED NO
#define DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES YES
#define DEFAULT_MARK_PLAYED_ITEMS_STRING "*"

/// \brief default values for CFG_IMPORT_SYSTEM_DIRECTORIES
static std::vector<std::string> excludes_fullpath {
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

/// \brief default values for CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN
static std::vector<std::string> defaultItemsPerPage {
    "10",
    "25",
    "50",
    "100",
};

/// \brief default values for CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST
static std::map<std::string, std::string> mt_ct_defaults {
    { "audio/mpeg", CONTENT_TYPE_MP3 },
    { "application/ogg", CONTENT_TYPE_OGG },
    { "audio/ogg", CONTENT_TYPE_OGG },
    { "audio/x-flac", CONTENT_TYPE_FLAC },
    { "audio/flac", CONTENT_TYPE_FLAC },
    { "audio/x-ms-wma", CONTENT_TYPE_WMA },
    { "audio/x-wavpack", CONTENT_TYPE_WAVPACK },
    { "image/jpeg", CONTENT_TYPE_JPG },
    { "audio/x-mpegurl", CONTENT_TYPE_PLAYLIST },
    { "audio/x-scpls", CONTENT_TYPE_PLAYLIST },
    { "audio/x-wav", CONTENT_TYPE_PCM },
    { "audio/L16", CONTENT_TYPE_PCM },
    { "video/x-msvideo", CONTENT_TYPE_AVI },
    { "video/mp4", CONTENT_TYPE_MP4 },
    { "audio/mp4", CONTENT_TYPE_MP4 },
    { "video/x-matroska", CONTENT_TYPE_MKV },
    { "audio/x-matroska", CONTENT_TYPE_MKA },
    { "audio/x-dsd", CONTENT_TYPE_DSD },
};

/// \brief default values for CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST
static std::map<std::string, std::string> mt_upnp_defaults {
    { "audio/*", UPNP_CLASS_MUSIC_TRACK },
    { "video/*", UPNP_CLASS_VIDEO_ITEM },
    { "image/*", UPNP_CLASS_IMAGE_ITEM },
    { "application/ogg", UPNP_CLASS_MUSIC_TRACK },
};

/// \brief default values for CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST
static std::map<std::string, std::string> ext_mt_defaults {
    { "asf", "video/x-ms-asf" },
    { "asx", "video/x-ms-asf" },
    { "dff", "audio/x-dsd" },
    { "dsf", "audio/x-dsd" },
    { "flv", "video/x-flv" },
    { "m2ts", "video/mp2t" }, // LibMagic fails to identify MPEG2 Transport Streams
    { "m3u", "audio/x-mpegurl" },
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

/// \brief default values for ATTR_TRANSCODING_MIMETYPE_PROF_MAP
static std::map<std::string, std::string> tr_mt_defaults {
    { "video/x-flv", "vlcmpeg" },
    { "application/ogg", "vlcmpeg" },
    { "audio/ogg", "ogg2mp3" },
};

/// \brief default values for CFG_UPNP_ALBUM_PROPERTIES
static std::map<std::string, std::string> upnp_album_prop_defaults {
    { "dc:creator", "M_ALBUMARTIST" },
    { "upnp:artist", "M_ALBUMARTIST" },
    { "upnp:albumArtist", "M_ALBUMARTIST" },
    { "upnp:composer", "M_COMPOSER" },
    { "upnp:conductor", "M_CONDUCTOR" },
    { "upnp:orchestra", "M_ORCHESTRA" },
    { "upnp:date", "M_UPNP_DATE" },
    { "dc:date", "M_UPNP_DATE" },
    { "upnp:producer", "M_PRODUCER" },
    { "dc:publisher", "M_PUBLISHER" },
    { "upnp:genre", "M_GENRE" },
};

/// \brief default values for CFG_UPNP_ARTIST_PROPERTIES
static std::map<std::string, std::string> upnp_artist_prop_defaults {
    { "upnp:artist", "M_ALBUMARTIST" },
    { "upnp:albumArtist", "M_ALBUMARTIST" },
    { "upnp:genre", "M_GENRE" },
};

/// \brief configure all known options
const std::vector<std::shared_ptr<ConfigSetup>> ConfigDefinition::complexOptions = {
    std::make_shared<ConfigIntSetup>(CFG_SERVER_PORT,
        "/server/port", "config-server.html#port",
        0, ConfigIntSetup::CheckPortValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_IP,
        "/server/ip", "config-server.html#ip",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_NETWORK_INTERFACE,
        "/server/interface", "config-server.html#interface",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_NAME,
        "/server/name", "config-server.html#name",
        DESC_FRIENDLY_NAME),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER,
        "/server/manufacturer", "config-server.html#manufacturer",
        DESC_MANUFACTURER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER_URL,
        "/server/manufacturerURL", "config-server.html#manufacturerurl",
        DESC_MANUFACTURER_URL),
    std::make_shared<ConfigStringSetup>(CFG_VIRTUAL_URL,
        "/server/virtualURL", "config-server.html#virtualURL",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NAME,
        "/server/modelName", "config-server.html#modelname",
        DESC_MODEL_NAME),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_DESCRIPTION,
        "/server/modelDescription", "config-server.html#modeldescription",
        DESC_MODEL_DESCRIPTION),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NUMBER,
        "/server/modelNumber", "config-server.html#modelnumber",
        DESC_MODEL_NUMBER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_URL,
        "/server/modelURL", "config-server.html#modelurl",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_SERIAL_NUMBER,
        "/server/serialNumber", "config-server.html#serialnumber",
        DESC_SERIAL_NUMBER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_PRESENTATION_URL,
        "/server/presentationURL", "config-server.html#presentationurl", ""),
    std::make_shared<ConfigEnumSetup<std::string>>(CFG_SERVER_APPEND_PRESENTATION_URL_TO,
        "/server/presentationURL/attribute::append-to", "config-server.html#presentationurl",
        DEFAULT_PRES_URL_APPENDTO_ATTR,
        std::map<std::string, std::string>({ { "none", "none" }, { "ip", "ip" }, { "port", "port" } })),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_UDN,
        "/server/udn", "config-server.html#udn"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_HOME,
        "/server/home", "config-server.html#home"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_TMPDIR,
        "/server/tmpdir", "config-server.html#tmpdir",
        DEFAULT_TMPDIR),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_WEBROOT,
        "/server/webroot", "config-server.html#webroot"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_SERVEDIR,
        "/server/servedir", "config-server.html#servedir",
        ""),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_ALIVE_INTERVAL,
        "/server/alive", "config-server.html#alive",
        DEFAULT_ALIVE_INTERVAL, ALIVE_INTERVAL_MIN, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_HIDE_PC_DIRECTORY,
        "/server/pc-directory/attribute::upnp-hide", "config-server.html#pc-directory",
        DEFAULT_HIDE_PC_DIRECTORY),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_BOOKMARK_FILE,
        "/server/bookmark", "config-server.html#bookmark",
        DEFAULT_BOOKMARK_FILE, true, false),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
        "/server/upnp-string-limit", "config-server.html#upnp-string-limit",
        DEFAULT_UPNP_STRING_LIMIT, ConfigIntSetup::CheckUpnpStringLimitValue),

    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE,
        "/server/storage", "config-server.html#storage",
        true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_USE_TRANSACTIONS,
        "/server/storage/attribute::use-transactions", "config-server.html#storage",
        NO),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL,
        "/server/storage/mysql", "config-server.html#storage"),
#ifdef HAVE_MYSQL
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_MYSQL_ENABLED,
        "/server/storage/mysql/attribute::enabled", "config-server.html#storage",
        DEFAULT_MYSQL_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_HOST,
        "/server/storage/mysql/host", "config-server.html#storage",
        DEFAULT_MYSQL_HOST),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_MYSQL_PORT,
        "/server/storage/mysql/port", "config-server.html#storage",
        0, ConfigIntSetup::CheckPortValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_USERNAME,
        "/server/storage/mysql/username", "config-server.html#storage",
        DEFAULT_MYSQL_USER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_SOCKET,
        "/server/storage/mysql/socket", "config-server.html#storage",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_PASSWORD,
        "/server/storage/mysql/password", "config-server.html#storage",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_DATABASE,
        "/server/storage/mysql/database", "config-server.html#storage",
        DEFAULT_MYSQL_DB),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE,
        "/server/storage/mysql/init-sql-file", "config-server.html#storage",
        "", true), // This should really be "dataDir / mysql.sql"
#else
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_MYSQL_ENABLED,
        "/server/storage/mysql/attribute::enabled", "config-server.html#storage",
        NO),
#endif
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_SQLITE,
        "/server/storage/sqlite3", "config-server.html#storage"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_DRIVER,
        "/server/storage/driver", "config-server.html#storage"),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_ENABLED,
        "/server/storage/sqlite3/attribute::enabled", "config-server.html#storage",
        DEFAULT_SQLITE_ENABLED),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE,
        "/server/storage/sqlite3/database-file", "config-server.html#storage",
        DEFAULT_SQLITE3_DB_FILENAME, true, false),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS,
        "/server/storage/sqlite3/synchronous", "config-server.html#storage",
        DEFAULT_SQLITE_SYNC, ConfigIntSetup::CheckSqlLiteSyncValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_RESTORE,
        "/server/storage/sqlite3/on-error", "config-server.html#storage",
        DEFAULT_SQLITE_RESTORE, ConfigBoolSetup::CheckSqlLiteRestoreValue),
#ifdef SQLITE_BACKUP_ENABLED
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
        "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#storage",
        YES),
#else
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
        "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#storage",
        DEFAULT_SQLITE_BACKUP_ENABLED),
#endif
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
        "/server/storage/sqlite3/backup/attribute::interval", "config-server.html#storage",
        DEFAULT_SQLITE_BACKUP_INTERVAL, 1, ConfigIntSetup::CheckMinValue),

    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE,
        "/server/storage/sqlite3/init-sql-file", "config-server.html#storage",
        "", true), // This should really be "dataDir / sqlite3.sql"

    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLED,
        "/server/ui/attribute::enabled", "config-server.html#ui",
        DEFAULT_UI_EN_VALUE),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_POLL_INTERVAL,
        "/server/ui/attribute::poll-interval", "config-server.html#ui",
        DEFAULT_POLL_INTERVAL, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_POLL_WHEN_IDLE,
        "/server/ui/attribute::poll-when-idle", "config-server.html#ui",
        DEFAULT_POLL_WHEN_IDLE_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ACCOUNTS_ENABLED,
        "/server/ui/accounts/attribute::enabled", "config-server.html#ui",
        DEFAULT_ACCOUNTS_EN_VALUE),
    std::make_shared<ConfigDictionarySetup>(CFG_SERVER_UI_ACCOUNT_LIST,
        "/server/ui/accounts", "config-server.html#ui",
        ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_SESSION_TIMEOUT,
        "/server/ui/accounts/attribute::session-timeout", "config-server.html#ui",
        DEFAULT_SESSION_TIMEOUT, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
        "/server/ui/items-per-page/attribute::default", "config-server.html#ui",
        DEFAULT_ITEMS_PER_PAGE, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
        "/server/ui/items-per-page", "config-server.html#ui",
        ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, ConfigArraySetup::InitItemsPerPage, true,
        std::move(defaultItemsPerPage)),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_SHOW_TOOLTIPS,
        "/server/ui/attribute::show-tooltips", "config-server.html#ui",
        DEFAULT_UI_SHOW_TOOLTIPS_VALUE),

    std::make_shared<ConfigBoolSetup>(CFG_THREAD_SCOPE_SYSTEM,
        "/server/attribute::system-threads", "config-server.html#system-threads",
        YES),

    std::make_shared<ConfigClientSetup>(CFG_CLIENTS_LIST,
        "/clients", "config-clients.html#clients"),
    std::make_shared<ConfigBoolSetup>(CFG_CLIENTS_LIST_ENABLED,
        "/clients/attribute::enabled", "config-clients.html#clients",
        DEFAULT_CLIENTS_EN_VALUE),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT,
        "/clients/client", "config-clients.html#clients",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_FLAGS,
        "flags", "config-clients.html#client",
        true),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP,
        "ip", "config-clients.html#client",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT,
        "userAgent", "config-clients.html#client",
        ""),

    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_HIDDEN_FILES,
        "/import/attribute::hidden-files", "config-import.html#import",
        DEFAULT_HIDDEN_FILES_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_FOLLOW_SYMLINKS,
        "/import/attribute::follow-symlinks", "config-import.html#import",
        DEFAULT_FOLLOW_SYMLINKS_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_READABLE_NAMES,
        "/import/attribute::readable-names", "config-import.html#import",
        YES),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
        "/import/mappings/extension-mimetype", "config-import.html#extension-mimetype",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, true, std::move(ext_mt_defaults)),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
        "/import/mappings/extension-mimetype/attribute::ignore-unknown", "config-import.html#extension-mimetype",
        DEFAULT_IGNORE_UNKNOWN_EXTENSIONS),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
        "/import/mappings/extension-mimetype/attribute::case-sensitive", "config-import.html#extension-mimetype",
        DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
        "/import/mappings/mimetype-upnpclass", "config-import.html#mime-type-upnpclass",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, true, std::move(mt_upnp_defaults)),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
        "/import/mappings/mimetype-contenttype", "config-import.html#mime-type-upnpclass",
        ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
        false, false, true, std::move(mt_ct_defaults)),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LAYOUT_MAPPING,
        "/import/layout", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_MAPPING_PATH, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_LAYOUT_PARENT_PATH,
        "/import/layout/attribute::parent-path", "config-import.html#layout",
        DEFAULT_IMPORT_LAYOUT_PARENT_PATH),
#ifdef HAVE_JS
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_CHARSET,
        "/import/scripting/attribute::script-charset", "config-import.html#scripting",
        DEFAULT_JS_CHARSET),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT,
        "/import/scripting/common-script", "config-import.html#common-script",
        "", true, false),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_CUSTOM_SCRIPT,
        "/import/scripting/custom-script", "config-import.html#custom-script",
        "", true),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
        "/import/scripting/playlist-script", "config-import.html#playlist-script",
        "", true),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
        "/import/scripting/playlist-script/attribute::create-link", "config-import.html#playlist-script",
        DEFAULT_PLAYLIST_CREATE_LINK),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT,
        "/import/scripting/virtual-layout/import-script", "config-import.html#scripting",
        "", true),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_AUDIO,
        "/import/scripting/virtual-layout/attribute::audio-layout", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_VIDEO,
        "/import/scripting/virtual-layout/attribute::video-layout", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_IMAGE,
        "/import/scripting/virtual-layout/attribute::image-layout", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_TRAILER,
        "/import/scripting/virtual-layout/attribute::trailer-layout", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
        "/import/scripting/virtual-layout/script-options", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_SCRIPT_OPTION, ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE),

    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS,
        "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_ALBUMBOX,
        "/import/scripting/virtual-layout/structured-layout/attribute::album-box", "config-import.html#scripting",
        6),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_ARTISTBOX,
        "/import/scripting/virtual-layout/structured-layout/attribute::artist-box", "config-import.html#scripting",
        9),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_GENREBOX,
        "/import/scripting/virtual-layout/structured-layout/attribute::genre-box", "config-import.html#scripting",
        6),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_TRACKBOX,
        "/import/scripting/virtual-layout/structured-layout/attribute::track-box", "config-import.html#scripting",
        6),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR,
        "/import/scripting/virtual-layout/structured-layout/attribute::div-char", "config-import.html#scripting",
        ""),
#endif // JS
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
        "/import/scripting/virtual-layout/genre-map", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_GENRE, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_FILESYSTEM_CHARSET,
        "/import/filesystem-charset", "config-import.html#filesystem-charset",
        DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_METADATA_CHARSET,
        "/import/metadata-charset", "config-import.html#metadata-charset",
        DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_PLAYLIST_CHARSET,
        "/import/playlist-charset", "config-import.html#playlist-charset",
        DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigEnumSetup<std::string>>(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
        "/import/scripting/virtual-layout/attribute::type", "config-import.html#scripting",
        DEFAULT_LAYOUT_TYPE,
        std::map<std::string, std::string>({ { "js", "js" }, { "builtin", "builtin" }, { "disabled", "disabled" } })),

    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_TRANSCODING_ENABLED,
        "/transcoding/attribute::enabled", "config-transcode.html#transcoding",
        DEFAULT_TRANSCODING_ENABLED),
    std::make_shared<ConfigTranscodingSetup>(CFG_TRANSCODING_PROFILE_LIST,
        "/transcoding", "config-transcode.html#transcoding"),

    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ENTRY_SEP,
        "/import/library-options/attribute::multi-value-separator", "config-import.html#library-options",
        DEFAULT_LIBOPTS_ENTRY_SEPARATOR),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP,
        "/import/library-options/attribute::legacy-value-separator", "config-import.html#library-options",
        ""),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_RESOURCES_CASE_SENSITIVE,
        "/import/resources/attribute::case-sensitive", "config-import.html#resources",
        DEFAULT_RESOURCES_CASE_SENSITIVE),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_FANART_FILE_LIST,
        "/import/resources/fanart", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),

    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
        "/import/resources/container", "config-import.html#container",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION,
        "/import/resources/container/attribute::location", "config-import.html#container",
        ""),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT,
        "/import/resources/container/attribute::parentCount", "config-import.html#container",
        2, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_IMPORT_RESOURCES_CONTAINERART_MINDEPTH,
        "/import/resources/container/attribute::minDepth", "config-import.html#container",
        2, 0, ConfigIntSetup::CheckMinValue),

    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
        "/import/resources/subtitle", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST,
        "/import/resources/resource", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_SYSTEM_DIRECTORIES,
        "/import/system-directories", "config-import.html#system-directories",
        ATTR_IMPORT_SYSTEM_DIR_ADD_PATH, ATTR_IMPORT_RESOURCES_NAME, true, true,
        std::move(excludes_fullpath)),

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
        "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
        "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
        "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS,
        "/server/extended-runtime-options/ffmpegthumbnailer/workaround-bugs", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
        "/server/extended-runtime-options/ffmpegthumbnailer/image-quality", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY, ConfigIntSetup::CheckImageQualityValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, // ConfigPathSetup
        "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir", "config-extended.html#ffmpegthumbnailer",
        DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR),
#endif

    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
        "/server/extended-runtime-options/mark-played-items/attribute::enabled", "config-extended.html#extended-runtime-options",
        DEFAULT_MARK_PLAYED_ITEMS_ENABLED),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
        "/server/extended-runtime-options/mark-played-items/string/attribute::mode", "config-extended.html#extended-runtime-options",
        DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE, ConfigBoolSetup::CheckMarkPlayedValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
        "/server/extended-runtime-options/mark-played-items/string", "config-extended.html#extended-runtime-options",
        false, DEFAULT_MARK_PLAYED_ITEMS_STRING, true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
        "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates", "config-extended.html#extended-runtime-options",
        DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
        "/server/extended-runtime-options/mark-played-items/mark", "config-extended.html#extended-runtime-options",
        ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, ConfigArraySetup::InitPlayedItemsMark),
#ifdef HAVE_LASTFMLIB
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_LASTFM_ENABLED,
        "/server/extended-runtime-options/lastfm/attribute::enabled", "config-extended.html#lastfm",
        DEFAULT_LASTFM_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_USERNAME,
        "/server/extended-runtime-options/lastfm/username", "config-extended.html#lastfm",
        false, DEFAULT_LASTFM_USERNAME, true),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD,
        "/server/extended-runtime-options/lastfm/password", "config-extended.html#lastfm",
        false, DEFAULT_LASTFM_PASSWORD, true),
#endif
#ifdef SOPCAST
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_SOPCAST_ENABLED,
        "/import/online-content/SopCast/attribute::enabled", "config-online.html#sopcast",
        DEFAULT_SOPCAST_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_SOPCAST_REFRESH,
        "/import/online-content/SopCast/attribute::refresh", "config-online.html#sopcast",
        0),
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START,
        "/import/online-content/SopCast/attribute::update-at-start", "config-online.html#sopcast",
        DEFAULT_SOPCAST_UPDATE_AT_START),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER,
        "/import/online-content/SopCast/attribute::purge-after", "config-online.html#sopcast",
        0),
#endif
#ifdef ATRAILERS
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED,
        "/import/online-content/AppleTrailers/attribute::enabled", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH,
        "/import/online-content/AppleTrailers/attribute::refresh", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START,
        "/import/online-content/AppleTrailers/attribute::update-at-start", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_UPDATE_AT_START),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER,
        "/import/online-content/AppleTrailers/attribute::purge-after", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigEnumSetup<std::string>>(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION,
        "/import/online-content/AppleTrailers/attribute::resolution", "config-online.html#appletrailers",
        fmt::to_string(DEFAULT_ATRAILERS_RESOLUTION).c_str(),
        std::map<std::string, std::string>({ { "640", "640" }, { "720", "720p" }, { "720p", "720p" } })),
#endif

#ifdef HAVE_INOTIFY
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_AUTOSCAN_USE_INOTIFY,
        "/import/autoscan/attribute::use-inotify", "config-import.html#autoscan",
        "auto", ConfigBoolSetup::CheckInotifyValue),
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        ScanMode::INotify),
#endif
    std::make_shared<ConfigSetup>(ATTR_AUTOSCAN_DIRECTORY,
        "directory", "config-import.html#autoscan",
        ""),
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_TIMED_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        ScanMode::Timed),
    std::make_shared<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION,
        "attribute::location", "config-import.html#autoscan",
        "", false, true, true),
    std::make_shared<ConfigEnumSetup<ScanMode>>(ATTR_AUTOSCAN_DIRECTORY_MODE,
        "attribute::mode", "config-import.html#autoscan",
        std::map<std::string, ScanMode>({ { "timed", ScanMode::Timed }, { "inotify", ScanMode::INotify } })),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL,
        "attribute::interval", "config-import.html#autoscan",
        -1, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE,
        "attribute::recursive", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES,
        "attribute::hidden-files", "config-import.html#autoscan"),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT,
        "attribute::scan-count", "config-import.html#autoscan"),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT,
        "attribute::task-count", "config-import.html#autoscan"),
    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_DIRECTORY_LMT,
        "attribute::last-modified", "config-import.html#autoscan"),

#ifdef HAVE_CURL
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
        "/transcoding/attribute::fetch-buffer-size", "config-transcode.html#transcoding",
        DEFAULT_CURL_BUFFER_SIZE, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
        "/transcoding/attribute::fetch-buffer-fill-size", "config-transcode.html#transcoding",
        DEFAULT_CURL_INITIAL_FILL_SIZE, 0, ConfigIntSetup::CheckMinValue),
#endif //HAVE_CURL
#ifdef HAVE_LIBEXIF
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
        "/import/library-options/libexif/auxdata", "config-import.html#auxdata",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_EXIF_CHARSET,
        "/import/library-options/libexif/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_EXIV2
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
        "/import/library-options/exiv2/auxdata", "config-import.html#library-options",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_EXIV2_CHARSET,
        "/import/library-options/exiv2/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_TAGLIB
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
        "/import/library-options/id3/auxdata", "config-import.html#id2",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ID3_CHARSET,
        "/import/library-options/id3/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_FFMPEG
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
        "/import/library-options/ffmpeg/auxdata", "config-import.html#id5",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET,
        "/import/library-options/ffmpeg/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_MAGIC
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_MAGIC_FILE,
        "/import/magic-file", "config-import.html#magic-file",
        ""),
#endif

    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
        "/transcoding/mimetype-profile-mappings/attribute::allow-unused", "config-transcode.html#mimetype-profile-mappings",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
        "/transcoding/profiles/attribute::allow-unused", "config-transcode.html#profiles",
        NO),
    std::make_shared<ConfigEnumSetup<transcoding_type_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE,
        "attribute::type", "config-transcode.html#profiles",
        std::map<std::string, transcoding_type_t>({ { "none", TR_None }, { "external", TR_External }, /* for the future...{"remote", TR_Remote}*/ })),
    std::make_shared<ConfigEnumSetup<avi_fourcc_listmode_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
        "mode", "config-transcode.html#profiles",
        std::map<std::string, avi_fourcc_listmode_t>({ { "ignore", FCC_Ignore }, { "process", FCC_Process }, { "disabled", FCC_None } })),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING,
        "attribute::using", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
        "fourcc", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED,
        "attribute::enabled", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL,
        "accept-url", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG,
        "hide-original-resource", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB,
        "thumbnail", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST,
        "first-resource", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG,
        "accept-ogg-theora", "config-transcode.html#profiles"),
    std::make_shared<ConfigArraySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC,
        "avi-fourcc-list", "config-transcode.html#profiles",
        ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, CFG_MAX, true, true),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE,
        "attribute::size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK,
        "attribute::chunk-size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL,
        "attribute::fill-size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ,
        "sample-frequency", "config-transcode.html#profiles",
        "-1", ConfigIntSetup::CheckProfileNumberValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN,
        "audio-channels", "config-transcode.html#profiles",
        "-1", ConfigIntSetup::CheckProfileNumberValue),
    std::make_shared<ConfigDictionarySetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP,
        "/transcoding/mimetype-profile-mappings", "config-transcode.html#profiles",
        ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING,
        false, false, false, std::move(tr_mt_defaults)),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE,
        "/transcoding/profiles/profile", "config-server.html#ui",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NAME,
        "attribute::name", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
        "mimetype", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
        "attribute::command", "config-transcode.html#profiles",
        "", ConfigPathSetup::checkAgentPath, true, true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
        "attribute::arguments", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_RES,
        "resolution", "config-transcode.html#profiles",
        false),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT,
        "agent", "config-transcode.html#profiles",
        true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER,
        "buffer", "config-transcode.html#profiles",
        true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE,
        "attribute::mimetype", "config-transcode.html#profiles",
        ""),

    std::make_shared<ConfigDirectorySetup>(CFG_IMPORT_DIRECTORIES_LIST,
        "/import/directories", "config-import.html#autoscan"),
    std::make_shared<ConfigSetup>(ATTR_DIRECTORIES_TWEAK,
        "/import/directories/tweak", "config-import.html#autoscan",
        ""),
    std::make_shared<ConfigPathSetup>(ATTR_DIRECTORIES_TWEAK_LOCATION,
        "attribute::location", "config-import.html#autoscan",
        "", false, true, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_INHERIT,
        "attribute::inherit", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_RECURSIVE,
        "attribute::recursive", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_HIDDEN,
        "attribute::hidden-files", "config-import.html#autoscan"),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE,
        "attribute::case-sensitive", "config-import.html#autoscan",
        DEFAULT_RESOURCES_CASE_SENSITIVE),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS,
        "attribute::follow-symlinks", "config-import.html#autoscan",
        DEFAULT_FOLLOW_SYMLINKS_VALUE),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_META_CHARSET,
        "attribute::meta-charset", "config-import.html#charset",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_FANART_FILE,
        "attribute::fanart-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE,
        "attribute::subtitle-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE,
        "attribute::resource-file", "config-import.html#resources",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT,
        "treat", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE,
        "attribute::mimetype", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
        "attribute::as", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM,
        "attribute::from", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        "attribute::to", "config-import.html#mappings",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_RESOURCES_NAME,
        "attribute::name", "config-import.html#resources",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_FROM,
        "attribute::from", "config-import.html#layout",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_TO,
        "attribute::to", "config-import.html#layout",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_TAG,
        "attribute::tag", "config-import.html#auxdata",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD,
        "attribute::password", "config-server.html#ui",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_USER,
        "attribute::user", "config-server.html#ui",
        ""),

    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ALBUM_PROPERTIES,
        "/server/upnp/album-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, std::move(upnp_album_prop_defaults)),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ARTIST_PROPERTIES,
        "/server/upnp/artist-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, std::move(upnp_artist_prop_defaults)),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_TITLE_PROPERTIES,
        "/server/upnp/title-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, false),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_PROPERTIES_UPNPTAG,
        "attribute::upnp-tag", "config-server.html#upnp"),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_PROPERTIES_METADATA,
        "attribute::meta-data", "config-server.html#upnp"),

    // simpleOptions

    std::make_shared<ConfigSetup>(ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION,
        "option", ""),
    std::make_shared<ConfigSetup>(ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT,
        "content", ""),
    std::make_shared<ConfigSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT,
        "account", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP,
        "map", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_RESOURCES_ADD_FILE,
        "add-file", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_DATA,
        "add-data", ""),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
        "transcode", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_MAPPING_PATH,
        "path", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_SCRIPT_OPTION,
        "script-option", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_GENRE,
        "genre", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_SYSTEM_DIR_ADD_PATH,
        "add-path", ""),

    std::make_shared<ConfigSetup>(ATTR_UPNP_PROPERTIES_PROPERTY,
        "upnp-property", ""),
};

/// \brief define parent options for path search
const std::map<config_option_t, std::vector<config_option_t>> ConfigDefinition::parentOptions = {
    { ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, { CFG_TRANSCODING_PROFILE_LIST } },

    { ATTR_UPNP_PROPERTIES_PROPERTY, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES } },
    { ATTR_UPNP_PROPERTIES_UPNPTAG, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES } },
    { ATTR_UPNP_PROPERTIES_METADATA, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES } },

    { ATTR_AUTOSCAN_DIRECTORY_LOCATION, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                            CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                        } },
    { ATTR_AUTOSCAN_DIRECTORY_MODE, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                        CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                    } },
    { ATTR_AUTOSCAN_DIRECTORY_RECURSIVE, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                             CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                         } },
    { ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                               CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                           } },
    { ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                             CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                         } },
    { ATTR_AUTOSCAN_DIRECTORY_LMT, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                       CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                   } },

    { ATTR_DIRECTORIES_TWEAK_LOCATION, { CFG_IMPORT_DIRECTORIES_LIST } },
    { ATTR_DIRECTORIES_TWEAK_RECURSIVE, { CFG_IMPORT_DIRECTORIES_LIST } },
    { ATTR_DIRECTORIES_TWEAK_HIDDEN, { CFG_IMPORT_DIRECTORIES_LIST } },
    { ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE, { CFG_IMPORT_DIRECTORIES_LIST } },
    { ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS, { CFG_IMPORT_DIRECTORIES_LIST } },

    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, {} },

    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST } },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST } },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST } },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, { CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST } },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_TO, { CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST } },

    { ATTR_IMPORT_RESOURCES_NAME, { CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, CFG_IMPORT_SYSTEM_DIRECTORIES } },

    { ATTR_IMPORT_LAYOUT_MAPPING_FROM, { CFG_IMPORT_LAYOUT_MAPPING, CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
    { ATTR_IMPORT_LAYOUT_MAPPING_TO, { CFG_IMPORT_LAYOUT_MAPPING, CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
#ifdef HAVE_JS
    { ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, { CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
    { ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE, { CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
#endif
};

const char* ConfigDefinition::mapConfigOption(config_option_t option)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& c) { return c->option == option; });
    if (co != complexOptions.end()) {
        return (*co)->xpath;
    }
    return "";
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetup(config_option_t option, bool save)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (save)
        return nullptr;

    throw_std_runtime_error("Error in config code: {} tag not found", option);
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetupByPath(const std::string& key, bool save, const std::shared_ptr<ConfigSetup>& parent)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) { return s->getUniquePath() == key; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (parent != nullptr) {
        auto attrKey = key.substr(parent->getUniquePath().length());
        if (attrKey.find_first_of(']') != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find_first_of(']') + 1);
        }
        if (attrKey.find_first_of("attribute::") != std::string::npos) {
            attrKey = attrKey.substr(attrKey.find_first_of("attribute::") + 11);
        }

        co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](auto&& s) { return s->getUniquePath() == attrKey && (parentOptions.find(s->option) == parentOptions.end() || parentOptions.at(s->option).end() != std::find(parentOptions.at(s->option).begin(), parentOptions.at(s->option).end(), s->option)); });
        if (co != complexOptions.end()) {
            log_debug("Config: attribute option found: '{}'", (*co)->xpath);
            return *co;
        }
    }

    if (save) {
        co = std::find_if(complexOptions.begin(), complexOptions.end(),
            [&](auto&& s) {
                auto uPath = s->getUniquePath();
                size_t len = std::min(uPath.length(), key.length());
                return key.substr(0, len) == uPath.substr(0, len);
            });
        return (co != complexOptions.end()) ? *co : nullptr;
    }

    throw_std_runtime_error("Error in config code: {} tag not found", key);
}

/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.

    Copyright (C) 2020-2023 Gerbera Contributors

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
#include "config/setup/config_setup_autoscan.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_client.h"
#include "config/setup/config_setup_dynamic.h"
#include "config/setup/config_setup_enum.h"
#include "config/setup/config_setup_time.h"
#include "config/setup/config_setup_transcoding.h"
#include "config/setup/config_setup_tweak.h"
#include "config_option_enum.h"
#include "config_options.h"
#include "config_setup.h"
#include "content/autoscan.h"
#include "content/content_manager.h"
#include "content/layout/box_layout.h"
#include "database/sqlite3/sqlite_config.h"
#include "metadata/metadata_handler.h"
#include "upnp_common.h"

// default values

#define DEFAULT_FOLLOW_SYMLINKS_VALUE YES
#define DEFAULT_RESOURCES_CASE_SENSITIVE YES
#define DEFAULT_UPNP_STRING_LIMIT (-1)

#ifdef ATRAILERS
#define DEFAULT_ATRAILERS_REFRESH 43200
#endif

#define DEFAULT_LIBOPTS_ENTRY_SEPARATOR "; "

/// \brief default values for CFG_IMPORT_SYSTEM_DIRECTORIES
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

/// \brief default values for CFG_IMPORT_RESOURCES_FANART_FILE_LIST
static const std::vector<std::string> defaultFanArtFile {
    // "%title%.jpg",
    // "%filename%.jpg",
    // "folder.jpg",
    // "poster.jpg",
    // "cover.jpg",
    // "albumartsmall.jpg",
    // "%album%.jpg",
};

/// \brief default values for CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST
static const std::vector<std::string> defaultContainerArtFile {
    // "folder.jpg",
    // "poster.jpg",
    // "cover.jpg",
    // "albumartsmall.jpg",
};

/// \brief default values for CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST
static const std::vector<std::string> defaultSubtitleFile {
    // "%title%.srt",
    // "%filename%.srt"
};

/// \brief default values for CFG_IMPORT_RESOURCES_METAFILE_FILE_LIST
static const std::vector<std::string> defaultMetadataFile {
    // "%filename%.nfo"
};

/// \brief default values for CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST
static const std::vector<std::string> defaultResourceFile {
    // "%filename%.srt",
    // "cover.jpg",
    // "%album%.jpg",
};

/// \brief default values for CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN
static const std::vector<std::string> defaultItemsPerPage {
    "10",
    "25",
    "50",
    "100",
};

/// \brief default values for CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST
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

/// \brief default values for CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST
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

/// \brief default values for CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST
static const std::map<std::string, std::string> mtUpnpDefaults {
    { "application/ogg", UPNP_CLASS_MUSIC_TRACK },
    { "audio/*", UPNP_CLASS_MUSIC_TRACK },
    { "image/*", UPNP_CLASS_IMAGE_ITEM },
    { "video/*", UPNP_CLASS_VIDEO_ITEM },
};

/// \brief default values for CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST
static const std::map<std::string, std::string> mtTransferDefaults {
    { "application/ogg", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "audio/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "image/*", UPNP_DLNA_TRANSFER_MODE_INTERACTIVE },
    { "video/*", UPNP_DLNA_TRANSFER_MODE_STREAMING },
    { "text/*", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
    { "application/x-srt", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
    { "srt", UPNP_DLNA_TRANSFER_MODE_BACKGROUND },
};

/// \brief default values for CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST
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

/// \brief default values for CFG_IMPORT_MAPPINGS_IGNORED_EXTENSIONS
static const std::vector<std::string> ignoreDefaults {
    "part",
    "tmp",
};

/// \brief default values for ATTR_TRANSCODING_MIMETYPE_PROF_MAP
static const std::map<std::string, std::string> trMtDefaults {
    { "video/x-flv", "vlcmpeg" },
    { "application/ogg", "vlcmpeg" },
    { "audio/ogg", "ogg2mp3" },
};

/// \brief default values for CFG_UPNP_SEARCH_SEGMENTS
static const std::vector<std::string> upnpSearchSegmentDefaults {
    "M_ARTIST",
    "M_TITLE",
};

/// \brief default values for CFG_UPNP_ALBUM_PROPERTIES
static const std::map<std::string, std::string> upnpAlbumPropDefaults {
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
static const std::map<std::string, std::string> upnpArtistPropDefaults {
    { "upnp:artist", "M_ALBUMARTIST" },
    { "upnp:albumArtist", "M_ALBUMARTIST" },
    { "upnp:genre", "M_GENRE" },
};

/// \brief default values for CFG_UPNP_GENRE_PROPERTIES
static const std::map<std::string, std::string> upnpGenrePropDefaults {
    { "upnp:genre", "M_GENRE" },
};

/// \brief default values for CFG_UPNP_PLAYLIST_PROPERTIES
static const std::map<std::string, std::string> upnpPlaylistPropDefaults {
    { "dc:date", "M_UPNP_DATE" },
};

/// \brief default values for CFG_IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST
static const std::map<std::string, std::string> id3SpecialPropertyMap {
    { "PERFORMER", "upnp:artist@role[Performer]" },
};

/// \brief default values for CFG_IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST
static const std::map<std::string, std::string> ffmpegSpecialPropertyMap {
    { "performer", "upnp:artist@role[Performer]" },
};

/// \brief default values for CFG_IMPORT_VIRTUAL_DIRECTORY_KEYS
static const std::vector<std::string> virtualDirectoryKeys {
    "M_ALBUMARTIST",
    "M_UPNP_DATE",
};

/// \brief default values for CFG_BOXLAYOUT_BOX
static const std::vector<BoxLayout> boxLayoutDefaults {
    BoxLayout("Audio/allAlbums", "Albums", "object.container"),
    BoxLayout("Audio/allArtists", "Artists", "object.container"),
    BoxLayout("Audio/allAudio", "All Audio", "object.container"),
    BoxLayout("Audio/allComposers", "Composers", "object.container"),
    BoxLayout("Audio/allDirectories", "Directories", "object.container"),
    BoxLayout("Audio/allGenres", "Genres", "object.container"),
    BoxLayout("Audio/allSongs", "All Songs", "object.container"),
    BoxLayout("Audio/allTracks", "All - full name", "object.container"),
    BoxLayout("Audio/allYears", "Year", "object.container"),
    BoxLayout("Audio/audioRoot", "Audio", "object.container"),
    BoxLayout("Audio/artistChronology", "Album Chronology", "object.container"),
    BoxLayout("AudioInitial/abc", "ABC", "object.container"),
    BoxLayout("AudioInitial/allArtistTracks", "000 All", "object.container"),
    BoxLayout("AudioInitial/allBooks", "Books", "object.container"),
    BoxLayout("AudioInitial/audioBookRoot", "AudioBooks", "object.container"),
    BoxLayout("AudioStructured/allAlbums", "-Album-", "object.container", true, 6),
    BoxLayout("AudioStructured/allArtistTracks", "all", "object.container"),
    BoxLayout("AudioStructured/allArtists", "-Artist-", "object.container", true, 9),
    BoxLayout("AudioStructured/allGenres", "-Genre-", "object.container", true, 6),
    BoxLayout("AudioStructured/allTracks", "-Track-", "object.container", true, 6),
    BoxLayout("AudioStructured/allYears", "-Year-", "object.container"),
    BoxLayout("Video/allDates", "Date", "object.container"),
    BoxLayout("Video/allDirectories", "Directories", "object.container"),
    BoxLayout("Video/allVideo", "All Video", "object.container"),
    BoxLayout("Video/allYears", "Year", "object.container"),
    BoxLayout("Video/unknown", "Unknown", "object.container"),
    BoxLayout("Video/videoRoot", "Video", "object.container"),
    BoxLayout("Image/allDates", "Date", "object.container"),
    BoxLayout("Image/allDirectories", "Directories", "object.container"),
    BoxLayout("Image/allImages", "All Photos", "object.container"),
    BoxLayout("Image/allYears", "Year", "object.container"),
    BoxLayout("Image/imageRoot", "Photos", "object.container"),
    BoxLayout("Image/unknown", "Unknown", "object.container"),
    BoxLayout("Trailer/trailerRoot", "Online Services", "object.container"),
    BoxLayout("Trailer/appleTrailers", "Apple Trailers", "object.container"),
    BoxLayout("Trailer/allTrailers", "All Trailers", "object.container"),
    BoxLayout("Trailer/allGenres", "Genres", "object.container"),
    BoxLayout("Trailer/relDate", "Release Date", "object.container"),
    BoxLayout("Trailer/postDate", "Post Date", "object.container"),
    BoxLayout("Trailer/unknown", "Unknown", "object.container"),
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
        "Gerbera"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER,
        "/server/manufacturer", "config-server.html#manufacturer",
        "Gerbera Contributors"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER_URL,
        "/server/manufacturerURL", "config-server.html#manufacturerurl",
        DESC_MANUFACTURER_URL),
    std::make_shared<ConfigStringSetup>(CFG_VIRTUAL_URL,
        "/server/virtualURL", "config-server.html#virtualURL",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_EXTERNAL_URL,
        "/server/externalURL", "config-server.html#externalURL",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NAME,
        "/server/modelName", "config-server.html#modelname",
        "Gerbera"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_DESCRIPTION,
        "/server/modelDescription", "config-server.html#modeldescription",
        "Free UPnP AV MediaServer, GNU GPL"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NUMBER,
        "/server/modelNumber", "config-server.html#modelnumber",
        GERBERA_VERSION),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_URL,
        "/server/modelURL", "config-server.html#modelurl",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_SERIAL_NUMBER,
        "/server/serialNumber", "config-server.html#serialnumber",
        "1"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_PRESENTATION_URL,
        "/server/presentationURL", "config-server.html#presentationurl", ""),
    std::make_shared<ConfigEnumSetup<UrlAppendMode>>(CFG_SERVER_APPEND_PRESENTATION_URL_TO,
        "/server/presentationURL/attribute::append-to", "config-server.html#presentationurl",
        UrlAppendMode::none,
        std::map<std::string, UrlAppendMode>({ { "none", UrlAppendMode::none }, { "ip", UrlAppendMode::ip }, { "port", UrlAppendMode::port } })),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_UDN,
        "/server/udn", "config-server.html#udn"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_HOME,
        "/server/home", "config-server.html#home"),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_HOME_OVERRIDE,
        "/server/home/attribute::override", "config-server.html#home",
        NO),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_TMPDIR,
        "/server/tmpdir", "config-server.html#tmpdir",
        "/tmp/"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_WEBROOT,
        "/server/webroot", "config-server.html#webroot"),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_ALIVE_INTERVAL,
        "/server/alive", "config-server.html#alive",
        180, ALIVE_INTERVAL_MIN, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_HIDE_PC_DIRECTORY,
        "/server/pc-directory/attribute::upnp-hide", "config-server.html#pc-directory",
        NO),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_BOOKMARK_FILE,
        "/server/bookmark", "config-server.html#bookmark",
        "gerbera.html", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
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
        NO),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_HOST,
        "/server/storage/mysql/host", "config-server.html#storage",
        "localhost"),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_MYSQL_PORT,
        "/server/storage/mysql/port", "config-server.html#storage",
        0, ConfigIntSetup::CheckPortValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_USERNAME,
        "/server/storage/mysql/username", "config-server.html#storage",
        "gerbera"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_SOCKET,
        "/server/storage/mysql/socket", "config-server.html#storage",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_PASSWORD,
        "/server/storage/mysql/password", "config-server.html#storage",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_DATABASE,
        "/server/storage/mysql/database", "config-server.html#storage",
        "gerbera"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE,
        "/server/storage/mysql/init-sql-file", "config-server.html#storage",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / mysql.sql"
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE,
        "/server/storage/mysql/upgrade-file", "config-server.html#storage",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),
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
        YES),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE,
        "/server/storage/sqlite3/database-file", "config-server.html#storage",
        "gerbera.db", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS,
        "/server/storage/sqlite3/synchronous", "config-server.html#storage",
        "off", SqliteConfig::CheckSqlLiteSyncValue, SqliteConfig::PrintSqlLiteSyncValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_SQLITE_JOURNALMODE,
        "/server/storage/sqlite3/journal-mode", "config-server.html#storage",
        "WAL", StringCheckFunction(SqliteConfig::CheckSqlJournalMode)),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_RESTORE,
        "/server/storage/sqlite3/on-error", "config-server.html#storage",
        "restore", StringCheckFunction(SqliteConfig::CheckSqlLiteRestoreValue)),
#ifdef SQLITE_BACKUP_ENABLED
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
        "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#storage",
        YES),
#else
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
        "/server/storage/sqlite3/backup/attribute::enabled", "config-server.html#storage",
        NO),
#endif
    std::make_shared<ConfigTimeSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
        "/server/storage/sqlite3/backup/attribute::interval", "config-server.html#storage",
        ConfigTimeType::Seconds, 600, 1),

    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE,
        "/server/storage/sqlite3/init-sql-file", "config-server.html#storage",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty), // This should really be "dataDir / sqlite3.sql"
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE,
        "/server/storage/sqlite3/upgrade-file", "config-server.html#storage",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty),

    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLED,
        "/server/ui/attribute::enabled", "config-server.html#ui",
        YES),
    std::make_shared<ConfigTimeSetup>(CFG_SERVER_UI_POLL_INTERVAL,
        "/server/ui/attribute::poll-interval", "config-server.html#ui",
        ConfigTimeType::Seconds, 2, 1),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_POLL_WHEN_IDLE,
        "/server/ui/attribute::poll-when-idle", "config-server.html#ui",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLE_NUMBERING,
        "/server/ui/attribute::show-numbering", "config-server.html#ui",
        YES),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLE_THUMBNAIL,
        "/server/ui/attribute::show-thumbnail", "config-server.html#ui",
        YES),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLE_VIDEO,
        "/server/ui/attribute::show-video", "config-server.html#ui",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ACCOUNTS_ENABLED,
        "/server/ui/accounts/attribute::enabled", "config-server.html#ui",
        NO),
    std::make_shared<ConfigDictionarySetup>(CFG_SERVER_UI_ACCOUNT_LIST,
        "/server/ui/accounts", "config-server.html#ui",
        ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD),
    std::make_shared<ConfigTimeSetup>(CFG_SERVER_UI_SESSION_TIMEOUT,
        "/server/ui/accounts/attribute::session-timeout", "config-server.html#ui",
        ConfigTimeType::Minutes, 30, 1),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
        "/server/ui/items-per-page/attribute::default", "config-server.html#ui",
        25, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
        "/server/ui/items-per-page", "config-server.html#ui",
        ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, ConfigArraySetup::InitItemsPerPage, true,
        defaultItemsPerPage),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_SHOW_TOOLTIPS,
        "/server/ui/attribute::show-tooltips", "config-server.html#ui",
        YES),

#ifdef GRBDEBUG
    std::make_shared<ConfigIntSetup>(CFG_SERVER_DEBUG_MODE,
        "/server/attribute::debug-mode", "config-server.html#debug-mode",
        0, GrbLogger::makeFacility, GrbLogger::printFacility),
#endif

    std::make_shared<ConfigClientSetup>(CFG_CLIENTS_LIST,
        "/clients", "config-clients.html#clients"),
    std::make_shared<ConfigBoolSetup>(CFG_CLIENTS_LIST_ENABLED,
        "/clients/attribute::enabled", "config-clients.html#clients",
        NO),
    std::make_shared<ConfigIntSetup>(CFG_CLIENTS_CACHE_THRESHOLD,
        "/clients/attribute::cache-threshold", "config-clients.html#clients",
        6, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigTimeSetup>(CFG_CLIENTS_BOOKMARK_OFFSET,
        "/clients/attribute::bookmark-offset", "config-clients.html#clients",
        ConfigTimeType::Seconds, 10, 0),

    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT,
        "/clients/client", "config-clients.html#clients",
        ""),
    std::make_shared<ConfigIntSetup>(ATTR_CLIENTS_CLIENT_FLAGS,
        "flags", "config-clients.html#client",
        0, ClientConfig::makeFlags, ClientConfig::mapFlags),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP,
        "ip", "config-clients.html#client",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_GROUP,
        "group", "config-clients.html#client",
        DEFAULT_CLIENT_GROUP),
    std::make_shared<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT,
        "userAgent", "config-clients.html#client",
        ""),

    std::make_shared<ConfigBoxLayoutSetup>(CFG_BOXLAYOUT_BOX,
        "/import/scripting/virtual-layout/boxlayout/box", "config-import.html#boxlayout",
        boxLayoutDefaults),
    std::make_shared<ConfigStringSetup>(ATTR_BOXLAYOUT_BOX_KEY,
        "attribute::key", "config-import.html#boxlayout",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_BOXLAYOUT_BOX_TITLE,
        "attribute::title", "config-import.html#boxlayout",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_BOXLAYOUT_BOX_CLASS,
        "attribute::class", "config-import.html#boxlayout",
        "object.container"),
    std::make_shared<ConfigIntSetup>(ATTR_BOXLAYOUT_BOX_SIZE,
        "attribute::size", "config-import.html#boxlayout",
        1, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(ATTR_BOXLAYOUT_BOX_ENABLED,
        "attribute::enabled", "config-import.html#boxlayout",
        YES),

    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_HIDDEN_FILES,
        "/import/attribute::hidden-files", "config-import.html#import",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_FOLLOW_SYMLINKS,
        "/import/attribute::follow-symlinks", "config-import.html#import",
        DEFAULT_FOLLOW_SYMLINKS_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_DEFAULT_DATE,
        "/import/attribute::default-date", "config-import.html#import",
        YES),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_NOMEDIA_FILE,
        "/import/attribute::nomedia-file", "config-import.html#import",
        ".nomedia"),
    std::make_shared<ConfigEnumSetup<ImportMode>>(CFG_IMPORT_LAYOUT_MODE,
        "/import/attribute::import-mode", "config-import.html#import",
        ImportMode::MediaTomb,
        std::map<std::string, ImportMode>({ { "mt", ImportMode::MediaTomb }, { "grb", ImportMode::Gerbera } })),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_VIRTUAL_DIRECTORY_KEYS,
        "/import/virtual-directories", "config-import.html#virtual-directories",
        ATTR_IMPORT_VIRT_DIR_KEY, ATTR_IMPORT_VIRT_DIR_METADATA, false, false,
        virtualDirectoryKeys),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_VIRT_DIR_KEY,
        "key", "config-import.html#virtual-directories",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_VIRT_DIR_METADATA,
        "metadata", "config-import.html#virtual-directories",
        ""),

    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_READABLE_NAMES,
        "/import/attribute::readable-names", "config-import.html#import",
        YES),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
        "/import/mappings/extension-mimetype", "config-import.html#extension-mimetype",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, true, extMtDefaults),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
        "/import/mappings/extension-mimetype/attribute::ignore-unknown", "config-import.html#extension-mimetype",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
        "/import/mappings/extension-mimetype/attribute::case-sensitive", "config-import.html#extension-mimetype",
        NO),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
        "/import/mappings/mimetype-upnpclass", "config-import.html#mimetype-upnpclass",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, true, mtUpnpDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
        "/import/mappings/mimetype-dlnatransfermode", "config-import.html#mimetype-dlnatransfermode",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, true, mtTransferDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
        "/import/mappings/mimetype-contenttype", "config-import.html#mimetype-contenttype",
        ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
        false, false, true, mtCtDefaults),
    std::make_shared<ConfigVectorSetup>(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
        "/import/mappings/contenttype-dlnaprofile", "config-import.html#contenttype-dlnaprofile",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, std::vector<config_option_t> { ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO },
        false, false, true, ctDlnaDefaults),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_MAPPINGS_IGNORED_EXTENSIONS,
        "/import/mappings/ignore-extensions", "config-import.html#ignore-extensions",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, ignoreDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LAYOUT_MAPPING,
        "/import/layout", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_MAPPING_PATH, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_LAYOUT_PARENT_PATH,
        "/import/layout/attribute::parent-path", "config-import.html#layout",
        NO),
#ifdef HAVE_JS
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_CHARSET,
        "/import/scripting/attribute::script-charset", "config-import.html#scripting",
        "UTF-8", ConfigStringSetup::CheckCharset),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT,
        "/import/scripting/common-script", "config-import.html#common-script",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_CUSTOM_SCRIPT,
        "/import/scripting/custom-script", "config-import.html#custom-script",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
        "/import/scripting/playlist-script", "config-import.html#playlist-script",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_METAFILE_SCRIPT,
        "/import/scripting/metafile-script", "config-import.html#metafile-script",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
        "/import/scripting/playlist-script/attribute::create-link", "config-import.html#playlist-script",
        NO),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT,
        "/import/scripting/virtual-layout/import-script", "config-import.html#scripting",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
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

    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_COMMON_FOLDER,
        "/import/scripting/script-folder/common", "config-import.html#script-folder",
        "", ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_CUSTOM_FOLDER,
        "/import/scripting/script-folder/custom", "config-import.html#script-folder",
        "", ConfigPathArguments::none),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST,
        "/import/scripting/import-function/playlist", "config-import.html#import-function",
        "importPlaylist"),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS,
        "/import/scripting/import-function/playlist/attribute::create-link", "config-import.html#import-function",
        YES),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE,
        "/import/scripting/import-function/meta-file", "config-import.html#import-function",
        "importMetadata"),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE,
        "/import/scripting/import-function/audio-file", "config-import.html#import-function",
        "importAudio"),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE,
        "/import/scripting/import-function/video-file", "config-import.html#import-function",
        "importVideo"),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE,
        "/import/scripting/import-function/image-file", "config-import.html#import-function",
        "importImage"),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER,
        "/import/scripting/import-function/trailer", "config-import.html#import-function",
        "importTrailer"),

    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS,
        "/import/scripting/virtual-layout/script-options", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_SCRIPT_OPTION, ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE),

    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS,
        "/import/scripting/virtual-layout/structured-layout/attribute::skip-chars", "config-import.html#scripting",
        ""),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR,
        "/import/scripting/virtual-layout/structured-layout/attribute::div-char", "config-import.html#scripting",
        ""),
#endif // HAVE_JS

    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP,
        "/import/scripting/virtual-layout/genre-map", "config-import.html#layout",
        ATTR_IMPORT_LAYOUT_GENRE, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_FILESYSTEM_CHARSET,
        "/import/filesystem-charset", "config-import.html#filesystem-charset",
        DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_METADATA_CHARSET,
        "/import/metadata-charset", "config-import.html#metadata-charset",
        DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_PLAYLIST_CHARSET,
        "/import/playlist-charset", "config-import.html#playlist-charset",
        DEFAULT_FILESYSTEM_CHARSET, ConfigStringSetup::CheckCharset),
    std::make_shared<ConfigEnumSetup<LayoutType>>(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
        "/import/scripting/virtual-layout/attribute::type", "config-import.html#scripting",
        LayoutType::Builtin,
        std::map<std::string, LayoutType>({ { "js", LayoutType::Js }, { "builtin", LayoutType::Builtin }, { "disabled", LayoutType::Disabled } })),

    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_TRANSCODING_ENABLED,
        "/transcoding/attribute::enabled", "config-transcode.html#transcoding",
        NO),
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
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, defaultFanArtFile),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_RESOURCES_FANART_DIR_LIST,
        "/import/resources/fanart", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_DIR, ATTR_IMPORT_RESOURCES_NAME, ATTR_IMPORT_RESOURCES_EXT),

    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST,
        "/import/resources/container", "config-import.html#container",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, defaultContainerArtFile),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_RESOURCES_CONTAINERART_DIR_LIST,
        "/import/resources/container", "config-import.html#container",
        ATTR_IMPORT_RESOURCES_ADD_DIR, ATTR_IMPORT_RESOURCES_NAME, ATTR_IMPORT_RESOURCES_EXT),
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
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, defaultSubtitleFile),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_RESOURCES_SUBTITLE_DIR_LIST,
        "/import/resources/subtitle", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_DIR, ATTR_IMPORT_RESOURCES_NAME, ATTR_IMPORT_RESOURCES_EXT),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_METAFILE_FILE_LIST,
        "/import/resources/metafile", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, defaultMetadataFile),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_RESOURCES_METAFILE_DIR_LIST,
        "/import/resources/metafile", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_DIR, ATTR_IMPORT_RESOURCES_NAME, ATTR_IMPORT_RESOURCES_EXT),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST,
        "/import/resources/resource", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME,
        false, false, defaultResourceFile),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST,
        "/import/resources/resource", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_DIR, ATTR_IMPORT_RESOURCES_NAME, ATTR_IMPORT_RESOURCES_EXT),

    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_ORDER,
        "/import/resources/order", "config-import.html#resources-order",
        ATTR_IMPORT_RESOURCES_HANDLER, ATTR_IMPORT_RESOURCES_NAME,
        MetadataHandler::checkContentHandler),

    std::make_shared<ConfigArraySetup>(CFG_IMPORT_SYSTEM_DIRECTORIES,
        "/import/system-directories", "config-import.html#system-directories",
        ATTR_IMPORT_SYSTEM_DIR_ADD_PATH, ATTR_IMPORT_RESOURCES_NAME, true, true,
        excludesFullpath),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_VISIBLE_DIRECTORIES,
        "/import/visible-directories", "config-import.html#visible-directories",
        ATTR_IMPORT_SYSTEM_DIR_ADD_PATH, ATTR_IMPORT_RESOURCES_NAME, false, false),

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
        YES),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/attribute::video-enabled", "config-extended.html#ffmpegthumbnailer",
        YES),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/attribute::image-enabled", "config-extended.html#ffmpegthumbnailer",
        YES),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
        "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size", "config-extended.html#ffmpegthumbnailer",
        160, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
        "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage", "config-extended.html#ffmpegthumbnailer",
        5, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
        "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay", "config-extended.html#ffmpegthumbnailer",
        NO),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
        "/server/extended-runtime-options/ffmpegthumbnailer/image-quality", "config-extended.html#ffmpegthumbnailer",
        8, ConfigIntSetup::CheckImageQualityValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
        "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled", "config-extended.html#ffmpegthumbnailer",
        YES),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, // ConfigPathSetup
        "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir", "config-extended.html#ffmpegthumbnailer",
        ""),
#endif

    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
        "/server/extended-runtime-options/mark-played-items/attribute::enabled", "config-extended.html#extended-runtime-options",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND,
        "/server/extended-runtime-options/mark-played-items/string/attribute::mode", "config-extended.html#extended-runtime-options",
        DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE, StringCheckFunction(ConfigBoolSetup::CheckMarkPlayedValue)),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING,
        "/server/extended-runtime-options/mark-played-items/string", "config-extended.html#extended-runtime-options",
        false, "*", true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
        "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates", "config-extended.html#extended-runtime-options",
        YES),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST,
        "/server/extended-runtime-options/mark-played-items/mark", "config-extended.html#extended-runtime-options",
        ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, ConfigArraySetup::InitPlayedItemsMark),
#ifdef HAVE_LASTFMLIB
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_LASTFM_ENABLED,
        "/server/extended-runtime-options/lastfm/attribute::enabled", "config-extended.html#lastfm",
        NO),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_USERNAME,
        "/server/extended-runtime-options/lastfm/username", "config-extended.html#lastfm",
        false, "lastfmuser", true),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD,
        "/server/extended-runtime-options/lastfm/password", "config-extended.html#lastfm",
        false, "lastfmpass", true),
#endif
#ifdef ATRAILERS
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED,
        "/import/online-content/AppleTrailers/attribute::enabled", "config-online.html#appletrailers",
        NO),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH,
        "/import/online-content/AppleTrailers/attribute::refresh", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START,
        "/import/online-content/AppleTrailers/attribute::update-at-start", "config-online.html#appletrailers",
        NO),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER,
        "/import/online-content/AppleTrailers/attribute::purge-after", "config-online.html#appletrailers",
        DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigEnumSetup<AtrailerResolution>>(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION,
        "/import/online-content/AppleTrailers/attribute::resolution", "config-online.html#appletrailers",
        AtrailerResolution::Low,
        std::map<std::string, AtrailerResolution>({ { "640", AtrailerResolution::Low }, { "720", AtrailerResolution::Hi }, { "720p", AtrailerResolution::Hi } })),
#endif

#ifdef HAVE_INOTIFY
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_AUTOSCAN_USE_INOTIFY,
        "/import/autoscan/attribute::use-inotify", "config-import.html#autoscan",
        "auto", StringCheckFunction(ConfigBoolSetup::CheckInotifyValue)),
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        AutoscanScanMode::INotify),
#endif
    std::make_shared<ConfigSetup>(ATTR_AUTOSCAN_DIRECTORY,
        "directory", "config-import.html#autoscan",
        ""),
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_TIMED_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        AutoscanScanMode::Timed),
    std::make_shared<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION,
        "attribute::location", "config-import.html#autoscan",
        "", ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigEnumSetup<AutoscanScanMode>>(ATTR_AUTOSCAN_DIRECTORY_MODE,
        "attribute::mode", "config-import.html#autoscan",
        std::map<std::string, AutoscanScanMode>({ { AUTOSCAN_TIMED, AutoscanScanMode::Timed }, { AUTOSCAN_INOTIFY, AutoscanScanMode::INotify } })),
    std::make_shared<ConfigTimeSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL,
        "attribute::interval", "config-import.html#autoscan",
        ConfigTimeType::Seconds, -1, 0),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE,
        "attribute::recursive", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE,
        "attribute::media-type", "config-import.html#autoscan",
        to_underlying(AutoscanDirectory::MediaType::Any), AutoscanDirectory::makeMediaType),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES,
        "attribute::hidden-files", "config-import.html#autoscan"),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS,
        "attribute::follow-symlinks", "config-import.html#autoscan"),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT,
        "attribute::scan-count", "config-import.html#autoscan"),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_TASKCOUNT,
        "attribute::task-count", "config-import.html#autoscan"),
    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_DIRECTORY_LMT,
        "attribute::last-modified", "config-import.html#autoscan"),

#ifdef HAVE_CURL
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
        "/transcoding/attribute::fetch-buffer-size", "config-transcode.html#transcoding",
        262144, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
        "/transcoding/attribute::fetch-buffer-fill-size", "config-transcode.html#transcoding",
        0, 0, ConfigIntSetup::CheckMinValue),
#endif // HAVE_CURL
#ifdef HAVE_LIBEXIF
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
        "/import/library-options/libexif/auxdata", "config-import.html#libexif",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
        "/import/library-options/libexif/metadata", "config-import.html#libexif",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, ATTR_IMPORT_LIBOPTS_AUXDATA_KEY,
        false, true, false),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_EXIF_CHARSET,
        "/import/library-options/libexif/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_EXIV2
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
        "/import/library-options/exiv2/auxdata", "config-import.html#exiv2",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
        "/import/library-options/exiv2/metadata", "config-import.html#exiv2",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, ATTR_IMPORT_LIBOPTS_AUXDATA_KEY,
        false, true, false),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_EXIV2_CHARSET,
        "/import/library-options/exiv2/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_TAGLIB
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
        "/import/library-options/id3/auxdata", "config-import.html#id3",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
        "/import/library-options/id3/metadata", "config-import.html#id3",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, ATTR_IMPORT_LIBOPTS_AUXDATA_KEY,
        false, true, false, id3SpecialPropertyMap),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ID3_CHARSET,
        "/import/library-options/id3/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_FFMPEG
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
        "/import/library-options/ffmpeg/auxdata", "config-import.html#ffmpeg",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
        "/import/library-options/ffmpeg/metadata", "config-import.html#ffmpeg",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, ATTR_IMPORT_LIBOPTS_AUXDATA_KEY,
        false, true, false, ffmpegSpecialPropertyMap),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET,
        "/import/library-options/ffmpeg/attribute::charset", "config-import.html#charset",
        ""),
#endif
#ifdef HAVE_MAGIC
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_MAGIC_FILE,
        "/import/magic-file", "config-import.html#magic-file",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist),
#endif

    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
        "/transcoding/mimetype-profile-mappings/attribute::allow-unused", "config-transcode.html#mimetype-profile-mappings",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
        "/transcoding/profiles/attribute::allow-unused", "config-transcode.html#profiles",
        NO),
    std::make_shared<ConfigEnumSetup<TranscodingType>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE,
        "attribute::type", "config-transcode.html#profiles",
        std::map<std::string, TranscodingType>({ { "none", TranscodingType::None }, { "external", TranscodingType::External }, /* for the future...{"remote", TranscodingType::Remote}*/ })),
    std::make_shared<ConfigEnumSetup<AviFourccListmode>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
        "mode", "config-transcode.html#profiles",
        std::map<std::string, AviFourccListmode>({ { "ignore", AviFourccListmode::Ignore }, { "process", AviFourccListmode::Process }, { "disabled", AviFourccListmode::None } })),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING,
        "attribute::using", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC,
        "fourcc", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS,
        "attribute::client-flags", "config-transcode.html#profiles",
        0, ClientConfig::makeFlags),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_DLNAPROF,
        "attribute::dlna-profile", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING,
        "attribute::no-transcoding", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SRCDLNA,
        "attribute::source-profile", "config-transcode.html#profiles",
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
        false, false, false, trMtDefaults),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_MIMETYPE_FILTER,
        "/transcoding/mimetype-profile-mappings/transcode", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE,
        "/transcoding/profiles/profile", "config-transcode.html#profiles",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NAME,
        "attribute::name", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
        "mimetype", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigPathSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
        "attribute::command", "config-transcode.html#profiles",
        "", ConfigPathArguments::isFile | ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::isExe | ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
        "attribute::arguments", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigDictionarySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON,
        "agent", "config-transcode.html#profiles",
        ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
        false, false, false),
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
        "", ConfigPathArguments::mustExist | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
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
        "", ConfigStringSetup::CheckCharset),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_FANART_FILE,
        "attribute::fanart-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_SUBTITLE_FILE,
        "attribute::subtitle-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_METAFILE_FILE,
        "attribute::metadata-file", "config-import.html#resources",
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

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_RESOURCES_EXT,
        "attribute::ext", "config-import.html#resources",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_FROM,
        "attribute::from", "config-import.html#layout",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_TO,
        "attribute::to", "config-import.html#layout",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_CONTAINER_TYPE_AUDIO,
        "container-type-audio", "config-import.html#autoscan",
        UPNP_CLASS_MUSIC_ALBUM),
    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_CONTAINER_TYPE_IMAGE,
        "container-type-image", "config-import.html#autoscan",
        UPNP_CLASS_PHOTO_ALBUM),
    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_CONTAINER_TYPE_VIDEO,
        "container-type-video", "config-import.html#autoscan",
        UPNP_CLASS_CONTAINER),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_TAG,
        "attribute::tag", "config-import.html#auxdata",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_KEY,
        "attribute::key", "config-import.html#auxdata",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD,
        "attribute::password", "config-server.html#ui",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_USER,
        "attribute::user", "config-server.html#ui",
        ""),

    std::make_shared<ConfigBoolSetup>(CFG_UPNP_LITERAL_HOST_REDIRECTION,
        "/server/upnp/attribute::literal-host-redirection", "config-server.html#upnp",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_UPNP_MULTI_VALUES_ENABLED,
        "/server/upnp/attribute::multi-value", "config-server.html#upnp",
        YES),
    std::make_shared<ConfigBoolSetup>(CFG_UPNP_SEARCH_CONTAINER_FLAG,
        "/server/upnp/attribute::searchable-container-flag", "config-server.html#upnp",
        NO),
    std::make_shared<ConfigStringSetup>(CFG_UPNP_SEARCH_SEPARATOR,
        "/server/upnp/attribute::search-result-separator", "config-server.html#upnp",
        " - "),
    std::make_shared<ConfigBoolSetup>(CFG_UPNP_SEARCH_FILENAME,
        "/server/upnp/attribute::search-filename", "config-server.html#upnp",
        NO),
    std::make_shared<ConfigIntSetup>(CFG_UPNP_CAPTION_COUNT,
        "/server/upnp/attribute::caption-info-count", "config-server.html#upnp",
        1, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigArraySetup>(CFG_UPNP_SEARCH_ITEM_SEGMENTS,
        "/server/upnp/search-item-result", "config-server.html#upnpf",
        ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG,
        false, false, upnpSearchSegmentDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ALBUM_PROPERTIES,
        "/server/upnp/album-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, upnpAlbumPropDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ARTIST_PROPERTIES,
        "/server/upnp/artist-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, upnpArtistPropDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_GENRE_PROPERTIES,
        "/server/upnp/genre-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, upnpGenrePropDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_PLAYLIST_PROPERTIES,
        "/server/upnp/playlist-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, true, upnpPlaylistPropDefaults),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_TITLE_PROPERTIES,
        "/server/upnp/title-properties", "config-server.html#upnp",
        ATTR_UPNP_PROPERTIES_PROPERTY, ATTR_UPNP_PROPERTIES_UPNPTAG, ATTR_UPNP_PROPERTIES_METADATA,
        false, false, false),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_PROPERTIES_UPNPTAG,
        "attribute::upnp-tag", "config-server.html#upnp"),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_PROPERTIES_METADATA,
        "attribute::meta-data", "config-server.html#upnp"),

    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ALBUM_NAMESPACES,
        "/server/upnp/album-properties", "config-server.html#upnp",
        ATTR_UPNP_NAMESPACE_PROPERTY, ATTR_UPNP_NAMESPACE_KEY, ATTR_UPNP_NAMESPACE_URI,
        false, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_ARTIST_NAMESPACES,
        "/server/upnp/artist-properties", "config-server.html#upnp",
        ATTR_UPNP_NAMESPACE_PROPERTY, ATTR_UPNP_NAMESPACE_KEY, ATTR_UPNP_NAMESPACE_URI,
        false, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_GENRE_NAMESPACES,
        "/server/upnp/genre-properties", "config-server.html#upnp",
        ATTR_UPNP_NAMESPACE_PROPERTY, ATTR_UPNP_NAMESPACE_KEY, ATTR_UPNP_NAMESPACE_URI,
        false, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_PLAYLIST_NAMESPACES,
        "/server/upnp/playlist-properties", "config-server.html#upnp",
        ATTR_UPNP_NAMESPACE_PROPERTY, ATTR_UPNP_NAMESPACE_KEY, ATTR_UPNP_NAMESPACE_URI,
        false, false, false),
    std::make_shared<ConfigDictionarySetup>(CFG_UPNP_TITLE_NAMESPACES,
        "/server/upnp/title-properties", "config-server.html#upnp",
        ATTR_UPNP_NAMESPACE_PROPERTY, ATTR_UPNP_NAMESPACE_KEY, ATTR_UPNP_NAMESPACE_URI,
        false, false, false),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_NAMESPACE_KEY,
        "attribute::xmlns", "config-server.html#upnp"),
    std::make_shared<ConfigStringSetup>(ATTR_UPNP_NAMESPACE_URI,
        "attribute::uri", "config-server.html#upnp"),

    std::make_shared<ConfigDynamicContentSetup>(CFG_SERVER_DYNAMIC_CONTENT_LIST,
        "/server/containers", "config-server.html#containers"),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_DYNAMIC_CONTENT_LIST_ENABLED,
        "/server/containers/attribute::enabled", "config-server.html#containers",
        true, false),
    std::make_shared<ConfigPathSetup>(ATTR_DYNAMIC_CONTAINER_LOCATION,
        "attribute::location", "config-server.html#location",
        "/Auto", ConfigPathArguments::isFile | ConfigPathArguments::notEmpty | ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigPathSetup>(ATTR_DYNAMIC_CONTAINER_IMAGE,
        "attribute::image", "config-server.html#image",
        "", ConfigPathArguments::isFile | ConfigPathArguments::resolveEmpty),
    std::make_shared<ConfigStringSetup>(ATTR_DYNAMIC_CONTAINER_TITLE,
        "attribute::title", "config-server.html#title",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DYNAMIC_CONTAINER_FILTER,
        "filter", "config-server.html#filter",
        true, "last_updated > \"@last31\""),
    std::make_shared<ConfigStringSetup>(ATTR_DYNAMIC_CONTAINER_SORT,
        "attribute::sort", "config-server.html#sort",
        ""),
    std::make_shared<ConfigIntSetup>(ATTR_DYNAMIC_CONTAINER_MAXCOUNT,
        "attribute::max-count", "config-server.html#max-count",
        500, 0, ConfigIntSetup::CheckMinValue),

    std::make_shared<ConfigDictionarySetup>(ATTR_CLIENTS_UPNP_MAP_MIMETYPE,
        "/clients/client", "config-import.html#map",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        false, false, false),
    std::make_shared<ConfigIntSetup>(ATTR_CLIENTS_UPNP_CAPTION_COUNT,
        "attribute::caption-info-count", "config-clients.html#caption-info-count",
        1, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_CLIENTS_UPNP_STRING_LIMIT,
        "attribute::upnp-string-limit", "config-clients.html#upnp-string-limit",
        DEFAULT_UPNP_STRING_LIMIT, ConfigIntSetup::CheckUpnpStringLimitValue),
    std::make_shared<ConfigBoolSetup>(ATTR_CLIENTS_UPNP_MULTI_VALUE,
        "attribute::multi-value", "config-clients.html#multi-value",
        YES),

    // simpleOptions

    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_KEY,
        "environ", ""),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_NAME,
        "name", ""),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ENVIRON_VALUE,
        "value", ""),
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
    std::make_shared<ConfigSetup>(ATTR_IMPORT_RESOURCES_ADD_DIR,
        "add-dir", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_DATA,
        "add-data", ""),
    std::make_shared<ConfigSetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE,
        "transcode", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_MAPPING_PATH,
        "path", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_SCRIPT_OPTION,
        "script-option", ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_NAME,
        "name", ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE,
        "value", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_LAYOUT_GENRE,
        "genre", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_SYSTEM_DIR_ADD_PATH,
        "add-path", ""),
    std::make_shared<ConfigSetup>(ATTR_DYNAMIC_CONTAINER,
        "container", ""),
    std::make_shared<ConfigSetup>(ATTR_IMPORT_RESOURCES_HANDLER,
        "handler", ""),

    std::make_shared<ConfigSetup>(ATTR_UPNP_PROPERTIES_PROPERTY,
        "upnp-property", ""),
    std::make_shared<ConfigSetup>(ATTR_UPNP_NAMESPACE_PROPERTY,
        "upnp-namespace", ""),
};

/// \brief define parent options for path search
const std::map<config_option_t, std::vector<config_option_t>> ConfigDefinition::parentOptions {
    { ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_CLIENTFLAGS, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, { CFG_TRANSCODING_PROFILE_LIST } },

    { ATTR_UPNP_PROPERTIES_PROPERTY, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_GENRE_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES, CFG_UPNP_PLAYLIST_PROPERTIES } },
    { ATTR_UPNP_PROPERTIES_UPNPTAG, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_GENRE_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES, CFG_UPNP_PLAYLIST_PROPERTIES } },
    { ATTR_UPNP_PROPERTIES_METADATA, { CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_GENRE_PROPERTIES, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_TITLE_PROPERTIES, CFG_UPNP_PLAYLIST_PROPERTIES } },

    { ATTR_UPNP_NAMESPACE_PROPERTY, { CFG_UPNP_ALBUM_NAMESPACES, CFG_UPNP_GENRE_NAMESPACES, CFG_UPNP_ARTIST_NAMESPACES, CFG_UPNP_TITLE_NAMESPACES, CFG_UPNP_PLAYLIST_NAMESPACES } },
    { ATTR_UPNP_NAMESPACE_KEY, { CFG_UPNP_ALBUM_NAMESPACES, CFG_UPNP_GENRE_NAMESPACES, CFG_UPNP_ARTIST_NAMESPACES, CFG_UPNP_TITLE_NAMESPACES, CFG_UPNP_PLAYLIST_NAMESPACES } },
    { ATTR_UPNP_NAMESPACE_URI, { CFG_UPNP_ALBUM_NAMESPACES, CFG_UPNP_GENRE_NAMESPACES, CFG_UPNP_ARTIST_NAMESPACES, CFG_UPNP_TITLE_NAMESPACES, CFG_UPNP_PLAYLIST_NAMESPACES } },

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
    { ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                             CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                         } },
    { ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
#ifdef HAVE_INOTIFY
                                               CFG_IMPORT_AUTOSCAN_INOTIFY_LIST
#endif
                                           } },
    { ATTR_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS, { CFG_IMPORT_AUTOSCAN_TIMED_LIST,
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

    { ATTR_DYNAMIC_CONTAINER, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },
    { ATTR_DYNAMIC_CONTAINER_LOCATION, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },
    { ATTR_DYNAMIC_CONTAINER_IMAGE, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },
    { ATTR_DYNAMIC_CONTAINER_TITLE, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },
    { ATTR_DYNAMIC_CONTAINER_FILTER, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },
    { ATTR_DYNAMIC_CONTAINER_SORT, { CFG_SERVER_DYNAMIC_CONTENT_LIST } },

    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, {} },

    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST } },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, {
                                              CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                              CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                              CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                              CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                              CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                          } },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_TO, {
                                            CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
                                            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
                                            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
                                            CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST,
                                            CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST,
                                        } },

    { ATTR_IMPORT_RESOURCES_NAME, { CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, CFG_IMPORT_RESOURCES_METAFILE_FILE_LIST, //
                                      CFG_IMPORT_RESOURCES_FANART_DIR_LIST, CFG_IMPORT_RESOURCES_CONTAINERART_DIR_LIST, CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_DIR_LIST, CFG_IMPORT_RESOURCES_METAFILE_DIR_LIST, //
                                      CFG_IMPORT_SYSTEM_DIRECTORIES, CFG_IMPORT_RESOURCES_ORDER } },
    { ATTR_IMPORT_RESOURCES_EXT, { CFG_IMPORT_RESOURCES_FANART_DIR_LIST, CFG_IMPORT_RESOURCES_CONTAINERART_DIR_LIST, CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_DIR_LIST, CFG_IMPORT_RESOURCES_METAFILE_DIR_LIST } },

    { ATTR_IMPORT_LAYOUT_MAPPING_FROM, { CFG_IMPORT_LAYOUT_MAPPING, CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
    { ATTR_IMPORT_LAYOUT_MAPPING_TO, { CFG_IMPORT_LAYOUT_MAPPING, CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP } },
#ifdef HAVE_JS
    { ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_NAME, { CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
    { ATTR_IMPORT_LAYOUT_SCRIPT_OPTION_VALUE, { CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS } },
#endif
};

/// \brief define option dependencies for automatic loading
const std::map<config_option_t, config_option_t> ConfigDefinition::dependencyMap = {
    { CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_JOURNALMODE, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_RESTORE, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE, CFG_SERVER_STORAGE_SQLITE_ENABLED },
    { CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE, CFG_SERVER_STORAGE_SQLITE_ENABLED },
#ifdef HAVE_MYSQL
    { CFG_SERVER_STORAGE_MYSQL_HOST, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_DATABASE, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_USERNAME, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_PORT, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_SOCKET, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_PASSWORD, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE, CFG_SERVER_STORAGE_MYSQL },
    { CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE, CFG_SERVER_STORAGE_MYSQL },
#endif
#ifdef HAVE_CURL
    { CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, CFG_TRANSCODING_TRANSCODING_ENABLED },
    { CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE, CFG_TRANSCODING_TRANSCODING_ENABLED },
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED },
#endif
#if defined(HAVE_LASTFMLIB)
    { CFG_SERVER_EXTOPTS_LASTFM_USERNAME, CFG_SERVER_EXTOPTS_LASTFM_ENABLED },
    { CFG_SERVER_EXTOPTS_LASTFM_PASSWORD, CFG_SERVER_EXTOPTS_LASTFM_ENABLED },
#endif
};

const char* ConfigDefinition::mapConfigOption(config_option_t option)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& c) { return c->option == option; });
    if (co != complexOptions.end()) {
        return (*co)->xpath;
    }
    return "";
}

bool ConfigDefinition::isDependent(config_option_t option)
{
    return (dependencyMap.find(option) != dependencyMap.end());
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetup(config_option_t option, bool save)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [=](auto&& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (save)
        return nullptr;

    throw_std_runtime_error("Error in config code: {} tag not found. CFG_MAX={}", option, CFG_MAX);
}

std::shared_ptr<ConfigSetup> ConfigDefinition::findConfigSetupByPath(const std::string& key, bool save, const std::shared_ptr<ConfigSetup>& parent)
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
                auto len = std::min(uPath.length(), key.length());
                return key.substr(0, len) == uPath.substr(0, len);
            });
        return (co != complexOptions.end()) ? *co : nullptr;
    }

    throw_std_runtime_error("Error in config code: {} tag not found", key);
}

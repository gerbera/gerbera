/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_manager.cc - this file is part of MediaTomb.

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

/// \file config_manager.cc

#include "config_manager.h" // API

#include <array>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <clocale>
#include <langinfo.h>
#include <utility>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "autoscan.h"
#include "client_config.h"
#include "config_options.h"
#include "config_setup.h"
#include "database/database.h"
#include "transcoding/transcoding.h"
#include "util/string_converter.h"
#include "util/tools.h"

bool ConfigManager::debug_logging = false;

ConfigManager::ConfigManager(fs::path filename,
    const fs::path& userhome, const fs::path& config_dir,
    fs::path prefix_dir, fs::path magic_file,
    std::string ip, std::string interface, in_port_t port,
    bool debug_logging)
    : filename(std::move(filename))
    , prefix_dir(std::move(prefix_dir))
    , magic_file(std::move(magic_file))
    , ip(std::move(ip))
    , interface(std::move(interface))
    , port(port)
    , xmlDoc(std::make_unique<pugi::xml_document>())
    , options(std::make_unique<std::vector<std::shared_ptr<ConfigOption>>>())
{
    ConfigManager::debug_logging = debug_logging;

    options->resize(CFG_MAX);

    if (this->filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userhome / config_dir;
        this->filename += home / DEFAULT_CONFIG_NAME;
    }

    std::error_code ec;
    if (!isRegularFile(this->filename, ec)) {
        std::ostringstream expErrMsg;
        expErrMsg << "\nThe server configuration file could not be found: ";
        expErrMsg << this->filename << "\n";
        expErrMsg << "Gerbera could not find a default configuration file.\n";
        expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
        expErrMsg << "For a list of options run: gerbera -h\n";

        throw std::runtime_error(expErrMsg.str());
    }
}

ConfigManager::~ConfigManager()
{
    log_debug("ConfigManager destroyed");
}

std::shared_ptr<Config> ConfigManager::getSelf()
{
    return shared_from_this();
}

const std::vector<std::shared_ptr<ConfigSetup>> ConfigManager::complexOptions = {
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
        "/server/home", "config-server.html#jome"),
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
        "", true, false),
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
        DEFAULT_ITEMS_PER_PAGE_2, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN,
        "/server/ui/items-per-page", "config-server.html#ui",
        ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, ConfigArraySetup::InitItemsPerPage, true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_SHOW_TOOLTIPS,
        "/server/ui/attribute::show-tooltips", "config-server.html#ui",
        DEFAULT_UI_SHOW_TOOLTIPS_VALUE),

    std::make_shared<ConfigClientSetup>(CFG_CLIENTS_LIST,
        "/clients", "config-clients.html#clients"),
    std::make_shared<ConfigBoolSetup>(CFG_CLIENTS_LIST_ENABLED,
        "/clients/attribute::enabled", "config-clients.html#clients",
        DEFAULT_CLIENTS_EN_VALUE),
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
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST,
        "/import/mappings/extension-mimetype", "config-import.html#extension-mimetype",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
        "/import/mappings/extension-mimetype/attribute::ignore-unknown", "config-import.html#extension-mimetype",
        DEFAULT_IGNORE_UNKNOWN_EXTENSIONS),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
        "/import/mappings/extension-mimetype/attribute::case-sensitive", "config-import.html#extension-mimetype",
        DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST,
        "/import/mappings/mimetype-upnpclass", "config-import.html#mime-type-upnpclass",
        ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST,
        "/import/mappings/mimetype-contenttype", "config-import.html#mime-type-upnpclass",
        ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS),
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
        "", true),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
        "/import/scripting/playlist-script", "config-import.html#playlist-script",
        "", true),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
        "/import/scripting/playlist-script/attribute::create-link", "config-import.html#playlist-script",
        DEFAULT_PLAYLIST_CREATE_LINK),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT,
        "/import/scripting/virtual-layout/import-script", "config-import.html#scripting"),
#endif // JS
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
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST,
        "/import/resources/subtitle", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST,
        "/import/resources/resource", "config-import.html#resources",
        ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),

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
        std::to_string(DEFAULT_ATRAILERS_RESOLUTION).c_str(),
        std::map<std::string, std::string>({ { "640", "640" }, { "720", "720p" }, { "720p", "720p" } })),
#endif

    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_AUTOSCAN_USE_INOTIFY,
        "/import/autoscan/attribute::use-inotify", "config-import.html#autoscan",
        "auto", ConfigBoolSetup::CheckInotifyValue),
#ifdef HAVE_INOTIFY
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        ScanMode::INotify),
#endif
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
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_TIMED_LIST,
        "/import/autoscan", "config-import.html#autoscan",
        ScanMode::Timed),
    std::make_shared<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION,
        "location", "config-import.html#autoscan",
        "", false, true, true),
    std::make_shared<ConfigEnumSetup<ScanMode>>(ATTR_AUTOSCAN_DIRECTORY_MODE,
        "mode", "config-import.html#autoscan",
        std::map<std::string, ScanMode>({ { "timed", ScanMode::Timed }, { "inotify", ScanMode::INotify } })),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL,
        "interval", "config-import.html#autoscan",
        -1, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE,
        "recursive", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES,
        "hidden-files", "config-import.html#autoscan"),
    std::make_shared<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_SCANCOUNT,
        "scan-count", "config-import.html#autoscan"),
    std::make_shared<ConfigStringSetup>(ATTR_AUTOSCAN_DIRECTORY_LMT,
        "last-modified", "config-import.html#autoscan"),

    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED,
        "/transcoding/mimetype-profile-mappings/attribute::allow-unused", "config-transcode.html#mimetype-profile-mappings",
        NO),
    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED,
        "/transcoding/profiles/attribute::allow-unused", "config-transcode.html#profiles",
        NO),
    std::make_shared<ConfigEnumSetup<transcoding_type_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE,
        "type", "config-transcode.html#profiles",
        std::map<std::string, transcoding_type_t>({ { "none", TR_None }, { "external", TR_External }, /* for the future...{"remote", TR_Remote}*/ })),
    std::make_shared<ConfigEnumSetup<avi_fourcc_listmode_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE,
        "mode", "config-transcode.html#profiles",
        std::map<std::string, avi_fourcc_listmode_t>({ { "ignore", FCC_Ignore }, { "process", FCC_Process }, { "disabled", FCC_None } })),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED,
        "enabled", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL,
        "accept-url", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG,
        "hide-original-resource", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB,
        "thumbnail", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST,
        "first-resource", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC,
        "use-chunked-encoding", "config-transcode.html#profiles"),
    std::make_shared<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG,
        "accept-ogg-theora", "config-transcode.html#profiles"),
    std::make_shared<ConfigArraySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC,
        "avi-fourcc-list", "config-transcode.html#profiles",
        ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, CFG_MAX, true, true),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE,
        "size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK,
        "chunk-size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL,
        "fill-size", "config-transcode.html#profiles",
        0, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ,
        "sample-frequency", "config-transcode.html#profiles",
        "-1", ConfigIntSetup::CheckProfileNumberValue),
    std::make_shared<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN,
        "audio-channels", "config-transcode.html#profiles",
        "-1", ConfigIntSetup::CheckProfileNumberValue),
    std::make_shared<ConfigDictionarySetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP,
        "mimetype-profile-mappings", "config-transcode.html#profiles",
        ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NAME,
        "name", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE,
        "mimetype", "config-transcode.html#profiles",
        true, "", true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND,
        "command", "config-transcode.html#profiles",
        "", ConfigPathSetup::checkAgentPath, true, true),
    std::make_shared<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS,
        "arguments", "config-transcode.html#profiles",
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
        "mimetype", "config-transcode.html#profiles",
        ""),

    std::make_shared<ConfigDirectorySetup>(CFG_IMPORT_DIRECTORIES_LIST,
        "/import/directories", "config-import.html#autoscan"),
    std::make_shared<ConfigPathSetup>(ATTR_DIRECTORIES_TWEAK_LOCATION,
        "location", "config-import.html#autoscan",
        "", false, true, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_INHERIT,
        "inherit", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_RECURSIVE,
        "recursive", "config-import.html#autoscan",
        false, true),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_HIDDEN,
        "hidden-files", "config-import.html#autoscan"),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE,
        "case-sensitive", "config-import.html#autoscan",
        DEFAULT_RESOURCES_CASE_SENSITIVE),
    std::make_shared<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS,
        "follow-symlinks", "config-import.html#autoscan",
        DEFAULT_FOLLOW_SYMLINKS_VALUE),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_META_CHARSET,
        "meta-charset", "config-import.html#charset",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_FANART_FILE,
        "fanart-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE,
        "subtitle-file", "config-import.html#resources",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE,
        "resource-file", "config-import.html#resources",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE,
        "mimetype", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT,
        "treat", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS,
        "as", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM,
        "from", "config-import.html#mappings",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_MAPPINGS_MIMETYPE_TO,
        "to", "config-import.html#mappings",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_RESOURCES_NAME,
        "name", "config-import.html#resources",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_FROM,
        "from", "config-import.html#layout",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LAYOUT_MAPPING_TO,
        "to", "config-import.html#layout",
        ""),

    std::make_shared<ConfigStringSetup>(ATTR_IMPORT_LIBOPTS_AUXDATA_TAG,
        "tag", "config-import.html#auxdata",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD,
        "password", "config-server.html#ui",
        ""),
    std::make_shared<ConfigStringSetup>(ATTR_SERVER_UI_ACCOUNT_LIST_USER,
        "user", "config-server.html#ui",
        ""),
};

const std::map<config_option_t, std::vector<config_option_t>> ConfigManager::parentOptions = {
    { ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, { CFG_TRANSCODING_PROFILE_LIST } },
    { ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, { CFG_TRANSCODING_PROFILE_LIST } },

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

    { ATTR_IMPORT_RESOURCES_NAME, { CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST } },

    { ATTR_IMPORT_LAYOUT_MAPPING_FROM, { CFG_IMPORT_LAYOUT_MAPPING } },
    { ATTR_IMPORT_LAYOUT_MAPPING_TO, { CFG_IMPORT_LAYOUT_MAPPING } },
};

constexpr std::array<std::pair<config_option_t, const char*>, 16> ConfigManager::simpleOptions { {
    { CFG_MAX, "max_option" },

    { ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, "option" },
    { ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, "content" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, "acount" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, "map" },
    { ATTR_IMPORT_RESOURCES_ADD_FILE, "add-file" },
    { ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, "add-data" },

    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, "transcode" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING, "using" },
    { ATTR_IMPORT_LAYOUT_MAPPING_PATH, "path" },
    { ATTR_TRANSCODING_PROFILES, "profiles" },
    { ATTR_TRANSCODING_PROFILES_PROFLE, "profile" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, "fourcc" },

    { ATTR_AUTOSCAN_DIRECTORY, "directory" },
    { ATTR_CLIENTS_CLIENT, "client" },
    { ATTR_DIRECTORIES_TWEAK, "tweak" },
} };

const char* ConfigManager::mapConfigOption(config_option_t option)
{
    auto co = std::find_if(simpleOptions.begin(), simpleOptions.end(), [&](const auto& s) { return s.first == option; });
    if (co != simpleOptions.end()) {
        return (*co).second;
    }

    auto co2 = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->option == option; });
    if (co2 != complexOptions.end()) {
        return (*co2)->xpath;
    }
    return "";
}

std::shared_ptr<ConfigSetup> ConfigManager::findConfigSetup(config_option_t option, bool save)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    if (save)
        return nullptr;

    throw std::runtime_error(fmt::format("Error in config code: {} tag not found", static_cast<int>(option)));
}

std::shared_ptr<ConfigSetup> ConfigManager::findConfigSetupByPath(const std::string& key, bool save, const std::shared_ptr<ConfigSetup>& parent)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->getUniquePath() == key; });

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
        co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->getUniquePath() == attrKey && (parentOptions.find(s->option) == parentOptions.end() || parentOptions.at(s->option).end() != std::find_if(parentOptions.at(s->option).begin(), parentOptions.at(s->option).end(), [&](const auto& o) { return o == s->option; })); });

        if (co != complexOptions.end()) {
            log_debug("Config: attribute option found: '{}'", (*co)->xpath);
            return *co;
        }
    }

    if (save) {
        co = std::find_if(complexOptions.begin(), complexOptions.end(),
            [&](const auto& s) {
                auto uPath = s->getUniquePath();
                size_t len = std::min(uPath.length(), key.length());
                return key.substr(0, len) == uPath.substr(0, len);
            });
        return (co != complexOptions.end()) ? *co : nullptr;
    }

    throw std::runtime_error(fmt::format("Error in config code: {} tag not found", key));
}

std::shared_ptr<ConfigOption> ConfigManager::setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments)
{
    auto co = findConfigSetup(option);
    auto self = getSelf();
    co->makeOption(root, self, arguments);
    log_debug("Config: option set: '{}'", co->xpath);
    return co->getValue();
}

void ConfigManager::addOption(config_option_t option, std::shared_ptr<ConfigOption> optionValue)
{
    options->at(option) = optionValue;
}

void ConfigManager::load(const fs::path& userHome)
{
    std::string temp;
    pugi::xml_node tmpEl;

    std::map<std::string, std::string> args;
    auto self = getSelf();
    std::shared_ptr<ConfigSetup> co;

    log_info("Loading configuration from: {}", filename.c_str());
    pugi::xml_parse_result result = xmlDoc->load_file(filename.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }

    log_info("Checking configuration...");

    auto root = xmlDoc->document_element();

    // first check if the config file itself looks ok, it must have a config
    // and a server tag
    if (std::string(root.name()) != ConfigSetup::ROOT_NAME)
        throw std::runtime_error("Error in config file: <config> tag not found");

    if (root.child("server") == nullptr)
        throw std::runtime_error("Error in config file: <server> tag not found");

    std::string version = root.attribute("version").as_string();
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw std::runtime_error("Config version \"" + version + "\" does not yet exist");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server
    co = findConfigSetup(CFG_SERVER_HOME);
    if (!userHome.empty()) {
        // respect command line; ignore xml value
        temp = userHome;
    } else {
        temp = co->getXmlContent(root);
    }

    if (!fs::is_directory(temp))
        throw std::runtime_error(fmt::format("Directory '{}' does not exist", temp));
    co->makeOption(temp, self);
    ConfigPathSetup::Home = temp;

    setOption(root, CFG_SERVER_WEBROOT);
    setOption(root, CFG_SERVER_TMPDIR);
    setOption(root, CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    setOption(root, CFG_SERVER_UDN);

    // checking database driver options
    bool mysql_en = false;
    bool sqlite3_en = false;

    co = findConfigSetup(CFG_SERVER_STORAGE);
    co->getXmlElement(root); // fails if missing

    co = findConfigSetup(CFG_SERVER_STORAGE_MYSQL);
    if (co->hasXmlElement(root)) {
        mysql_en = setOption(root, CFG_SERVER_STORAGE_MYSQL_ENABLED)->getBoolOption();
    }

    co = findConfigSetup(CFG_SERVER_STORAGE_SQLITE);
    if (co->hasXmlElement(root)) {
        sqlite3_en = setOption(root, CFG_SERVER_STORAGE_SQLITE_ENABLED)->getBoolOption();
    }

    if (sqlite3_en && mysql_en)
        throw std::runtime_error("You enabled both, sqlite3 and mysql but "
                                 "only one database driver may be active at a time");

    if (!sqlite3_en && !mysql_en)
        throw std::runtime_error("You disabled both sqlite3 and mysql but "
                                 "one database driver must be active");

#ifdef HAVE_MYSQL
    if (mysql_en) {
        setOption(root, CFG_SERVER_STORAGE_MYSQL_HOST);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_DATABASE);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_USERNAME);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PORT);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_SOCKET);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    }
#else
    if (mysql_en) {
        throw std::runtime_error("You enabled MySQL database in configuration, "
                                 "however this version of Gerbera was compiled "
                                 "without MySQL support!");
    }
#endif // HAVE_MYSQL

    if (sqlite3_en) {
        setOption(root, CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_RESTORE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
    }

    std::string dbDriver;
    if (sqlite3_en)
        dbDriver = "sqlite3";
    if (mysql_en)
        dbDriver = "mysql";

    co = findConfigSetup(CFG_SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, self);

    // now go through the optional settings and fix them if anything is missing
    setOption(root, CFG_SERVER_UI_ENABLED);
    setOption(root, CFG_SERVER_UI_SHOW_TOOLTIPS);
    setOption(root, CFG_SERVER_UI_POLL_WHEN_IDLE);
    setOption(root, CFG_SERVER_UI_POLL_INTERVAL);

    auto def_ipp = setOption(root, CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)->getIntOption();

    // now get the option list for the drop down menu
    auto menu_opts = setOption(root, CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN)->getArrayOption();
    if (std::none_of(menu_opts.begin(), menu_opts.end(), [=](const auto& s) { return s == std::to_string(def_ipp); }))
        throw std::runtime_error("Error in config file: at least one <option> "
                                 "under <items-per-page> must match the "
                                 "<items-per-page default=\"\" /> attribute");

    setOption(root, CFG_SERVER_UI_ACCOUNTS_ENABLED);
    setOption(root, CFG_SERVER_UI_ACCOUNT_LIST);
    setOption(root, CFG_SERVER_UI_SESSION_TIMEOUT);

    bool cl_en = setOption(root, CFG_CLIENTS_LIST_ENABLED)->getBoolOption();
    args["isEnabled"] = cl_en ? "true" : "false";
    setOption(root, CFG_CLIENTS_LIST, &args);
    args.clear();

    setOption(root, CFG_IMPORT_HIDDEN_FILES);
    setOption(root, CFG_IMPORT_FOLLOW_SYMLINKS);
    setOption(root, CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    bool csens = setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE)->getBoolOption();
    args["tolower"] = std::to_string(!csens);
    setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, &args);
    args.clear();
    setOption(root, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    setOption(root, CFG_IMPORT_LAYOUT_PARENT_PATH);
    setOption(root, CFG_IMPORT_LAYOUT_MAPPING);

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "") != nullptr) {
        temp = nl_langinfo(CODESET);
        log_debug("received {} from nl_langinfo", temp.c_str());
    }

    if (temp.empty())
        temp = DEFAULT_FILESYSTEM_CHARSET;
#else
    temp = DEFAULT_FILESYSTEM_CHARSET;
#endif
    // check if the one we take as default is actually available
    co = findConfigSetup(CFG_IMPORT_FILESYSTEM_CHARSET);
    try {
        auto conv = std::make_unique<StringConverter>(temp,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        temp = DEFAULT_FALLBACK_CHARSET;
    }
    co->setDefaultValue(temp);
    std::string charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported filesystem-charset specified: " + charset);
    }
    log_debug("Setting filesystem import charset to {}", charset.c_str());
    co->makeOption(charset, self);

    co = findConfigSetup(CFG_IMPORT_METADATA_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported metadata-charset specified: " + charset);
    }
    log_debug("Setting metadata import charset to {}", charset.c_str());
    co->makeOption(charset, self);

    co = findConfigSetup(CFG_IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported playlist-charset specified: " + charset);
    }
    log_debug("Setting playlist charset to {}", charset.c_str());
    co->makeOption(charset, self);

    setOption(root, CFG_SERVER_HIDE_PC_DIRECTORY);

    co = findConfigSetup(CFG_SERVER_NETWORK_INTERFACE);
    if (interface.empty()) {
        temp = co->getXmlContent(root);
    } else {
        temp = interface;
    }
    co->makeOption(temp, self);

    co = findConfigSetup(CFG_SERVER_IP);
    if (ip.empty()) {
        temp = co->getXmlContent(root); // bind to any IP address
    } else {
        temp = ip;
    }
    co->makeOption(temp, self);

    if (!getOption(CFG_SERVER_NETWORK_INTERFACE).empty() && !getOption(CFG_SERVER_IP).empty())
        throw std::runtime_error("Error in config file: you can not specify interface and ip at the same time");

    setOption(root, CFG_SERVER_BOOKMARK_FILE);
    setOption(root, CFG_SERVER_NAME);
    setOption(root, CFG_SERVER_MODEL_NAME);
    setOption(root, CFG_SERVER_MODEL_DESCRIPTION);
    setOption(root, CFG_SERVER_MODEL_NUMBER);
    setOption(root, CFG_SERVER_MODEL_URL);
    setOption(root, CFG_SERVER_SERIAL_NUMBER);
    setOption(root, CFG_SERVER_MANUFACTURER);
    setOption(root, CFG_SERVER_MANUFACTURER_URL);
    setOption(root, CFG_SERVER_PRESENTATION_URL);
    setOption(root, CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

    temp = setOption(root, CFG_SERVER_APPEND_PRESENTATION_URL_TO)->getOption();
    if (((temp == "ip") || (temp == "port")) && getOption(CFG_SERVER_PRESENTATION_URL).empty()) {
        throw std::runtime_error("Error in config file: \"append-to\" attribute "
                                 "value in <presentationURL> tag is set to \""
            + temp + "\" but no URL is specified");
    }

#ifdef HAVE_JS
    co = findConfigSetup(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT);
    co->makeOption(root, self);

    co = findConfigSetup(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT);
    co->makeOption(root, self);

    setOption(root, CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
#endif

    auto layoutType = setOption(root, CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE)->getOption();

#ifndef HAVE_JS
    if (layoutType == "js")
        throw std::runtime_error("Gerbera was compiled without JS support, "
                                 "however you specified \"js\" to be used for the "
                                 "virtual-layout.");
#else
    charset = setOption(root, CFG_IMPORT_SCRIPTING_CHARSET)->getOption();
    if (layoutType == "js") {
        try {
            auto conv = std::make_unique<StringConverter>(charset,
                DEFAULT_INTERNAL_CHARSET);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Error in config file: unsupported import script charset specified: " + charset);
        }
    }

    co = findConfigSetup(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
    args["isFile"] = std::to_string(true);
    args["mustExist"] = std::to_string(layoutType == "js");
    args["notEmpty"] = std::to_string(layoutType == "js");
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    co->makeOption(root, self, &args);
    args.clear();
    auto script_path = co->getValue()->getOption();

#endif
    co = findConfigSetup(CFG_SERVER_PORT);
    // 0 means, that the SDK will any free port itself
    co->makeOption((port == 0) ? co->getXmlContent(root) : std::to_string(port), self);

    setOption(root, CFG_SERVER_ALIVE_INTERVAL);
    setOption(root, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    auto useInotify = setOption(root, CFG_IMPORT_AUTOSCAN_USE_INOTIFY)->getBoolOption();

    args["hiddenFiles"] = getBoolOption(CFG_IMPORT_HIDDEN_FILES) ? "true" : "false";
    setOption(root, CFG_IMPORT_AUTOSCAN_TIMED_LIST, &args);

#ifdef HAVE_INOTIFY
    if (useInotify) {
        setOption(root, CFG_IMPORT_AUTOSCAN_INOTIFY_LIST, &args);
    }
#endif
    args.clear();

    auto tr_en = setOption(root, CFG_TRANSCODING_TRANSCODING_ENABLED)->getBoolOption();
    setOption(root, CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED);
    setOption(root, CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED);
    args["isEnabled"] = tr_en ? "true" : "false";
    setOption(root, CFG_TRANSCODING_PROFILE_LIST, &args);
    args.clear();

#ifdef HAVE_CURL
    if (tr_en) {
        setOption(root, CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);
        setOption(root, CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }
#endif //HAVE_CURL

    setOption(root, CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
    setOption(root, CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    setOption(root, CFG_IMPORT_DIRECTORIES_LIST);

    args["trim"] = "false";
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

#ifdef HAVE_LIBEXIF
    setOption(root, CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
    setOption(root, CFG_IMPORT_LIBOPTS_EXIF_CHARSET);
#endif // HAVE_LIBEXIF

#ifdef HAVE_EXIV2
    setOption(root, CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
    setOption(root, CFG_IMPORT_LIBOPTS_EXIV2_CHARSET);
#endif // HAVE_EXIV2

#ifdef HAVE_TAGLIB
    setOption(root, CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    setOption(root, CFG_IMPORT_LIBOPTS_ID3_CHARSET);
#endif

#ifdef HAVE_FFMPEG
    setOption(root, CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    setOption(root, CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET);
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    auto ffmp_en = setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)->getBoolOption();
    if (ffmp_en) {
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
    }
#endif

    bool markingEnabled = setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED)->getBoolOption();
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);
    bool contentArrayEmpty = setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST)->getArrayOption().empty();
    if (markingEnabled && contentArrayEmpty) {
        throw std::runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag");
    }

#if defined(HAVE_LASTFMLIB)
    auto lfm_en = setOption(root, CFG_SERVER_EXTOPTS_LASTFM_ENABLED)->getBoolOption();
    if (lfm_en) {
        setOption(root, CFG_SERVER_EXTOPTS_LASTFM_USERNAME);
        setOption(root, CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

#ifdef HAVE_MAGIC
    co = findConfigSetup(CFG_IMPORT_MAGIC_FILE);
    args["isFile"] = "true";
    args["resolveEmpty"] = "false";
    co->makeOption(!magic_file.empty() ? magic_file.string() : co->getXmlContent(root), self, &args);
    args.clear();
#endif

#ifdef HAVE_INOTIFY
    auto config_timed_list = getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    auto config_inotify_list = getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);

    for (size_t i = 0; i < config_inotify_list->size(); i++) {
        auto i_dir = config_inotify_list->get(i);
        for (size_t j = 0; j < config_timed_list->size(); j++) {
            auto t_dir = config_timed_list->get(j);
            if (i_dir->getLocation() == t_dir->getLocation())
                throw std::runtime_error("Error in config file: same path used in both inotify and timed scan modes");
        }
    }
#endif

#ifdef SOPCAST
    setOption(root, CFG_ONLINE_CONTENT_SOPCAST_ENABLED);

    int sopcast_refresh = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_REFRESH)->getIntOption();
    int sopcast_purge = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER)->getIntOption();

    if (sopcast_refresh >= sopcast_purge) {
        if (sopcast_purge != 0)
            throw std::runtime_error("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }

    setOption(root, CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START);
#endif

#ifdef ATRAILERS
    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);
    int atrailers_refresh = setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_REFRESH)->getIntOption();

    co = findConfigSetup(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
    co->makeOption(std::to_string(atrailers_refresh), self);

    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);
    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str().c_str());

#ifdef TOMBDEBUG
    dumpOptions();
#endif

    // now the XML is no longer needed we can destroy it
    xmlDoc = nullptr;
}

void ConfigManager::updateConfigFromDatabase(std::shared_ptr<Database> database)
{
    auto values = database->getConfigValues();
    auto self = getSelf();
    origValues.clear();
    log_info("Loading {} configuration items from database", values.size());

    for (const auto& cfgValue : values) {
        try {
            auto cs = ConfigManager::findConfigSetupByPath(cfgValue.key, true);

            if (cs != nullptr) {
                if (cfgValue.item == cs->xpath) {
                    origValues[cfgValue.item] = cs->getCurrentValue();
                    cs->makeOption(cfgValue.value, self);
                } else {
                    std::string parValue = cfgValue.value;
                    if (cfgValue.status == STATUS_CHANGED || cfgValue.status == STATUS_UNCHANGED) {
                        if (!cs->updateDetail(cfgValue.item, parValue, self)) {
                            log_error("unhandled option {} != {}", cfgValue.item, cs->xpath);
                        }
                    } else if (cfgValue.status == STATUS_REMOVED || cfgValue.status == STATUS_ADDED || cfgValue.status == STATUS_MANUAL) {
                        std::map<std::string, std::string> arguments = { { "status", cfgValue.status } };
                        if (!cs->updateDetail(cfgValue.item, parValue, self, &arguments)) {
                            log_error("unhandled option {} != {}", cfgValue.item, cs->xpath);
                        }
                    }
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("error setting option {}. Exception {}", cfgValue.key, e.what());
        }
    }
}

void ConfigManager::setOrigValue(const std::string& item, const std::string& value)
{
    if (origValues.find(item) == origValues.end()) {
        log_debug("Caching {}='{}'", item, value);
        origValues[item] = value;
    }
}

void ConfigManager::setOrigValue(const std::string& item, bool value)
{
    if (origValues.find(item) == origValues.end()) {
        origValues[item] = value ? "true" : "false";
    }
}

void ConfigManager::setOrigValue(const std::string& item, int value)
{
    if (origValues.find(item) == origValues.end()) {
        origValues[item] = fmt::format("{}", value);
    }
}

void ConfigManager::dumpOptions() const
{
#ifdef TOMBDEBUG
    log_debug("Dumping options!");
    for (int i = 0; i < static_cast<int>(CFG_MAX); i++) {
        try {
            std::string opt = getOption(static_cast<config_option_t>(i));
            log_debug("    Option {:02d} {}", i, opt.c_str());
        } catch (const std::runtime_error& e) {
        }
        try {
            int opt = getIntOption(static_cast<config_option_t>(i));
            log_debug(" IntOption {:02d} {}", i, opt);
        } catch (const std::runtime_error& e) {
        }
        try {
            bool opt = getBoolOption(static_cast<config_option_t>(i));
            log_debug("BoolOption {:02d} {}", i, opt ? "true" : "false");
        } catch (const std::runtime_error& e) {
        }
    }
#endif
}

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(config_option_t option) const
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getOption();
}

int ConfigManager::getIntOption(config_option_t option) const
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getIntOption();
}

bool ConfigManager::getBoolOption(config_option_t option) const
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getBoolOption();
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(config_option_t option) const
{
    return options->at(option)->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getArrayOption(config_option_t option) const
{
    return options->at(option)->getArrayOption();
}

std::shared_ptr<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option) const
{
    return options->at(option)->getAutoscanListOption();
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(config_option_t option) const
{
    return options->at(option)->getClientConfigListOption();
}

std::shared_ptr<DirectoryConfigList> ConfigManager::getDirectoryTweakOption(config_option_t option) const
{
    return options->at(option)->getDirectoryTweakOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option) const
{
    return options->at(option)->getTranscodingProfileListOption();
}

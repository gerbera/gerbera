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
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "transcoding/transcoding.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

bool ConfigManager::debug_logging = false;

ConfigManager::ConfigManager(fs::path filename,
    const fs::path& userhome, const fs::path& config_dir,
    fs::path prefix_dir, fs::path magic_file,
    std::string ip, std::string interface, int port,
    bool debug_logging)
    : filename(filename)
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

    if (filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userhome / config_dir;
        filename += home / DEFAULT_CONFIG_NAME;
    }

    std::error_code ec;
    if (!isRegularFile(filename, ec)) {
        std::ostringstream expErrMsg;
        expErrMsg << "\nThe server configuration file could not be found: ";
        expErrMsg << filename << "\n";
        expErrMsg << "Gerbera could not find a default configuration file.\n";
        expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
        expErrMsg << "For a list of options run: gerbera -h\n";

        throw std::runtime_error(expErrMsg.str());
    }

    load(filename, userhome);

#ifdef TOMBDEBUG
    dumpOptions();
#endif

    // now the XML is no longer needed we can destroy it
    xmlDoc = nullptr;
}

ConfigManager::~ConfigManager()
{
    log_debug("ConfigManager destroyed");
}

#define NEW_OPTION(optval) opt = std::make_shared<Option>(optval)
#define SET_OPTION(opttype) options->at(opttype) = opt

#define NEW_INT_OPTION(optval) int_opt = std::make_shared<IntOption>(optval)
#define SET_INT_OPTION(opttype) options->at(opttype) = int_opt

#define NEW_BOOL_OPTION(optval) bool_opt = std::make_shared<BoolOption>(optval)
#define SET_BOOL_OPTION(opttype) options->at(opttype) = bool_opt

#define NEW_DICT_OPTION(optval) dict_opt = std::make_shared<DictionaryOption>(optval)
#define SET_DICT_OPTION(opttype) options->at(opttype) = dict_opt

#define NEW_STRARR_OPTION(optval) str_array_opt = std::make_shared<ArrayOption>(optval)
#define SET_STRARR_OPTION(opttype) options->at(opttype) = str_array_opt

#define NEW_AUTOSCANLIST_OPTION(optval) alist_opt = std::make_shared<AutoscanListOption>(optval)
#define SET_AUTOSCANLIST_OPTION(opttype) options->at(opttype) = alist_opt

#define NEW_CLIENTCONFIGLIST_OPTION(optval) cclist_opt = std::make_shared<ClientConfigListOption>(optval)
#define SET_CLIENTCONFIGLIST_OPTION(opttype) options->at(opttype) = cclist_opt

#define NEW_TRANSCODING_PROFILELIST_OPTION(optval) trlist_opt = std::make_shared<TranscodingProfileListOption>(optval)
#define SET_TRANSCODING_PROFILELIST_OPTION(opttype) options->at(opttype) = trlist_opt

static std::map<config_option_t, const char*> optionMap = {
    { CFG_SERVER_PORT, "/server/port" },
    { CFG_SERVER_IP, "/server/ip" },
    { CFG_SERVER_NETWORK_INTERFACE, "/server/interface" },
    { CFG_SERVER_NAME, "/server/name" },
    { CFG_SERVER_MANUFACTURER, "/server/manufacturer" },
    { CFG_SERVER_MANUFACTURER_URL, "/server/manufacturerURL" },
    { CFG_SERVER_MODEL_NAME, "/server/modelName" },
    { CFG_SERVER_MODEL_DESCRIPTION, "/server/modelDescription" },
    { CFG_SERVER_MODEL_NUMBER, "/server/modelNumber" },
    { CFG_SERVER_MODEL_URL, "/server/modelURL" },
    { CFG_SERVER_SERIAL_NUMBER, "/server/serialNumber" },
    { CFG_SERVER_PRESENTATION_URL, "/server/presentationURL" },
    { CFG_SERVER_APPEND_PRESENTATION_URL_TO, "/server/presentationURL/attribute::append-to" },
    { CFG_SERVER_UDN, "/server/udn" },
    { CFG_SERVER_HOME, "/server/home" },
    { CFG_SERVER_TMPDIR, "/server/tmpdir" },
    { CFG_SERVER_WEBROOT, "/server/webroot" },
    { CFG_SERVER_SERVEDIR, "/server/servedir" },
    { CFG_SERVER_ALIVE_INTERVAL, "/server/alive" },
    { CFG_SERVER_HIDE_PC_DIRECTORY, "/server/pc-directory/attribute::upnp-hide" },
    { CFG_SERVER_BOOKMARK_FILE, "/server/bookmark" },
    { CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT, "/server/upnp-string-limit" },
    { CFG_SERVER_UI_ENABLED, "/server/ui/attribute::enabled" },
    { CFG_SERVER_UI_POLL_INTERVAL, "/server/ui/attribute::poll-interval" },
    { CFG_SERVER_UI_POLL_WHEN_IDLE, "/server/ui/attribute::poll-when-idle" },
    { CFG_SERVER_UI_ACCOUNTS_ENABLED, "/server/ui/accounts/attribute::enabled" },
    { CFG_SERVER_UI_ACCOUNT_LIST, "/server/ui/accounts" },
    { CFG_SERVER_UI_SESSION_TIMEOUT, "/server/ui/accounts/attribute::session-timeout" },
    { CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE, "/server/ui/items-per-page/attribute::default" },
    { CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, "/server/ui/items-per-page" },
    { CFG_SERVER_UI_SHOW_TOOLTIPS, "/server/ui/attribute::show-tooltips" },
    { CFG_SERVER_STORAGE, "/server/storage" },
    { CFG_SERVER_STORAGE_MYSQL, "/server/storage/mysql" },
    { CFG_SERVER_STORAGE_SQLITE, "/server/storage/sqlite3" },
    { CFG_SERVER_STORAGE_DRIVER, "/server/storage/driver" },
    { CFG_SERVER_STORAGE_SQLITE_ENABLED, "/server/storage/sqlite3/attribute::enabled" },
    { CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE, "/server/storage/sqlite3/database-file" },
    { CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS, "/server/storage/sqlite3/synchronous" },
    { CFG_SERVER_STORAGE_SQLITE_RESTORE, "/server/storage/sqlite3/on-error" },
    { CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED, "/server/storage/sqlite3/backup/attribute::enabled" },
    { CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, "/server/storage/sqlite3/backup/attribute::interval" },
#ifdef HAVE_MYSQL
    { CFG_SERVER_STORAGE_MYSQL_ENABLED, "/server/storage/mysql/attribute::enabled" },
    { CFG_SERVER_STORAGE_MYSQL_HOST, "/server/storage/mysql/host" },
    { CFG_SERVER_STORAGE_MYSQL_PORT, "/server/storage/mysql/port" },
    { CFG_SERVER_STORAGE_MYSQL_USERNAME, "/server/storage/mysql/username" },
    { CFG_SERVER_STORAGE_MYSQL_SOCKET, "/server/storage/mysql/socket" },
    { CFG_SERVER_STORAGE_MYSQL_PASSWORD, "/server/storage/mysql/password" },
    { CFG_SERVER_STORAGE_MYSQL_DATABASE, "/server/storage/mysql/database" },
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS, "/server/extended-runtime-options/ffmpegthumbnailer/workaround-bugs" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, "/server/extended-runtime-options/ffmpegthumbnailer/image-quality" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled" },
    { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir" },
#endif
    { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED, "/server/extended-runtime-options/mark-played-items/attribute::enabled" },
    { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, "/server/extended-runtime-options/mark-played-items/string/attribute::mode" },
    { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, "/server/extended-runtime-options/mark-played-items/string" },
    { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES, "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates" },
    { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, "/server/extended-runtime-options/mark-played-items/mark" },
#ifdef HAVE_LASTFMLIB
    { CFG_SERVER_EXTOPTS_LASTFM_ENABLED, "/server/extended-runtime-options/lastfm/attribute::enabled" },
    { CFG_SERVER_EXTOPTS_LASTFM_USERNAME, "/server/extended-runtime-options/lastfm/username" },
    { CFG_SERVER_EXTOPTS_LASTFM_PASSWORD, "/server/extended-runtime-options/lastfm/password" },
#endif
    { CFG_IMPORT_HIDDEN_FILES, "/import/attribute::hidden-files" },
    { CFG_IMPORT_FOLLOW_SYMLINKS, "/import/attribute::follow-symlinks" },
    { CFG_IMPORT_FILESYSTEM_CHARSET, "/import/filesystem-charset" },
    { CFG_IMPORT_METADATA_CHARSET, "/import/metadata-charset" },
    { CFG_IMPORT_PLAYLIST_CHARSET, "/import/playlist-charset" },
#ifdef HAVE_JS
    { CFG_IMPORT_SCRIPTING_CHARSET, "/import/scripting/attribute::script-charset" },
    { CFG_IMPORT_SCRIPTING_COMMON_SCRIPT, "/import/scripting/common-script" },
    { CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT, "/import/scripting/playlist-script" },
    { CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS, "/import/scripting/playlist-script/attribute::create-link" },
    { CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT, "/import/scripting/virtual-layout/import-script" },
#endif // JS
    { CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE, "/import/scripting/virtual-layout/attribute::type" },
#ifdef HAVE_MAGIC
    { CFG_IMPORT_MAGIC_FILE, "/import/magic-file" },
#endif
    { CFG_IMPORT_AUTOSCAN_TIMED_LIST, "/import/autoscan" },
#ifdef HAVE_INOTIFY
    { CFG_IMPORT_AUTOSCAN_USE_INOTIFY, "/import/autoscan/attribute::use-inotify" },
    { CFG_IMPORT_AUTOSCAN_INOTIFY_LIST, "/import/autoscan" },
#endif
    { CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS, "/import/mappings/extension-mimetype/attribute::ignore-unknown" },
    { CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE, "/import/mappings/extension-mimetype/attribute::case-sensitive" },
    { CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "/import/mappings/extension-mimetype" },
    { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "/import/mappings/mimetype-upnpclass" },
    { CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "/import/mappings/mimetype-contenttype" },
#ifdef HAVE_LIBEXIF
    { CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, "/import/library-options/libexif/auxdata" },
#endif
#ifdef HAVE_EXIV2
    { CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST, "/import/library-options/exiv2/auxdata" },
#endif
#if defined(HAVE_TAGLIB)
    { CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, "/import/library-options/id3/auxdata" },
#endif
    { CFG_TRANSCODING_TRANSCODING_ENABLED, "/transcoding/attribute::enabled" },
    { CFG_TRANSCODING_PROFILE_LIST, "/transcoding" },
#ifdef HAVE_CURL
    { CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, "/transcoding/attribute::fetch-buffer-size" },
    { CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE, "/transcoding/attribute::fetch-buffer-fill-size" },
#endif //HAVE_CURL
#ifdef SOPCAST
    { CFG_ONLINE_CONTENT_SOPCAST_ENABLED, "/import/online-content/SopCast/attribute::enabled" },
    { CFG_ONLINE_CONTENT_SOPCAST_REFRESH, "/import/online-content/SopCast/attribute::refresh" },
    { CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START, "/import/online-content/SopCast/attribute::update-at-start" },
    { CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER, "/import/online-content/SopCast/attribute::purge-after" },
#endif
#ifdef ATRAILERS
    { CFG_ONLINE_CONTENT_ATRAILERS_ENABLED, "/import/online-content/AppleTrailers/attribute::enabled" },
    { CFG_ONLINE_CONTENT_ATRAILERS_REFRESH, "/import/online-content/AppleTrailers/attribute::refresh" },
    { CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START, "/import/online-content/AppleTrailers/attribute::update-at-start" },
    { CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER, "/import/online-content/AppleTrailers/attribute::purge-after" },
    { CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION, "/import/online-content/AppleTrailers/attribute::resolution" },
#endif
#if defined(HAVE_FFMPEG)
    { CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, "/import/library-options/ffmpeg/auxdata" },
#endif
    { CFG_CLIENTS_LIST, "/clients" },
    { CFG_CLIENTS_LIST_ENABLED, "/clients/attribute::enabled" },
    { CFG_IMPORT_LAYOUT_PARENT_PATH, "/import/layout/attribute::parent-path" },
    { CFG_IMPORT_LAYOUT_MAPPING, "/import/layout" },
    { CFG_IMPORT_LIBOPTS_ENTRY_SEP, "/import/library-options/attribute::multi-value-separator" },
    { CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, "/import/library-options/attribute::legacy-value-separator" },
    { CFG_IMPORT_RESOURCES_CASE_SENSITIVE, "/import/resources/attribute::case-sensitive" },
    { CFG_IMPORT_RESOURCES_FANART_FILE_LIST, "/import/resources/fanart" },
    { CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, "/import/resources/subtitle" },
    { CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, "/import/resources/resource" },

    { CFG_MAX, "last_option" },

    { ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, "option" },
    { ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, "content" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, "acount" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_USER, "user" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD, "password" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, "map" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, "from" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_TO, "to" },
    { ATTR_IMPORT_RESOURCES_ADD_FILE, "add-file" },
    { ATTR_IMPORT_RESOURCES_NAME, "name" },
    { ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, "add-data" },
    { ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, "tag" },

    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP, "mimetype-profile-mappings" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, "transcode" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, "mimetype" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING, "using" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, "treat" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, "mimetype" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, "as" },
    { ATTR_IMPORT_LAYOUT_MAPPING_PATH, "path" },
    { ATTR_IMPORT_LAYOUT_MAPPING_FROM, "from" },
    { ATTR_IMPORT_LAYOUT_MAPPING_TO, "to" },
    { ATTR_TRANSCODING_PROFILES, "profiles" },
    { ATTR_TRANSCODING_PROFILES_PROFLE, "profile" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, "enabled" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "type" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "name" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "mimetype" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_RES, "resolution" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, "avi-fourcc-list" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE, "mode" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, "fourcc" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, "accept-url" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ, "sample-frequency" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN, "audio-channels" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG, "hide-original-resource" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_THUMB, "thumbnail" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, "first-resource" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, "accept-ogg-theora" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC, "use-chunked-encoding" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, "agent" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "command" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "arguments" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, "buffer" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, "size" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, "chunk-size" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, "fill-size" },

    { ATTR_AUTOSCAN_DIRECTORY, "directory" },
    { ATTR_AUTOSCAN_DIRECTORY_LOCATION, "location" },
    { ATTR_AUTOSCAN_DIRECTORY_MODE, "mode" },
    { ATTR_AUTOSCAN_DIRECTORY_INTERVAL, "interval" },
    { ATTR_AUTOSCAN_DIRECTORY_RECURSIVE, "recursive" },
    { ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, "hidden-files" },

    { ATTR_CLIENTS_CLIENT, "client" },
    { ATTR_CLIENTS_CLIENT_FLAGS, "flags" },
    { ATTR_CLIENTS_CLIENT_IP, "ip" },
    { ATTR_CLIENTS_CLIENT_USERAGENT, "userAgent" },
};

const char* ConfigManager::mapConfigOption(config_option_t option)
{
    return optionMap[option];
}

void ConfigManager::load(const fs::path& filename, const fs::path& userHome)
{
    std::string temp;
    int temp_int;
    pugi::xml_node tmpEl;

    std::shared_ptr<Option> opt;
    std::shared_ptr<BoolOption> bool_opt;
    std::shared_ptr<IntOption> int_opt;
    std::shared_ptr<DictionaryOption> dict_opt;
    std::shared_ptr<ArrayOption> str_array_opt;
    std::shared_ptr<AutoscanListOption> alist_opt;
    std::shared_ptr<ClientConfigListOption> cclist_opt;
    std::shared_ptr<TranscodingProfileListOption> trlist_opt;

    log_info("Loading configuration from: {}", filename.c_str());
    this->filename = filename;
    pugi::xml_parse_result result = xmlDoc->load_file(filename.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }

    log_info("Checking configuration...");

    auto root = xmlDoc->document_element();

    // first check if the config file itself looks ok, it must have a config
    // and a server tag
    if (std::string(root.name()) != "config")
        throw std::runtime_error("Error in config file: <config> tag not found");

    if (root.child("server") == nullptr)
        throw std::runtime_error("Error in config file: <server> tag not found");

    std::string version = root.attribute("version").as_string();
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw std::runtime_error("Config version \"" + version + "\" does not yet exist");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server

    if (!userHome.empty()) {
        // respect command line; ignore xml value
        temp = userHome;
    } else
        temp = getXmlOption(CFG_SERVER_HOME);
    if (!fs::is_directory(temp))
        throw std::runtime_error("Directory '" + temp + "' does not exist");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_HOME);

    temp = getXmlOption(CFG_SERVER_WEBROOT);
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_WEBROOT);

    temp = getXmlOption(CFG_SERVER_TMPDIR, DEFAULT_TMPDIR);
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_TMPDIR);

    temp = getXmlOption(CFG_SERVER_SERVEDIR, "");
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    NEW_OPTION(getXmlOption(CFG_SERVER_UDN));
    SET_OPTION(CFG_SERVER_UDN);

    // checking database driver options
    std::string mysql_en = "no";
    std::string sqlite3_en = "no";

    tmpEl = getXmlElement(CFG_SERVER_STORAGE);
    if (tmpEl == nullptr)
        throw std::runtime_error("Error in config file: <storage> tag not found");

    tmpEl = getXmlElement(CFG_SERVER_STORAGE_MYSQL);
    if (tmpEl != nullptr) {
        mysql_en = getXmlOption(CFG_SERVER_STORAGE_MYSQL_ENABLED, DEFAULT_MYSQL_ENABLED);
        NEW_OPTION(mysql_en);
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_ENABLED);
        if (!validateYesNo(mysql_en))
            throw std::runtime_error("Invalid <mysql enabled=\"\"> value");
    }

    tmpEl = getXmlElement(CFG_SERVER_STORAGE_SQLITE);
    if (tmpEl != nullptr) {
        sqlite3_en = getXmlOption(CFG_SERVER_STORAGE_SQLITE_ENABLED, DEFAULT_SQLITE_ENABLED);
        NEW_OPTION(sqlite3_en);
        SET_OPTION(CFG_SERVER_STORAGE_SQLITE_ENABLED);
        if (!validateYesNo(sqlite3_en))
            throw std::runtime_error("Invalid <sqlite3 enabled=\"\"> value");
    }

    if ((sqlite3_en == "yes") && (mysql_en == "yes"))
        throw std::runtime_error("You enabled both, sqlite3 and mysql but "
                                 "only one database driver may be active at a time");

    if ((sqlite3_en == "no") && (mysql_en == "no"))
        throw std::runtime_error("You disabled both, sqlite3 and mysql but "
                                 "one database driver must be active");

#ifdef HAVE_MYSQL
    if (mysql_en == "yes") {
        NEW_OPTION(getXmlOption(CFG_SERVER_STORAGE_MYSQL_HOST, DEFAULT_MYSQL_HOST));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_HOST);

        NEW_OPTION(getXmlOption(CFG_SERVER_STORAGE_MYSQL_DATABASE, DEFAULT_MYSQL_DB));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_DATABASE);

        NEW_OPTION(getXmlOption(CFG_SERVER_STORAGE_MYSQL_USERNAME, DEFAULT_MYSQL_USER));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_USERNAME);

        NEW_INT_OPTION(getXmlIntOption(CFG_SERVER_STORAGE_MYSQL_PORT, 0));
        SET_INT_OPTION(CFG_SERVER_STORAGE_MYSQL_PORT);

        if (getXmlElement(CFG_SERVER_STORAGE_MYSQL_SOCKET) == nullptr) {
            NEW_OPTION("");
        } else {
            NEW_OPTION(getXmlOption(CFG_SERVER_STORAGE_MYSQL_SOCKET));
        }

        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_SOCKET);

        if (getXmlElement(CFG_SERVER_STORAGE_MYSQL_PASSWORD) == nullptr) {
            NEW_OPTION("");
        } else {
            NEW_OPTION(getXmlOption(CFG_SERVER_STORAGE_MYSQL_PASSWORD));
        }
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    }
#else
    if (mysql_en == "yes") {
        throw std::runtime_error("You enabled MySQL storage in configuration, "
                                 "however this version of Gerbera was compiled "
                                 "without MySQL support!");
    }
#endif // HAVE_MYSQL

    if (sqlite3_en == "yes") {
        temp = getXmlOption(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
        temp = resolvePath(temp, true, false);
        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

        temp = getXmlOption(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS, DEFAULT_SQLITE_SYNC);

        temp_int = 0;

        if (temp == "off")
            temp_int = MT_SQLITE_SYNC_OFF;
        else if (temp == "normal")
            temp_int = MT_SQLITE_SYNC_NORMAL;
        else if (temp == "full")
            temp_int = MT_SQLITE_SYNC_FULL;
        else
            throw std::runtime_error("Invalid <synchronous> value in sqlite3 section");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);

        temp = getXmlOption(CFG_SERVER_STORAGE_SQLITE_RESTORE,
            DEFAULT_SQLITE_RESTORE);

        bool tmp_bool = true;

        if (temp == "restore")
            tmp_bool = true;
        else if (temp == "fail")
            tmp_bool = false;
        else
            throw std::runtime_error("Invalid <on-error> value in sqlite3 section");

        NEW_BOOL_OPTION(tmp_bool);
        SET_BOOL_OPTION(CFG_SERVER_STORAGE_SQLITE_RESTORE);
#ifdef SQLITE_BACKUP_ENABLED
        temp = getXmlOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
            YES);
#else
        temp = getXmlOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED,
            DEFAULT_SQLITE_BACKUP_ENABLED);
#endif
        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: incorrect parameter "
                                     "for <backup enabled=\"\" /> attribute");
        NEW_BOOL_OPTION(temp == "yes");
        SET_BOOL_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);

        temp_int = getXmlIntOption(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL,
            DEFAULT_SQLITE_BACKUP_INTERVAL);
        if (temp_int < 1)
            throw std::runtime_error("Error in config file: incorrect parameter for "
                                     "<backup interval=\"\" /> attribute");
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
    }

    std::string dbDriver;
    if (sqlite3_en == "yes")
        dbDriver = "sqlite3";

    if (mysql_en == "yes")
        dbDriver = "mysql";

    NEW_OPTION(dbDriver);
    SET_OPTION(CFG_SERVER_STORAGE_DRIVER);

    // now go through the optional settings and fix them if anything is missing

    temp = getXmlOption(CFG_SERVER_UI_ENABLED, DEFAULT_UI_EN_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <ui enabled=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_ENABLED);

    temp = getXmlOption(CFG_SERVER_UI_SHOW_TOOLTIPS,
        DEFAULT_UI_SHOW_TOOLTIPS_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <ui show-tooltips=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_SHOW_TOOLTIPS);

    temp = getXmlOption(CFG_SERVER_UI_POLL_WHEN_IDLE,
        DEFAULT_POLL_WHEN_IDLE_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <ui poll-when-idle=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_POLL_WHEN_IDLE);

    temp_int = getXmlIntOption(CFG_SERVER_UI_POLL_INTERVAL,
        DEFAULT_POLL_INTERVAL);
    if (temp_int < 1)
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<ui poll-interval=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_POLL_INTERVAL);

    temp_int = getXmlIntOption(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE,
        DEFAULT_ITEMS_PER_PAGE_2);
    if (temp_int < 1)
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<items-per-page default=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

    // now get the option list for the drop down menu
    tmpEl = getXmlElement(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
    // create default structure
    if (std::distance(tmpEl.begin(), tmpEl.end()) == 0) {
        if ((temp_int != DEFAULT_ITEMS_PER_PAGE_1) && (temp_int != DEFAULT_ITEMS_PER_PAGE_2) && (temp_int != DEFAULT_ITEMS_PER_PAGE_3) && (temp_int != DEFAULT_ITEMS_PER_PAGE_4)) {
            throw std::runtime_error("Error in config file: you specified an "
                                     "<items-per-page default=\"\"> value that is "
                                     "not listed in the options");
        }

        tmpEl.append_child("option").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_1).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_2).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_3).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_4).c_str());
    } else // validate user settings
    {
        bool default_found = false;
        for (const pugi::xml_node& child : tmpEl.children()) {
            if (std::string(child.name()) == optionMap[ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION]) {
                int i = child.text().as_int();
                if (i < 1)
                    throw std::runtime_error("Error in config file: incorrect "
                                             "<option> value for <items-per-page>");

                if (i == temp_int)
                    default_found = true;
            }
        }

        if (!default_found)
            throw std::runtime_error("Error in config file: at least one <option> "
                                     "under <items-per-page> must match the "
                                     "<items-per-page default=\"\" /> attribute");
    }

    // create the array from either user or default settings
    std::vector<std::string> menu_opts;
    for (const pugi::xml_node& child : tmpEl.children()) {
        if (std::string(child.name()) == "option")
            menu_opts.emplace_back(child.text().as_string());
    }
    NEW_STRARR_OPTION(menu_opts);
    SET_STRARR_OPTION(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);

    temp = getXmlOption(CFG_SERVER_UI_ACCOUNTS_ENABLED, DEFAULT_ACCOUNTS_EN_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<accounts enabled=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_ACCOUNTS_ENABLED);

    tmpEl = getXmlElement(CFG_SERVER_UI_ACCOUNT_LIST);
    NEW_DICT_OPTION(createDictionaryFromNode(tmpEl, ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD));
    SET_DICT_OPTION(CFG_SERVER_UI_ACCOUNT_LIST);

    temp_int = getXmlIntOption(CFG_SERVER_UI_SESSION_TIMEOUT,
        DEFAULT_SESSION_TIMEOUT);
    if (temp_int < 1) {
        throw std::runtime_error("Error in config file: invalid session-timeout "
                                 "(must be > 0)");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_SESSION_TIMEOUT);

    temp = getXmlOption(CFG_CLIENTS_LIST_ENABLED,
        DEFAULT_CLIENTS_EN_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<clients enabled=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_CLIENTS_LIST_ENABLED);

    tmpEl = getXmlElement(CFG_CLIENTS_LIST);
    if (tmpEl != nullptr && temp == "yes") {
        NEW_CLIENTCONFIGLIST_OPTION(createClientConfigListFromNode(tmpEl));
        SET_CLIENTCONFIGLIST_OPTION(CFG_CLIENTS_LIST);
    }

    temp = getXmlOption(CFG_IMPORT_HIDDEN_FILES,
        DEFAULT_HIDDEN_FILES_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<import hidden-files=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_HIDDEN_FILES);

    temp = getXmlOption(CFG_IMPORT_FOLLOW_SYMLINKS,
        DEFAULT_FOLLOW_SYMLINKS_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<import follow-symlinks=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_FOLLOW_SYMLINKS);

    temp = getXmlOption(
        CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS,
        DEFAULT_IGNORE_UNKNOWN_EXTENSIONS);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<extension-mimetype ignore-unknown=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    temp = getXmlOption(
        CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE,
        DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<extension-mimetype case-sensitive=\"\" /> attribute");

    bool csens = false;

    if (temp == "yes")
        csens = true;

    NEW_BOOL_OPTION(csens);
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    tmpEl = getXmlElement(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    NEW_DICT_OPTION(createDictionaryFromNode(tmpEl, ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO, !csens));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    std::map<std::string, std::string> mime_content;
    tmpEl = getXmlElement(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    if (tmpEl != nullptr) {
        mime_content = createDictionaryFromNode(tmpEl, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS);
    } else {
        mime_content["audio/mpeg"] = CONTENT_TYPE_MP3;
        mime_content["audio/mp4"] = CONTENT_TYPE_MP4;
        mime_content["video/mp4"] = CONTENT_TYPE_MP4;
        mime_content["application/ogg"] = CONTENT_TYPE_OGG;
        mime_content["audio/x-flac"] = CONTENT_TYPE_FLAC;
        mime_content["audio/flac"] = CONTENT_TYPE_FLAC;
        mime_content["image/jpeg"] = CONTENT_TYPE_JPG;
        mime_content["audio/x-mpegurl"] = CONTENT_TYPE_PLAYLIST;
        mime_content["audio/x-scpls"] = CONTENT_TYPE_PLAYLIST;
        mime_content["audio/x-wav"] = CONTENT_TYPE_PCM;
        mime_content["audio/wave"] = CONTENT_TYPE_PCM;
        mime_content["audio/wav"] = CONTENT_TYPE_PCM;
        mime_content["audio/vnd.wave"] = CONTENT_TYPE_PCM;
        mime_content["audio/L16"] = CONTENT_TYPE_PCM;
        mime_content["audio/x-aiff"] = CONTENT_TYPE_AIFF;
        mime_content["audio/aiff"] = CONTENT_TYPE_AIFF;
        mime_content["video/x-msvideo"] = CONTENT_TYPE_AVI;
        mime_content["video/mpeg"] = CONTENT_TYPE_MPEG;
    }

    NEW_DICT_OPTION(mime_content);
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    temp = getXmlOption(CFG_IMPORT_LAYOUT_PARENT_PATH,
        DEFAULT_IMPORT_LAYOUT_PARENT_PATH);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<layout parent-path=\"\" /> attribute");

    bool ppath = false;

    if (temp == "yes")
        ppath = true;

    NEW_BOOL_OPTION(ppath);
    SET_BOOL_OPTION(CFG_IMPORT_LAYOUT_PARENT_PATH);

    tmpEl = getXmlElement(CFG_IMPORT_LAYOUT_MAPPING);
    std::map<std::string, std::string> layout_mapping_content;
    if (tmpEl != nullptr) {
        layout_mapping_content = createDictionaryFromNode(tmpEl, ATTR_IMPORT_LAYOUT_MAPPING_PATH, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO);
    }

    NEW_DICT_OPTION(layout_mapping_content);
    SET_DICT_OPTION(CFG_IMPORT_LAYOUT_MAPPING);

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
    try {
        auto conv = std::make_unique<StringConverter>(temp,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        temp = DEFAULT_FALLBACK_CHARSET;
    }
    std::string charset = getXmlOption(CFG_IMPORT_FILESYSTEM_CHARSET, temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported filesystem-charset specified: " + charset);
    }

    log_info("Setting filesystem import charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_FILESYSTEM_CHARSET);

    charset = getXmlOption(CFG_IMPORT_METADATA_CHARSET, temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported metadata-charset specified: " + charset);
    }

    log_info("Setting metadata import charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_METADATA_CHARSET);

    charset = getXmlOption(CFG_IMPORT_PLAYLIST_CHARSET, temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported playlist-charset specified: " + charset);
    }

    log_info("Setting playlist charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_PLAYLIST_CHARSET);

    temp = getXmlOption(CFG_SERVER_HIDE_PC_DIRECTORY, DEFAULT_HIDE_PC_DIRECTORY);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: hide attribute of the "
                                 "pc-directory tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_HIDE_PC_DIRECTORY);

    if (interface.empty()) {
        temp = getXmlOption(CFG_SERVER_NETWORK_INTERFACE, "");
    } else {
        temp = interface;
    }
    if (!temp.empty() && !getXmlOption(CFG_SERVER_IP, "").empty())
        throw std::runtime_error("Error in config file: you can not specify interface and ip at the same time");

    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_NETWORK_INTERFACE);

    if (ip.empty()) {
        temp = getXmlOption(CFG_SERVER_IP, ""); // bind to any IP address
    } else {
        temp = ip;
    }
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_IP);

    temp = getXmlOption(CFG_SERVER_BOOKMARK_FILE, DEFAULT_BOOKMARK_FILE);
    temp = resolvePath(temp, true, false);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_BOOKMARK_FILE);

    temp = getXmlOption(CFG_SERVER_NAME, DESC_FRIENDLY_NAME);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_NAME);

    temp = getXmlOption(CFG_SERVER_MODEL_NAME, DESC_MODEL_NAME);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_NAME);

    temp = getXmlOption(CFG_SERVER_MODEL_DESCRIPTION, DESC_MODEL_DESCRIPTION);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_DESCRIPTION);

    temp = getXmlOption(CFG_SERVER_MODEL_NUMBER, DESC_MODEL_NUMBER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_NUMBER);

    temp = getXmlOption(CFG_SERVER_MODEL_URL, "");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_URL);

    temp = getXmlOption(CFG_SERVER_SERIAL_NUMBER, DESC_SERIAL_NUMBER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_SERIAL_NUMBER);

    temp = getXmlOption(CFG_SERVER_MANUFACTURER, DESC_MANUFACTURER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MANUFACTURER);

    temp = getXmlOption(CFG_SERVER_MANUFACTURER_URL, DESC_MANUFACTURER_URL);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MANUFACTURER_URL);

    temp = getXmlOption(CFG_SERVER_PRESENTATION_URL, "");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_PRESENTATION_URL);

    temp = getXmlOption(CFG_SERVER_APPEND_PRESENTATION_URL_TO, DEFAULT_PRES_URL_APPENDTO_ATTR);

    if ((temp != "none") && (temp != "ip") && (temp != "port")) {
        throw std::runtime_error("Error in config file: "
                                 "invalid \"append-to\" attribute value in "
                                 "<presentationURL> tag");
    }

    if (((temp == "ip") || (temp == "port")) && getXmlOption(CFG_SERVER_PRESENTATION_URL).empty()) {
        throw std::runtime_error("Error in config file: \"append-to\" attribute "
                                 "value in <presentationURL> tag is set to \""
            + temp + "\" but no URL is specified");
    }
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_APPEND_PRESENTATION_URL_TO);

    temp_int = getXmlIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT,
        DEFAULT_UPNP_STRING_LIMIT);
    if ((temp_int != -1) && (temp_int < 4)) {
        throw std::runtime_error("Error in config file: invalid value for "
                                 "<upnp-string-limit>");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

#ifdef HAVE_JS
    temp = getXmlOption(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT,
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT);
    temp = resolvePath(temp, true);
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);

    temp = getXmlOption(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT,
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT);
    temp = resolvePath(temp, true);
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);

    temp = getXmlOption(
        CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS,
        DEFAULT_PLAYLIST_CREATE_LINK);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"create-link\" attribute value in <playlist-script> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
#endif

    temp = getXmlOption(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE,
        DEFAULT_LAYOUT_TYPE);
    if ((temp != "js") && (temp != "builtin") && (temp != "disabled"))
        throw std::runtime_error("Error in config file: invalid virtual layout type specified");
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);

#ifndef HAVE_JS
    if (temp == "js")
        throw std::runtime_error("Gerbera was compiled without JS support, "
                                 "however you specified \"js\" to be used for the "
                                 "virtual-layout.");
#else
    charset = getXmlOption(CFG_IMPORT_SCRIPTING_CHARSET, DEFAULT_JS_CHARSET);
    if (temp == "js") {
        try {
            auto conv = std::make_unique<StringConverter>(charset,
                DEFAULT_INTERNAL_CHARSET);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Error in config file: unsupported import script charset specified: " + charset);
        }
    }

    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_SCRIPTING_CHARSET);

    std::string script_path = getXmlOption(
        CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT,
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    script_path = resolvePath(script_path, true, temp == "js");
    if (temp == "js" && script_path.empty())
        throw std::runtime_error("Error in config file: you specified \"js\" to "
                                 "be used for virtual layout, but script location is invalid.");

    NEW_OPTION(script_path);
    SET_OPTION(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
#endif

    // 0 means, that the SDK will any free port itself
    if (port <= 0) {
        temp_int = getXmlIntOption(CFG_SERVER_PORT, 0);
    } else {
        temp_int = port;
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_PORT);

    temp_int = getXmlIntOption(CFG_SERVER_ALIVE_INTERVAL, DEFAULT_ALIVE_INTERVAL);
    if (temp_int < ALIVE_INTERVAL_MIN)
        throw std::runtime_error(fmt::format("Error in config file: incorrect parameter for /server/alive, must be at least {}",
            ALIVE_INTERVAL_MIN));
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_ALIVE_INTERVAL);

    pugi::xml_node el = getXmlElement(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "");
    }
    NEW_DICT_OPTION(createDictionaryFromNode(el, ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    temp = getXmlOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY, "auto");
    if ((temp != "auto") && !validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for \"<autoscan use-inotify=\" attribute");

    el = getXmlElement(CFG_IMPORT_AUTOSCAN_TIMED_LIST);

    NEW_AUTOSCANLIST_OPTION(createAutoscanListFromNode(nullptr, el, ScanMode::Timed));
    SET_AUTOSCANLIST_OPTION(CFG_IMPORT_AUTOSCAN_TIMED_LIST);

#ifdef HAVE_INOTIFY
    bool inotify_supported = false;
    inotify_supported = Inotify::supported();
#endif

    if (temp == YES) {
#ifdef HAVE_INOTIFY
        if (!inotify_supported)
            throw std::runtime_error("You specified "
                                     "\"yes\" in \"<autoscan use-inotify=\"\">"
                                     " however your system does not have inotify support");
#else
        throw std::runtime_error("You specified"
                                 " \"yes\" in \"<autoscan use-inotify=\"\">"
                                 " however this version of Gerbera was compiled without inotify support");
#endif
    }

#ifdef HAVE_INOTIFY
    if (temp == "auto" || (temp == YES)) {
        if (inotify_supported) {
            NEW_AUTOSCANLIST_OPTION(createAutoscanListFromNode(nullptr, el, ScanMode::INotify));
            SET_AUTOSCANLIST_OPTION(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);

            NEW_BOOL_OPTION(true);
            SET_BOOL_OPTION(CFG_IMPORT_AUTOSCAN_USE_INOTIFY);
        } else {
            NEW_BOOL_OPTION(false);
            SET_BOOL_OPTION(CFG_IMPORT_AUTOSCAN_USE_INOTIFY);
        }
    } else {
        NEW_BOOL_OPTION(false);
        SET_BOOL_OPTION(CFG_IMPORT_AUTOSCAN_USE_INOTIFY);
    }
#endif

    temp = getXmlOption(
        CFG_TRANSCODING_TRANSCODING_ENABLED,
        DEFAULT_TRANSCODING_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <transcoding enabled=\"\"> attribute");

    SET_BOOL_OPTION(CFG_TRANSCODING_TRANSCODING_ENABLED);

    if (temp == "yes")
        el = getXmlElement(CFG_TRANSCODING_PROFILE_LIST);
    else
        el = pugi::xml_node(nullptr);
    NEW_TRANSCODING_PROFILELIST_OPTION(createTranscodingProfileListFromNode(el));
    SET_TRANSCODING_PROFILELIST_OPTION(CFG_TRANSCODING_PROFILE_LIST);

#ifdef HAVE_CURL
    if (temp == "yes") {
        temp_int = getXmlIntOption(
            CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE,
            DEFAULT_CURL_BUFFER_SIZE);
        if (temp_int < CURL_MAX_WRITE_SIZE)
            throw std::runtime_error(fmt::format("Error in config file: incorrect parameter "
                                                 "for <transcoding fetch-buffer-size=\"\"> attribute, "
                                                 "must be at least {}",
                CURL_MAX_WRITE_SIZE));
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);

        temp_int = getXmlIntOption(
            CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE,
            DEFAULT_CURL_INITIAL_FILL_SIZE);
        if (temp_int < 0)
            throw std::runtime_error("Error in config file: incorrect parameter "
                                     "for <transcoding fetch-buffer-fill-size=\"\"> attribute");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }

#endif //HAVE_CURL

    temp = getXmlOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP, DEFAULT_LIBOPTS_ENTRY_SEPARATOR, false);

    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_LIBOPTS_ENTRY_SEP);

    temp = getXmlOption(CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, "", false);

    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP);

#ifdef HAVE_LIBEXIF

    el = getXmlElement(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);

#endif // HAVE_LIBEXIF

    temp = getXmlOption(CFG_IMPORT_RESOURCES_CASE_SENSITIVE,
        DEFAULT_RESOURCES_CASE_SENSITIVE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                                 "<resources case-sensitive=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_RESOURCES_CASE_SENSITIVE);

    el = getXmlElement(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_RESOURCES_FANART_FILE_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME));
    SET_STRARR_OPTION(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);

    el = getXmlElement(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME));
    SET_STRARR_OPTION(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);

    el = getXmlElement(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME));
    SET_STRARR_OPTION(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);

#ifdef HAVE_EXIV2

    el = getXmlElement(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
            "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);

#endif // HAVE_EXIV2

#if defined(HAVE_TAGLIB)
    el = getXmlElement(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG)
    el = getXmlElement(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    if (el == nullptr) {
        getXmlOption(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    temp = getXmlOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED,
        DEFAULT_FFMPEGTHUMBNAILER_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"enabled\" attribute value in "
                                 "<ffmpegthumbnailer> tag");

    NEW_BOOL_OPTION(temp == YES);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED);

    if (temp == YES) {
        temp_int = getXmlIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE,
            DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE);

        if (temp_int <= 0)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value attribute value in "
                                     "<thumbnail-size> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);

        temp_int = getXmlIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE,
            DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        if (temp_int < 0)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value attribute value in "
                                     "<seek-percentage> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        temp = getXmlOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY,
            DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value in <filmstrip-overlay> tag");

        NEW_BOOL_OPTION(temp == YES);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        temp = getXmlOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS,
            DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value in <workaround-bugs> tag");

        NEW_BOOL_OPTION(temp == YES);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        temp_int = getXmlIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY,
            DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY);

        if (temp_int < 0)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value attribute value in "
                                     "<image-quality> tag, allowed values: 0-10");

        if (temp_int > 10)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                                     "invalid value attribute value in "
                                     "<image-quality> tag, allowed values: 0-10");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);

        temp = getXmlOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR,
            DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR);

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);

        temp = getXmlOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED,
            DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: "
                                     "invalid \"enabled\" attribute value in "
                                     "ffmpegthumbnailer <cache-dir> tag");

        NEW_BOOL_OPTION(temp == YES);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
    }
#endif

    temp = getXmlOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED,
        DEFAULT_MARK_PLAYED_ITEMS_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"enabled\" attribute value in "
                                 "<mark-played-items> tag");

    bool markingEnabled = temp == YES;
    NEW_BOOL_OPTION(markingEnabled);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED);

    temp = getXmlOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES,
        DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \":suppress-cds-updates\" attribute "
                                 "value in <mark-played-items> tag");

    NEW_BOOL_OPTION(temp == YES);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);

    temp = getXmlOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);

    if ((temp != "prepend") && (temp != "append"))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"mode\" attribute value in "
                                 "<string> tag in the <mark-played-items> section");

    NEW_BOOL_OPTION(temp == DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);

    temp = getXmlOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, DEFAULT_MARK_PLAYED_ITEMS_STRING);
    if (temp.empty())
        throw std::runtime_error("Error in config file: "
                                 "empty string given for the <string> tag in the "
                                 "<mark-played-items> section");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

    std::vector<std::string> mark_content_list;
    tmpEl = getXmlElement(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);

    int contentElementCount = 0;
    if (tmpEl != nullptr) {
        for (const pugi::xml_node& content : tmpEl.children()) {
            if (std::string(content.name()) != optionMap[ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT])
                continue;

            contentElementCount++;

            std::string mark_content = content.text().as_string();
            if (mark_content.empty())
                throw std::runtime_error("error in configuration, <mark-played-items>, empty <content> parameter");

            if ((mark_content != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_IMAGE))
                throw std::runtime_error(R"(error in configuration, <mark-played-items>, invalid <content> parameter! Allowed values are "video", "audio", "image")");

            mark_content_list.push_back(mark_content);
            NEW_STRARR_OPTION(mark_content_list);
            SET_STRARR_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        }
    }

    if (markingEnabled && contentElementCount == 0) {
        throw std::runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag");
    }

#if defined(HAVE_LASTFMLIB)
    temp = getXmlOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED, DEFAULT_LASTFM_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"enabled\" attribute value in <lastfm> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_LASTFM_ENABLED);

    if (temp == YES) {
        temp = getXmlOption(CFG_SERVER_EXTOPTS_LASTFM_USERNAME, DEFAULT_LASTFM_USERNAME);

        if (temp.empty())
            throw std::runtime_error("Error in config file: lastfm - "
                                     "invalid username value in <username> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_USERNAME);

        temp = getXmlOption(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD, DEFAULT_LASTFM_PASSWORD);

        if (temp.empty())
            throw std::runtime_error("Error in config file: lastfm - "
                                     "invalid password value in <password> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

#ifdef HAVE_MAGIC
    if (!magic_file.empty()) {
        // respect command line; ignore xml value
        magic_file = resolvePath(magic_file, true);
    } else {
        magic_file = getXmlOption(CFG_IMPORT_MAGIC_FILE, "");
        if (!magic_file.empty())
            magic_file = resolvePath(magic_file, true);
    }
    NEW_OPTION(magic_file);
    SET_OPTION(CFG_IMPORT_MAGIC_FILE);
#endif

#ifdef HAVE_INOTIFY
    tmpEl = getXmlElement(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    auto config_timed_list = createAutoscanListFromNode(nullptr, tmpEl, ScanMode::Timed);
    auto config_inotify_list = createAutoscanListFromNode(nullptr, tmpEl, ScanMode::INotify);

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
    temp = getXmlOption(CFG_ONLINE_CONTENT_SOPCAST_ENABLED, DEFAULT_SOPCAST_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"enabled\" attribute value in <SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_ENABLED);

    int sopcast_refresh = getXmlIntOption(CFG_ONLINE_CONTENT_SOPCAST_REFRESH, 0);
    NEW_INT_OPTION(sopcast_refresh);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);

    temp_int = getXmlIntOption(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER, 0);
    if (sopcast_refresh >= temp_int) {
        if (temp_int != 0)
            throw std::runtime_error("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }

    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);

    temp = getXmlOption(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START,
        DEFAULT_SOPCAST_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"update-at-start\" attribute value in <SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START);
#endif

#ifdef ATRAILERS
    temp = getXmlOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED,
        DEFAULT_ATRAILERS_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"enabled\" attribute value in <AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);

    temp_int = getXmlIntOption(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH, DEFAULT_ATRAILERS_REFRESH);
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);

    temp = getXmlOption(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START,
        DEFAULT_ATRAILERS_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                                 "invalid \"update-at-start\" attribute value in <AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);

    temp = getXmlOption(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION,
        std::to_string(DEFAULT_ATRAILERS_RESOLUTION));
    if ((temp != "640") && (temp != "720p")) {
        throw std::runtime_error("Error in config file: "
                                 "invalid \"resolution\" attribute value in "
                                 "<AppleTrailers> tag, only \"640\" and \"720p\" is supported");
    }

    NEW_OPTION(temp);
    SET_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str().c_str());
}

std::string ConfigManager::getXmlOption(config_option_t option, std::string def, bool trim) const
{
    auto root = xmlDoc->document_element();
    std::string xpath = std::string("/config") + optionMap[option];
    pugi::xpath_node xpathNode = root.select_node(xpath.c_str());

    if (xpathNode.node() != nullptr) {
        return trim ? trim_string(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
    }

    if (xpathNode.attribute() != nullptr) {
        return trim ? trim_string(xpathNode.attribute().value()) : xpathNode.attribute().value();
    }

    log_debug("Config: option not found: '{}' using default value: '{}'",
        xpath.c_str(), def.c_str());

    return def;
}

int ConfigManager::getXmlIntOption(config_option_t option, int def) const
{
    std::string sDef;
    sDef = std::to_string(def);
    std::string sVal = getXmlOption(option, sDef);
    return std::stoi(sVal);
}

std::string ConfigManager::getXmlOption(config_option_t option) const
{
    std::string xpath = std::string("/config") + optionMap[option];
    auto root = xmlDoc->document_element();
    pugi::xpath_node xpathNode = root.select_node(xpath.c_str());

    if (xpathNode.node() != nullptr) {
        return trim_string(xpathNode.node().text().as_string());
    }

    if (xpathNode.attribute() != nullptr) {
        return trim_string(xpathNode.attribute().value());
    }

    throw std::runtime_error(fmt::format("Option '{}' not found in configuration file", xpath));
}

int ConfigManager::getXmlIntOption(config_option_t option) const
{
    std::string sVal = getXmlOption(option);
    return std::stoi(sVal);
}

pugi::xml_node ConfigManager::getXmlElement(config_option_t option) const
{
    std::string xpath = std::string("/config") + optionMap[option];
    auto root = xmlDoc->document_element();
    pugi::xpath_node xpathNode = root.select_node(xpath.c_str());
    return xpathNode.node();
}

fs::path ConfigManager::resolvePath(fs::path path, bool isFile, bool mustExist)
{
    fs::path home = getOption(CFG_SERVER_HOME);

    if (path.is_absolute() || (home.is_relative() && path.is_relative()))
        ; // absolute or relative, nothing to resolve
    else if (home.empty())
        path = "." / path;
    else
        path = home / path;

    // verify that file/directory is there
    std::error_code ec;
    if (isFile) {
        if (mustExist) {
            if (!isRegularFile(path, ec) && !fs::is_symlink(path, ec))
                throw std::runtime_error("File '" + path.string() + "' does not exist");
        } else {
            std::string parent_path = path.parent_path();
            if (!fs::is_directory(parent_path, ec) && !fs::is_symlink(path, ec))
                throw std::runtime_error("Parent directory '" + path.string() + "' does not exist");
        }
    } else if (mustExist) {
        if (!fs::is_directory(path, ec) && !fs::is_symlink(path, ec))
            throw std::runtime_error("Directory '" + path.string() + "' does not exist");
    }

    return path;
}

std::map<std::string, std::string> ConfigManager::createDictionaryFromNode(const pugi::xml_node& element,
    config_option_t nodeName, config_option_t keyAttr, config_option_t valAttr, bool tolower)
{
    std::map<std::string, std::string> dict;

    if (element != nullptr) {
        for (const pugi::xml_node& child : element.children()) {
            if (std::string(child.name()) == optionMap[nodeName]) {
                std::string key = child.attribute(optionMap[keyAttr]).as_string();
                std::string value = child.attribute(optionMap[valAttr]).as_string();

                if (!key.empty() && !value.empty()) {
                    if (tolower) {
                        key = tolower_string(key);
                    }
                    dict[key] = value;
                }
            }
        }
    }

    return dict;
}

std::shared_ptr<TranscodingProfileList> ConfigManager::createTranscodingProfileListFromNode(const pugi::xml_node& element)
{
    std::string param;
    int param_int;

    auto list = std::make_shared<TranscodingProfileList>();
    if (element == nullptr)
        return list;

    std::map<std::string, std::string> mt_mappings;

    auto mtype_profile = element.child(optionMap[ATTR_TRANSCODING_MIMETYPE_PROF_MAP]);
    if (mtype_profile != nullptr) {
        for (const pugi::xml_node& child : mtype_profile.children()) {
            if (std::string(optionMap[ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE]) == child.name()) {
                std::string mt = child.attribute(optionMap[ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE]).as_string();
                std::string pname = child.attribute(optionMap[ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING]).as_string();

                if (!mt.empty() && !pname.empty()) {
                    mt_mappings[mt] = pname;
                } else {
                    throw std::runtime_error("error in configuration: invalid or missing mimetype to profile mapping");
                }
            }
        }
    }

    auto profiles = element.child(optionMap[ATTR_TRANSCODING_PROFILES]);
    if (profiles == nullptr)
        return list;

    for (const pugi::xml_node& child : profiles.children()) {
        if (std::string(child.name()) != optionMap[ATTR_TRANSCODING_PROFILES_PROFLE])
            continue;

        param = child.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED]).as_string();
        if (!validateYesNo(param))
            throw std::runtime_error("Error in config file: incorrect parameter "
                                     "for <profile enabled=\"\" /> attribute");

        if (param == "no")
            continue;

        param = child.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_TYPE]).as_string();
        if (param.empty())
            throw std::runtime_error("error in configuration: missing transcoding type in profile");

        transcoding_type_t tr_type;
        if (param == "external")
            tr_type = TR_External;
        /* for the future...
        else if (param == "remote")
            tr_type = TR_Remote;
         */
        else
            throw std::runtime_error("error in configuration: invalid transcoding type " + param + " in profile");

        param = child.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_NAME]).as_string();
        if (param.empty())
            throw std::runtime_error("error in configuration: invalid transcoding profile name");

        auto prof = std::make_shared<TranscodingProfile>(tr_type, param);

        pugi::xml_node sub;
        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE]);
        param = sub.text().as_string();
        if (param.empty())
            throw std::runtime_error("error in configuration: invalid target mimetype in transcoding profile");
        prof->setTargetMimeType(param);

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_RES]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!param.empty()) {
                if (check_resolution(param))
                    prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
            }
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC]);
        if (sub != nullptr) {
            std::string mode = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE]).as_string();
            if (mode.empty())
                throw std::runtime_error("error in configuration: avi-fourcc-list requires a valid \"mode\" attribute");

            avi_fourcc_listmode_t fcc_mode;
            if (mode == "ignore")
                fcc_mode = FCC_Ignore;
            else if (mode == "process")
                fcc_mode = FCC_Process;
            else if (mode == "disabled")
                fcc_mode = FCC_None;
            else
                throw std::runtime_error("error in configuration: invalid mode given for avi-fourcc-list: \"" + mode + "\"");

            if (fcc_mode != FCC_None) {
                std::vector<std::string> fcc_list;
                for (const pugi::xml_node& fourcc : sub.children()) {
                    if (std::string(fourcc.name()) != optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC])
                        continue;

                    std::string fcc = fourcc.text().as_string();
                    if (fcc.empty())
                        throw std::runtime_error("error in configuration: empty fourcc specified");
                    fcc_list.push_back(fcc);
                }

                prof->setAVIFourCCList(fcc_list, fcc_mode);
            }
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for <accept-url> tag");
            if (param == "yes")
                prof->setAcceptURL(true);
            else
                prof->setAcceptURL(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (param == "source")
                prof->setSampleFreq(SOURCE);
            else if (param == "off")
                prof->setSampleFreq(OFF);
            else {
                int freq = std::stoi(param);
                if (freq <= 0)
                    throw std::runtime_error("Error in config file: incorrect parameter for <sample-frequency> tag");

                prof->setSampleFreq(freq);
            }
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (param == "source")
                prof->setNumChannels(SOURCE);
            else if (param == "off")
                prof->setNumChannels(OFF);
            else {
                int chan = std::stoi(param);
                if (chan <= 0)
                    throw std::runtime_error("Error in config file: incorrect parameter for <number-of-channels> tag");
                prof->setNumChannels(chan);
            }
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for <hide-original-resource> tag");
            if (param == "yes")
                prof->setHideOriginalResource(true);
            else
                prof->setHideOriginalResource(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_THUMB]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for <thumbnail> tag");
            if (param == "yes")
                prof->setThumbnail(true);
            else
                prof->setThumbnail(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_FIRST]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for <profile first-resource=\"\" /> attribute");

            if (param == "yes")
                prof->setFirstResource(true);
            else
                prof->setFirstResource(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for use-chunked-encoding tag");

            if (param == "yes")
                prof->setChunked(true);
            else
                prof->setChunked(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG]);
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter for accept-ogg-theora tag");

            if (param == "yes")
                prof->setTheora(true);
            else
                prof->setTheora(false);
        }

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AGENT]);
        if (sub == nullptr)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" is missing the <agent> option");

        param = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND]).as_string();
        if (param.empty())
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" has an invalid command setting");
        prof->setCommand(param);

        std::string tmp_path;
        if (fs::path(param).is_absolute()) {
            if (!isRegularFile(param) && !fs::is_symlink(param))
                throw std::runtime_error("error in configuration, transcoding profile \""
                    + prof->getName() + "\" could not find transcoding command " + param);
            tmp_path = param;
        } else {
            tmp_path = find_in_path(param);
            if (tmp_path.empty())
                throw std::runtime_error("error in configuration, transcoding profile \""
                    + prof->getName() + "\" could not find transcoding command " + param + " in $PATH");
        }

        int err = 0;
        if (!is_executable(tmp_path, &err))
            throw std::runtime_error("error in configuration, transcoding profile "
                + prof->getName() + ": transcoder " + param + "is not executable - " + strerror(err));

        param = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS]).as_string();
        if (param.empty())
            throw std::runtime_error("error in configuration: transcoding profile " + prof->getName() + " has an empty argument string");

        prof->setArguments(param);

        sub = child.child(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER]);
        if (sub == nullptr)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" is missing the <buffer> option");

        param_int = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE]).as_int();
        if (param_int < 0)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" buffer size can not be negative");
        size_t bs = param_int;

        param_int = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK]).as_int();
        if (param_int < 0)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" chunk size can not be negative");
        size_t cs = param_int;

        if (cs > bs)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" chunk size can not be greater than buffer size");

        param_int = sub.attribute(optionMap[ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL]).as_int();
        if (param_int < 0)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" fill size can not be negative");
        size_t fs = param_int;

        if (fs > bs)
            throw std::runtime_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" fill size can not be greater than buffer size");

        prof->setBufferOptions(bs, cs, fs);

        if (mtype_profile == nullptr) {
            throw std::runtime_error("error in configuration: transcoding "
                                     "profiles exist, but no mimetype to profile mappings specified");
        }

        bool set = false;
        for (const auto& mt_mapping : mt_mappings) {
            if (mt_mapping.second == prof->getName()) {
                list->add(mt_mapping.first, prof);
                set = true;
            }
        }

        if (!set)
            throw std::runtime_error("error in configuration: you specified a mimetype to transcoding profile mapping, "
                                     "but no match for profile \""
                + prof->getName() + "\" exists");

        set = false;
    }

    return list;
}

std::shared_ptr<AutoscanList> ConfigManager::createAutoscanListFromNode(const std::shared_ptr<Storage>& storage, const pugi::xml_node& element,
    ScanMode scanmode)
{
    auto list = std::make_shared<AutoscanList>(storage);

    if (element == nullptr)
        return list;

    for (const pugi::xml_node& child : element.children()) {

        // We only want directories
        if (std::string(child.name()) != optionMap[ATTR_AUTOSCAN_DIRECTORY])
            continue;

        fs::path location = child.attribute(optionMap[ATTR_AUTOSCAN_DIRECTORY_LOCATION]).as_string();
        if (location.empty()) {
            log_warning("Found an Autoscan directory with invalid location!");
            continue;
        }

        if (!fs::is_directory(location)) {
            log_warning("Autoscan path is not a directory: {}", location.string());
            continue;
        }

        std::string temp = child.attribute(optionMap[ATTR_AUTOSCAN_DIRECTORY_MODE]).as_string();
        if (temp.empty() || ((temp != "timed") && (temp != "inotify"))) {
            throw std::runtime_error("autoscan directory " + location.string() + ": mode attribute is missing or invalid");
        }

        ScanMode mode = (temp == "timed") ? ScanMode::Timed : ScanMode::INotify;
        if (mode != scanmode) {
            continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
        }

        unsigned int interval = 0;
        if (mode == ScanMode::Timed) {
            temp = child.attribute(optionMap[ATTR_AUTOSCAN_DIRECTORY_INTERVAL]).as_string();
            if (temp.empty()) {
                throw std::runtime_error("autoscan directory " + location.string() + ": interval attribute is required for timed mode");
            }

            interval = std::stoi(temp);
            if (interval == 0) {
                throw std::runtime_error("autoscan directory " + location.string() + ": invalid interval attribute");
            }
        }

        temp = child.attribute(optionMap[ATTR_AUTOSCAN_DIRECTORY_RECURSIVE]).as_string();
        if (temp.empty())
            throw std::runtime_error("autoscan directory " + location.string() + ": recursive attribute is missing or invalid");

        bool recursive;
        if (temp == "yes")
            recursive = true;
        else if (temp == "no")
            recursive = false;
        else {
            throw std::runtime_error("autoscan directory " + location.string() + ": recursive attribute " + temp + " is invalid");
        }

        bool hidden;
        temp = child.attribute(optionMap[ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES]).as_string();
        if (temp.empty())
            temp = getXmlOption(CFG_IMPORT_HIDDEN_FILES);

        if (temp == "yes")
            hidden = true;
        else if (temp == "no")
            hidden = false;
        else
            throw std::runtime_error("autoscan directory " + location.string() + ": hidden attribute " + temp + " is invalid");

        auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, true, INVALID_SCAN_ID, interval, hidden);
        try {
            list->add(dir);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Could not add " + location.string() + ": " + e.what());
        }
    }

    return list;
}

std::shared_ptr<ClientConfigList> ConfigManager::createClientConfigListFromNode(const pugi::xml_node& element)
{
    auto list = std::make_shared<ClientConfigList>();

    if (element == nullptr)
        return list;

    for (const pugi::xml_node& child : element.children()) {

        // We only want directories
        if (std::string(child.name()) != optionMap[ATTR_CLIENTS_CLIENT])
            continue;

        std::string flags = child.attribute(optionMap[ATTR_CLIENTS_CLIENT_FLAGS]).as_string();
        std::string ip = child.attribute(optionMap[ATTR_CLIENTS_CLIENT_IP]).as_string();
        std::string userAgent = child.attribute(optionMap[ATTR_CLIENTS_CLIENT_USERAGENT]).as_string();

        std::vector<std::string> flagsVector = split_string(flags, '|', false);
        int flag = std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](int flg, const auto& i) //
            { return flg | ClientConfig::remapFlag(i); });

        auto client = std::make_shared<ClientConfig>(flag, ip, userAgent);
        auto clientInfo = client->getClientInfo();
        Clients::addClientInfo(clientInfo);
        try {
            list->add(client);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Could not add " + ip + " client: " + e.what());
        }
    }

    return list;
}

void ConfigManager::dumpOptions()
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

std::vector<std::string> ConfigManager::createArrayFromNode(const pugi::xml_node& element, config_option_t nodeName, config_option_t attrName)
{
    std::vector<std::string> arr;

    if (element != nullptr) {
        for (const pugi::xml_node& child : element.children()) {
            if (std::string(child.name()) == optionMap[nodeName]) {
                std::string attrValue = child.attribute(optionMap[attrName]).as_string();
                if (!attrValue.empty())
                    arr.push_back(attrValue);
            }
        }
    }

    return arr;
}

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getOption();
}

int ConfigManager::getIntOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getIntOption();
}

bool ConfigManager::getBoolOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getBoolOption();
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(config_option_t option)
{
    return options->at(option)->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getArrayOption(config_option_t option)
{
    return options->at(option)->getArrayOption();
}

std::shared_ptr<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option)
{
    return options->at(option)->getAutoscanListOption();
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(config_option_t option)
{
    return options->at(option)->getClientConfigListOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option)
{
    return options->at(option)->getTranscodingProfileListOption();
}

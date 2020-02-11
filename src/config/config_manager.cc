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

#ifdef BSD_NATIVE_UUID
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
namespace fs = std::filesystem;

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <clocale>
#include <langinfo.h>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "config_manager.h"
#include "common.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

bool ConfigManager::debug_logging = false;

ConfigManager::ConfigManager(fs::path filename,
    fs::path userhome, fs::path config_dir,
    fs::path prefix_dir, fs::path magic_file,
    std::string ip, std::string interface, int port,
    bool debug_logging)
    :filename(filename)
    , prefix_dir(prefix_dir)
    , magic_file(magic_file)
    , ip(ip)
    , interface(interface)
    , port(port)
{
    this->debug_logging = debug_logging;

    xmlDoc = std::make_unique<pugi::xml_document>();
    options = std::make_unique<std::vector<std::shared_ptr<ConfigOption>>>();
    options->resize(CFG_MAX);

    if (filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userhome / config_dir;
        filename += home / DEFAULT_CONFIG_NAME;
    }

    if (!fs::is_regular_file(filename)) {
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

#define NEW_OPTION(optval) opt = std::make_shared<Option>(optval);
#define SET_OPTION(opttype) options->at(opttype) = opt;

#define NEW_INT_OPTION(optval) int_opt = std::make_shared<IntOption>(optval);
#define SET_INT_OPTION(opttype) options->at(opttype) = int_opt;

#define NEW_BOOL_OPTION(optval) bool_opt = std::make_shared<BoolOption>(optval);
#define SET_BOOL_OPTION(opttype) options->at(opttype) = bool_opt;

#define NEW_DICT_OPTION(optval) dict_opt = std::make_shared<DictionaryOption>(optval);
#define SET_DICT_OPTION(opttype) options->at(opttype) = dict_opt;

#define NEW_STRARR_OPTION(optval) str_array_opt = std::make_shared<StringArrayOption>(optval);
#define SET_STRARR_OPTION(opttype) options->at(opttype) = str_array_opt;

#define NEW_AUTOSCANLIST_OPTION(optval) alist_opt = std::make_shared<AutoscanListOption>(optval);
#define SET_AUTOSCANLIST_OPTION(opttype) options->at(opttype) = alist_opt;

#define NEW_TRANSCODING_PROFILELIST_OPTION(optval) trlist_opt = std::make_shared<TranscodingProfileListOption>(optval);
#define SET_TRANSCODING_PROFILELIST_OPTION(opttype) options->at(opttype) = trlist_opt;


void ConfigManager::load(fs::path filename, fs::path userHome)
{
    std::string temp;
    int temp_int;
    pugi::xml_node tmpEl;

    std::shared_ptr<Option> opt;
    std::shared_ptr<BoolOption> bool_opt;
    std::shared_ptr<IntOption> int_opt;
    std::shared_ptr<DictionaryOption> dict_opt;
    std::shared_ptr<StringArrayOption> str_array_opt;
    std::shared_ptr<AutoscanListOption> alist_opt;
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
        throw std::runtime_error("Config version \"" + version + "\" does not yet exist!");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server

    if (!userHome.empty()) {
        // respect command line; ignore xml value
        temp = userHome;
    } else
        temp = getOption("/server/home");
    if (!fs::is_directory(temp))
        throw std::runtime_error("Directory '" + temp + "' does not exist!");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_HOME);

    temp = getOption("/server/webroot");
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_WEBROOT);

    temp = getOption("/server/tmpdir", DEFAULT_TMPDIR);
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_TMPDIR);

    temp = getOption("/server/servedir", "");
    temp = resolvePath(temp);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    NEW_OPTION(getOption("/server/udn"));
    SET_OPTION(CFG_SERVER_UDN);

    // checking database driver options
    std::string mysql_en = "no";
    std::string sqlite3_en = "no";

    tmpEl = getElement("/server/storage");
    if (tmpEl == nullptr)
        throw std::runtime_error("Error in config file: <storage> tag not found");

    tmpEl = getElement("/server/storage/mysql");
    if (tmpEl != nullptr) {
        mysql_en = getOption("/server/storage/mysql/attribute::enabled",
            DEFAULT_MYSQL_ENABLED);
        if (!validateYesNo(mysql_en))
            throw std::runtime_error("Invalid <mysql enabled=\"\"> value");
    }

    tmpEl = getElement("/server/storage/sqlite3");
    if (tmpEl != nullptr) {
        sqlite3_en = getOption("/server/storage/sqlite3/attribute::enabled",
            DEFAULT_SQLITE_ENABLED);
        if (!validateYesNo(sqlite3_en))
            throw std::runtime_error("Invalid <sqlite3 enabled=\"\"> value");
    }

    if ((sqlite3_en == "yes") && (mysql_en == "yes"))
        throw std::runtime_error("You enabled both, sqlite3 and mysql but "
                         "only one database driver may be active at "
                         "a time!");

    if ((sqlite3_en == "no") && (mysql_en == "no"))
        throw std::runtime_error("You disabled both, sqlite3 and mysql but "
                         "one database driver must be active!");

#ifdef HAVE_MYSQL
    if (mysql_en == "yes") {
        NEW_OPTION(getOption("/server/storage/mysql/host",
            DEFAULT_MYSQL_HOST));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_HOST);

        NEW_OPTION(getOption("/server/storage/mysql/database",
            DEFAULT_MYSQL_DB));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_DATABASE);

        NEW_OPTION(getOption("/server/storage/mysql/username",
            DEFAULT_MYSQL_USER));
        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_USERNAME);

        NEW_INT_OPTION(getIntOption("/server/storage/mysql/port", 0));
        SET_INT_OPTION(CFG_SERVER_STORAGE_MYSQL_PORT);

        if (getElement("/server/storage/mysql/socket") == nullptr) {
            NEW_OPTION(nullptr);
        } else {
            NEW_OPTION(getOption("/server/storage/mysql/socket"));
        }

        SET_OPTION(CFG_SERVER_STORAGE_MYSQL_SOCKET);

        if (getElement("/server/storage/mysql/password") == nullptr) {
            NEW_OPTION(nullptr);
        } else {
            NEW_OPTION(getOption("/server/storage/mysql/password"));
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

#ifdef HAVE_SQLITE3

    if (sqlite3_en == "yes") {
        temp = getOption("/server/storage/sqlite3/database-file");
        temp = resolvePath(temp, true, false);
        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

        temp = getOption("/server/storage/sqlite3/synchronous",
            DEFAULT_SQLITE_SYNC);

        temp_int = 0;

        if (temp == "off")
            temp_int = MT_SQLITE_SYNC_OFF;
        else if (temp == "normal")
            temp_int = MT_SQLITE_SYNC_NORMAL;
        else if (temp == "full")
            temp_int = MT_SQLITE_SYNC_FULL;
        else
            throw std::runtime_error("Invalid <synchronous> value in sqlite3 "
                             "section");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);

        temp = getOption("/server/storage/sqlite3/on-error",
            DEFAULT_SQLITE_RESTORE);

        bool tmp_bool = true;

        if (temp == "restore")
            tmp_bool = true;
        else if (temp == "fail")
            tmp_bool = false;
        else
            throw std::runtime_error("Invalid <on-error> value in sqlite3 "
                             "section");

        NEW_BOOL_OPTION(tmp_bool);
        SET_BOOL_OPTION(CFG_SERVER_STORAGE_SQLITE_RESTORE);
#ifdef SQLITE_BACKUP_ENABLED
        temp = getOption("/server/storage/sqlite3/backup/attribute::enabled",
            YES);
#else
        temp = getOption("/server/storage/sqlite3/backup/attribute::enabled",
            DEFAULT_SQLITE_BACKUP_ENABLED);
#endif
        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: incorrect parameter "
                             "for <backup enabled=\"\" /> attribute");
        NEW_BOOL_OPTION(temp == "yes");
        SET_BOOL_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);

        temp_int = getIntOption("/server/storage/sqlite3/backup/attribute::interval",
            DEFAULT_SQLITE_BACKUP_INTERVAL);
        if (temp_int < 1)
            throw std::runtime_error("Error in config file: incorrect parameter for "
                             "<backup interval=\"\" /> attribute");
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
    }
#else
    if (sqlite3_en == "yes") {
        throw std::runtime_error("You enabled sqlite3 storage in configuration, "
                         "however this version of Gerbera was compiled "
                         "without sqlite3 support!");
    }

#endif // SQLITE3

    std::string dbDriver;
    if (sqlite3_en == "yes")
        dbDriver = "sqlite3";

    if (mysql_en == "yes")
        dbDriver = "mysql";

    NEW_OPTION(dbDriver);
    SET_OPTION(CFG_SERVER_STORAGE_DRIVER);


    // now go through the optional settings and fix them if anything is missing

    temp = getOption("/server/ui/attribute::enabled",
        DEFAULT_UI_EN_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                         "for <ui enabled=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_ENABLED);

    temp = getOption("/server/ui/attribute::show-tooltips",
        DEFAULT_UI_SHOW_TOOLTIPS_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                         "for <ui show-tooltips=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_SHOW_TOOLTIPS);

    temp = getOption("/server/ui/attribute::poll-when-idle",
        DEFAULT_POLL_WHEN_IDLE_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                         "for <ui poll-when-idle=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_POLL_WHEN_IDLE);

    temp_int = getIntOption("/server/ui/attribute::poll-interval",
        DEFAULT_POLL_INTERVAL);
    if (temp_int < 1)
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<ui poll-interval=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_POLL_INTERVAL);

    temp_int = getIntOption("/server/ui/items-per-page/attribute::default",
        DEFAULT_ITEMS_PER_PAGE_2);
    if (temp_int < 1)
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<items-per-page default=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

    // now get the option list for the drop down menu
    tmpEl = getElement("/server/ui/items-per-page");
    // create default structure
    if (std::distance(tmpEl.begin(), tmpEl.end()) == 0) {
        if ((temp_int != DEFAULT_ITEMS_PER_PAGE_1) && (temp_int != DEFAULT_ITEMS_PER_PAGE_2) &&
            (temp_int != DEFAULT_ITEMS_PER_PAGE_3) && (temp_int != DEFAULT_ITEMS_PER_PAGE_4)) {
            throw std::runtime_error("Error in config file: you specified an "
                             "<items-per-page default=\"\"> value that is "
                             "not listed in the options");
        }

        tmpEl.append_child("option").append_child(pugi::node_pcdata)
            .set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_1).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata)
            .set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_2).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata)
            .set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_3).c_str());
        tmpEl.append_child("option").append_child(pugi::node_pcdata)
            .set_value(std::to_string(DEFAULT_ITEMS_PER_PAGE_4).c_str());
    } else // validate user settings
    {
        bool default_found = false;
        for (pugi::xml_node child: tmpEl.children()) {
            if (std::string(child.name()) == "option") {
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
    for (pugi::xml_node child: tmpEl.children()) {
        if (std::string(child.name()) == "option")
            menu_opts.emplace_back(child.text().as_string());
    }
    NEW_STRARR_OPTION(menu_opts);
    SET_STRARR_OPTION(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);

    temp = getOption("/server/ui/accounts/attribute::enabled",
        DEFAULT_ACCOUNTS_EN_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<accounts enabled=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_UI_ACCOUNTS_ENABLED);

    tmpEl = getElement("/server/ui/accounts");
    NEW_DICT_OPTION(createDictionaryFromNode(tmpEl, "account", "user", "password"));
    SET_DICT_OPTION(CFG_SERVER_UI_ACCOUNT_LIST);

    temp_int = getIntOption("/server/ui/accounts/attribute::session-timeout",
        DEFAULT_SESSION_TIMEOUT);
    if (temp_int < 1) {
        throw std::runtime_error("Error in config file: invalid session-timeout "
                         "(must be > 0)\n");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_SESSION_TIMEOUT);

    temp = getOption("/import/attribute::hidden-files",
        DEFAULT_HIDDEN_FILES_VALUE);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<import hidden-files=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_HIDDEN_FILES);

    temp = getOption(
        "/import/mappings/extension-mimetype/attribute::ignore-unknown",
        DEFAULT_IGNORE_UNKNOWN_EXTENSIONS);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<extension-mimetype ignore-unknown=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    temp = getOption(
        "/import/mappings/extension-mimetype/attribute::case-sensitive",
        DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "<extension-mimetype case-sensitive=\"\" /> attribute");

    bool csens = false;

    if (temp == "yes")
        csens = true;

    NEW_BOOL_OPTION(csens);
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    tmpEl = getElement("/import/mappings/extension-mimetype");
    NEW_DICT_OPTION(createDictionaryFromNode(tmpEl, "map", "from", "to", !csens));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    std::map<std::string,std::string> mime_content;
    tmpEl = getElement("/import/mappings/mimetype-contenttype");
    if (tmpEl != nullptr) {
        mime_content = createDictionaryFromNode(tmpEl, "treat", "mimetype", "as");
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

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "") != nullptr) {
        temp = nl_langinfo(CODESET);
        log_debug("received {} from nl_langinfo", temp.c_str());
    }

    if (!string_ok(temp))
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
    std::string charset = getOption("/import/filesystem-charset", temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported "
                         "filesystem-charset specified: "
            + charset);
    }

    log_info("Setting filesystem import charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_FILESYSTEM_CHARSET);

    charset = getOption("/import/metadata-charset", temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported "
                         "metadata-charset specified: "
            + charset);
    }

    log_info("Setting metadata import charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_METADATA_CHARSET);

    charset = getOption("/import/playlist-charset", temp);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported playlist-charset specified: " + charset);
    }

    log_info("Setting playlist charset to {}", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_PLAYLIST_CHARSET);

    temp = getOption("/server/protocolInfo/attribute::extend",
        DEFAULT_EXTEND_PROTOCOLINFO);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: extend attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO);

    /*
    temp = getOption("/server/protocolInfo/attribute::ps3-hack",
                     DEFAULT_EXTEND_PROTOCOLINFO_CL_HACK);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: ps3-hack attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_CL_HACK);
*/
    temp = getOption("/server/protocolInfo/attribute::samsung-hack",
        DEFAULT_EXTEND_PROTOCOLINFO_SM_HACK);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: samsung-hack attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK);

    temp = getOption("/server/protocolInfo/attribute::dlna-seek",
        DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: dlna-seek attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK);

    temp = getOption("/server/pc-directory/attribute::upnp-hide",
        DEFAULT_HIDE_PC_DIRECTORY);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: hide attribute of the "
                         "pc-directory tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_SERVER_HIDE_PC_DIRECTORY);

    if (!string_ok(interface)) {
        temp = getOption("/server/interface", "");
    } else {
        temp = interface;
    }
    if (string_ok(temp) && string_ok(getOption("/server/ip", "")))
        throw std::runtime_error("Error in config file: you can not specify interface and ip at the same time!");

    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_NETWORK_INTERFACE);

    if (!string_ok(ip)) {
        temp = getOption("/server/ip", ""); // bind to any IP address
    } else {
        temp = ip;
    }
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_IP);

    temp = getOption("/server/bookmark", DEFAULT_BOOKMARK_FILE);
    temp = resolvePath(temp, true, false);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_BOOKMARK_FILE);

    temp = getOption("/server/name", DESC_FRIENDLY_NAME);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_NAME);

    temp = getOption("/server/modelName", DESC_MODEL_NAME);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_NAME);

    temp = getOption("/server/modelDescription", DESC_MODEL_DESCRIPTION);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_DESCRIPTION);

    temp = getOption("/server/modelNumber", DESC_MODEL_NUMBER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_NUMBER);

    temp = getOption("/server/modelURL", "");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MODEL_URL);

    temp = getOption("/server/serialNumber", DESC_SERIAL_NUMBER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_SERIAL_NUMBER);

    temp = getOption("/server/manufacturer", DESC_MANUFACTURER);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MANUFACTURER);

    temp = getOption("/server/manufacturerURL", DESC_MANUFACTURER_URL);
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_MANUFACTURER_URL);

    temp = getOption("/server/presentationURL", "");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_PRESENTATION_URL);

    temp = getOption("/server/presentationURL/attribute::append-to",
        DEFAULT_PRES_URL_APPENDTO_ATTR);

    if ((temp != "none") && (temp != "ip") && (temp != "port")) {
        throw std::runtime_error("Error in config file: "
                         "invalid \"append-to\" attribute value in "
                         "<presentationURL> tag");
    }

    if (((temp == "ip") || (temp == "port")) && !string_ok(getOption("/server/presentationURL"))) {
        throw std::runtime_error("Error in config file: \"append-to\" attribute "
                         "value in <presentationURL> tag is set to \""
            + temp + "\" but no URL is specified");
    }
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_APPEND_PRESENTATION_URL_TO);

    temp_int = getIntOption("/server/upnp-string-limit",
        DEFAULT_UPNP_STRING_LIMIT);
    if ((temp_int != -1) && (temp_int < 4)) {
        throw std::runtime_error("Error in config file: invalid value for "
                         "<upnp-string-limit>");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

#ifdef HAVE_JS
    temp = getOption("/import/scripting/playlist-script",
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT);
    temp = resolvePath(temp, true);
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);

    temp = getOption("/import/scripting/common-script",
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT);
    temp = resolvePath(temp, true);
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);

    temp = getOption(
        "/import/scripting/playlist-script/attribute::create-link",
        DEFAULT_PLAYLIST_CREATE_LINK);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"create-link\" attribute value in "
                         "<playlist-script> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
#endif

    temp = getOption("/import/scripting/virtual-layout/attribute::type",
        DEFAULT_LAYOUT_TYPE);
    if ((temp != "js") && (temp != "builtin") && (temp != "disabled"))
        throw std::runtime_error("Error in config file: invalid virtual layout "
                         "type specified!");
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);

#ifndef HAVE_JS
    if (temp == "js")
        throw std::runtime_error("Gerbera was compiled without JS support, "
                         "however you specified \"js\" to be used for the "
                         "virtual-layout.");
#else
    charset = getOption("/import/scripting/attribute::script-charset",
        DEFAULT_JS_CHARSET);
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

    std::string script_path = getOption(
        "/import/scripting/virtual-layout/import-script",
        prefix_dir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    temp = resolvePath(temp, true, temp == "js");
    if (temp == "js" && script_path.empty())
        throw std::runtime_error("Error in config file: you specified \"js\" to "
                                "be used for virtual layout, but script "
                                "location is invalid.");

    NEW_OPTION(script_path);
    SET_OPTION(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
#endif

    // 0 means, that the SDK will any free port itself
    if (port <= 0) {
        temp_int = getIntOption("/server/port", 0);
    } else {
        temp_int = port;
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_PORT);

    temp_int = getIntOption("/server/alive", DEFAULT_ALIVE_INTERVAL);
    if (temp_int < ALIVE_INTERVAL_MIN)
        throw std::runtime_error(fmt::format("Error in config file: incorrect parameter for /server/alive, must be at least {}", ALIVE_INTERVAL_MIN));
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_ALIVE_INTERVAL);

    pugi::xml_node el = getElement("/import/mappings/mimetype-upnpclass");
    if (el == nullptr) {
        getOption("/import/mappings/mimetype-upnpclass", "");
    }
    NEW_DICT_OPTION(createDictionaryFromNode(el, "map", "from", "to"));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    temp = getOption("/import/autoscan/attribute::use-inotify", "auto");
    if ((temp != "auto") && !validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter for "
                         "\"<autoscan use-inotify=\" attribute");

    el = getElement("/import/autoscan");

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
                             " however your system does not have "
                             "inotify support");
#else
        throw std::runtime_error("You specified"
                         " \"yes\" in \"<autoscan use-inotify=\"\">"
                         " however this version of Gerbera was compiled "
                         "without inotify support");
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

    temp = getOption(
        "/transcoding/attribute::enabled",
        DEFAULT_TRANSCODING_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: incorrect parameter "
                         "for <transcoding enabled=\"\"> attribute");

    if (temp == "yes")
        el = getElement("/transcoding");
    else
        el = pugi::xml_node(nullptr);
    NEW_TRANSCODING_PROFILELIST_OPTION(createTranscodingProfileListFromNode(el));
    SET_TRANSCODING_PROFILELIST_OPTION(CFG_TRANSCODING_PROFILE_LIST);

#ifdef HAVE_CURL
    if (temp == "yes") {
        temp_int = getIntOption(
            "/transcoding/attribute::fetch-buffer-size",
            DEFAULT_CURL_BUFFER_SIZE);
        if (temp_int < CURL_MAX_WRITE_SIZE)
            throw std::runtime_error(fmt::format("Error in config file: incorrect parameter "
                             "for <transcoding fetch-buffer-size=\"\"> attribute, "
                             "must be at least {}", CURL_MAX_WRITE_SIZE));
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);

        temp_int = getIntOption(
            "/transcoding/attribute::fetch-buffer-fill-size",
            DEFAULT_CURL_INITIAL_FILL_SIZE);
        if (temp_int < 0)
            throw std::runtime_error("Error in config file: incorrect parameter "
                             "for <transcoding fetch-buffer-fill-size=\"\"> attribute");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }

#endif //HAVE_CURL

    el = getElement("/server/custom-http-headers");
    NEW_STRARR_OPTION(createArrayFromNode(el, "add", "header"));
    SET_STRARR_OPTION(CFG_SERVER_CUSTOM_HTTP_HEADERS);

#ifdef HAVE_LIBEXIF

    el = getElement("/import/library-options/libexif/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/libexif/auxdata",
            "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);

#endif // HAVE_LIBEXIF

#ifdef HAVE_EXIV2

    el = getElement("/import/library-options/exiv2/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/exiv2/auxdata",
            "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);

#endif // HAVE_EXIV2

#if defined(HAVE_TAGLIB)
    el = getElement("/import/library-options/id3/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/id3/auxdata", "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG)
    el = getElement("/import/library-options/ffmpeg/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/ffmpeg/auxdata", "");
    }
    NEW_STRARR_OPTION(createArrayFromNode(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                     "attribute::enabled",
        DEFAULT_FFMPEGTHUMBNAILER_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<ffmpegthumbnailer> tag");

    NEW_BOOL_OPTION(temp == YES ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED);

    if (temp == YES) {
        temp_int = getIntOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                                "thumbnail-size",
            DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE);

        if (temp_int <= 0)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                             "invalid value attribute value in "
                             "<thumbnail-size> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);

        temp_int = getIntOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                                "seek-percentage",
            DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        if (temp_int < 0)
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                             "invalid value attribute value in "
                             "<seek-percentage> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "filmstrip-overlay",
            DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                             "invalid value in <filmstrip-overlay> tag");

        NEW_BOOL_OPTION(temp == YES ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "workaround-bugs",
            DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: ffmpegthumbnailer - "
                             "invalid value in <workaround-bugs> tag");

        NEW_BOOL_OPTION(temp == YES ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        temp_int = getIntOption("/server/extended-runtime-options/"
                                "ffmpegthumbnailer/image-quality",
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

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "cache-dir",
            DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR);

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "cache-dir/attribute::enabled",
            DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);

        if (!validateYesNo(temp))
            throw std::runtime_error("Error in config file: "
                             "invalid \"enabled\" attribute value in "
                             "ffmpegthumbnailer <cache-dir> tag");

        NEW_BOOL_OPTION(temp == YES ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
    }
#endif

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "attribute::enabled",
        DEFAULT_MARK_PLAYED_ITEMS_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<mark-played-items> tag");

    bool markingEnabled = temp == YES;
    NEW_BOOL_OPTION(markingEnabled);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "attribute::suppress-cds-updates",
        DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \":suppress-cds-updates\" attribute "
                         "value in <mark-played-items> tag");

    NEW_BOOL_OPTION(temp == YES);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "string/attribute::mode",
        DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);

    if ((temp != "prepend") && (temp != "append"))
        throw std::runtime_error("Error in config file: "
                         "invalid \"mode\" attribute value in "
                         "<string> tag in the <mark-played-items> section");

    NEW_BOOL_OPTION(temp == DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "string",
        DEFAULT_MARK_PLAYED_ITEMS_STRING);
    if (!string_ok(temp))
        throw std::runtime_error("Error in config file: "
                         "empty string given for the <string> tag in the "
                         "<mark-played-items> section");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

    std::vector<std::string> mark_content_list;
    tmpEl = getElement("/server/extended-runtime-options/mark-played-items/mark");

    int contentElementCount = 0;
    if (tmpEl != nullptr) {
        for (pugi::xml_node content: tmpEl.children()) {
            if (std::string(content.name()) != "content")
                continue;

            contentElementCount++;

            std::string mark_content = content.text().as_string();
            if (!string_ok(mark_content))
                throw std::runtime_error("error in configuration, <mark-played-items>, empty <content> parameter!");

            if ((mark_content != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_IMAGE))
                throw std::runtime_error(R"(error in configuration, <mark-played-items>, invalid <content> parameter! Allowed values are "video", "audio", "image")");

            mark_content_list.push_back(mark_content);
            NEW_STRARR_OPTION(mark_content_list);
            SET_STRARR_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        }
    }

    if (markingEnabled && contentElementCount == 0) {
        throw std::runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag!");
    }

#if defined(HAVE_LASTFMLIB)
    temp = getOption("/server/extended-runtime-options/lastfm/attribute::enabled", DEFAULT_LASTFM_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<lastfm> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_LASTFM_ENABLED);

    if (temp == YES) {
        temp = getOption("/server/extended-runtime-options/lastfm/username",
            DEFAULT_LASTFM_USERNAME);

        if (!string_ok(temp))
            throw std::runtime_error("Error in config file: lastfm - "
                             "invalid username value in "
                             "<username> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_USERNAME);

        temp = getOption("/server/extended-runtime-options/lastfm/password",
            DEFAULT_LASTFM_PASSWORD);

        if (!string_ok(temp))
            throw std::runtime_error("Error in config file: lastfm - "
                             "invalid password value in "
                             "<password> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

#ifdef HAVE_MAGIC
    if (!magic_file.empty()) {
        // respect command line; ignore xml value
        magic_file = resolvePath(magic_file, true);
    } else {
        magic_file = getOption("/import/magic-file", "");
        if (!magic_file.empty())
            magic_file = resolvePath(magic_file, true);
    }
    NEW_OPTION(magic_file);
    SET_OPTION(CFG_IMPORT_MAGIC_FILE);
#endif

#ifdef HAVE_INOTIFY
    tmpEl = getElement("/import/autoscan");
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
    temp = getOption("/import/online-content/SopCast/attribute::enabled",
        DEFAULT_SOPCAST_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_ENABLED);

    int sopcast_refresh = getIntOption("/import/online-content/SopCast/attribute::refresh", 0);
    NEW_INT_OPTION(sopcast_refresh);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);

    temp_int = getIntOption("/import/online-content/SopCast/attribute::purge-after", 0);
    if (sopcast_refresh >= temp_int) {
        if (temp_int != 0)
            throw std::runtime_error("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }

    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);

    temp = getOption("/import/online-content/SopCast/attribute::update-at-start",
        DEFAULT_SOPCAST_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"update-at-start\" attribute value in "
                         "<SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START);
#endif

#ifdef ATRAILERS
    temp = getOption("/import/online-content/AppleTrailers/attribute::enabled",
        DEFAULT_ATRAILERS_ENABLED);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);

    temp_int = getIntOption("/import/online-content/AppleTrailers/attribute::refresh", DEFAULT_ATRAILERS_REFRESH);
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);

    temp = getOption("/import/online-content/AppleTrailers/attribute::update-at-start",
        DEFAULT_ATRAILERS_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw std::runtime_error("Error in config file: "
                         "invalid \"update-at-start\" attribute value in "
                         "<AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes");
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);

    temp = getOption("/import/online-content/AppleTrailers/attribute::resolution",
        std::to_string(DEFAULT_ATRAILERS_RESOLUTION));
    if ((temp != "640") && (temp != "720p")) {
        throw std::runtime_error("Error in config file: "
                         "invalid \"resolution\" attribute value in "
                         "<AppleTrailers> tag, only \"640\" and \"720p\" is "
                         "supported");
    }

    NEW_OPTION(temp);
    SET_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str().c_str());
}

std::string ConfigManager::getOption(std::string xpath, std::string def)
{
    auto root = xmlDoc->document_element();
    xpath = "/config" + xpath;
    pugi::xpath_node xpathNode = root.select_node(xpath.c_str());

    if (xpathNode.node() != nullptr) {
        return trim_string(xpathNode.node().text().as_string());
    }

    if (xpathNode.attribute() != nullptr) {
        return trim_string(xpathNode.attribute().value());
    }

    log_debug("Config: option not found: '{}' using default value: '{}'",
        xpath.c_str(), def.c_str());

    return def;
}

int ConfigManager::getIntOption(std::string xpath, int def)
{
    std::string sDef;
    sDef = std::to_string(def);
    std::string sVal = getOption(xpath, sDef);
    return std::stoi(sVal);
}

std::string ConfigManager::getOption(std::string xpath)
{
    xpath = "/config" + xpath;
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

int ConfigManager::getIntOption(std::string xpath)
{
    std::string sVal = getOption(xpath);
    return std::stoi(sVal);
}

pugi::xml_node ConfigManager::getElement(std::string xpath)
{
    xpath = "/config" + xpath;
    auto root = xmlDoc->document_element();
    pugi::xpath_node xpathNode = root.select_node(xpath.c_str());
    return xpathNode.node();
}

fs::path ConfigManager::resolvePath(fs::path path, bool isFile, bool exists)
{
    fs::path home = getOption(CFG_SERVER_HOME);

    if (path.is_absolute() || (home.is_relative() && path.is_relative()))
        ; // absolute or relative, nothing to resolve
    else if (home.empty())
        path = "." / path;
    else
        path = home / path;

    // verify that file/directory is there
    if (isFile) {
        if (exists) {
            if (!fs::is_regular_file(path))
                throw std::runtime_error("File '" + path.string() + "' does not exist!");
        } else {
            std::string parent_path = path.parent_path();
            if (!fs::is_directory(parent_path))
                throw std::runtime_error("Parent directory '" + path.string() + "' does not exist!");
        }
    } else if (exists) {
        if (!fs::is_directory(path))
            throw std::runtime_error("Directory '" + path.string() + "' does not exist!");
    }

    return path;
}

void ConfigManager::writeBookmark(std::string ip, std::string port)
{
    std::string data;
    if (!getBoolOption(CFG_SERVER_UI_ENABLED)) {
        data = http_redirect_to(ip, port, "disabled.html");
    } else {
        data = http_redirect_to(ip, port);
    }

    fs::path path = getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Writing bookmark file to: {}", path.c_str());
    writeTextFile(path, data);
}

void ConfigManager::emptyBookmark()
{
    std::string data = "<html><body><h1>Gerbera Media Server is not running.</h1><p>Please start it and try again.</p></body></html>";

    fs::path path = getOption(CFG_SERVER_BOOKMARK_FILE);
    log_debug("Clearing bookmark file at: {}", path.c_str());
    writeTextFile(path, data);;
}

std::map<std::string,std::string> ConfigManager::createDictionaryFromNode(const pugi::xml_node& element,
    std::string nodeName, std::string keyAttr, std::string valAttr, bool tolower)
{
    std::map<std::string,std::string> dict;

    if (element != nullptr) {
        for (pugi::xml_node child: element.children()) {
            if (child.name() == nodeName) {
                std::string key = child.attribute(keyAttr.c_str()).as_string();
                std::string value = child.attribute(valAttr.c_str()).as_string();

                if (string_ok(key) && string_ok(value)) {
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

    std::map<std::string,std::string> mt_mappings;

    auto mtype_profile = element.child("mimetype-profile-mappings");
    if (mtype_profile != nullptr) {
        for (pugi::xml_node child: element.children()) {
            if (std::string(child.name()) == "transcode") {
                std::string mt = child.attribute("mimetype").as_string();
                std::string pname = child.attribute("using").as_string();

                if (string_ok(mt) && string_ok(pname)) {
                    mt_mappings[mt] = pname;
                } else {
                    throw std::runtime_error("error in configuration: invalid or missing mimetype to profile mapping");
                }
            }
        }
    }

    auto profiles = element.child("profiles");
    if (profiles == nullptr)
        return list;

    for (pugi::xml_node child: element.children()) {
        if (std::string(child.name()) != "profile")
            continue;

        param = child.attribute("enabled").as_string();
        if (!validateYesNo(param))
            throw std::runtime_error("Error in config file: incorrect parameter "
                             "for <profile enabled=\"\" /> attribute");

        if (param == "no")
            continue;

        param = child.attribute("type").as_string();
        if (!string_ok(param))
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

        param = child.attribute("name").as_string();
        if (!string_ok(param))
            throw std::runtime_error("error in configuration: invalid transcoding profile name");

        auto prof = std::make_shared<TranscodingProfile>(tr_type, param);

        pugi::xml_node sub;
        sub = child.child("mimetype");
        param = sub.text().as_string();
        if (!string_ok(param))
            throw std::runtime_error("error in configuration: invalid target mimetype in transcoding profile");
        prof->setTargetMimeType(param);

        sub = child.child("resolution");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (string_ok(param)) {
                if (check_resolution(param))
                    prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
            }
        }

        sub = child.child("avi-fourcc-list");
        if (sub != nullptr) {
            std::string mode = sub.attribute("mode").as_string();
            if (!string_ok(mode))
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
                for (pugi::xml_node fourcc: sub.children()) {
                    if (std::string(fourcc.name()) != "fourcc")
                        continue;

                    std::string fcc = fourcc.text().as_string();
                    if (!string_ok(fcc))
                        throw std::runtime_error("error in configuration: empty fourcc specified!");
                    fcc_list.push_back(fcc);
                }

                prof->setAVIFourCCList(fcc_list, fcc_mode);
            }
        }

        sub = child.child("accept-url");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <accept-url> tag");
            if (param == "yes")
                prof->setAcceptURL(true);
            else
                prof->setAcceptURL(false);
        }

        sub = child.child("sample-frequency");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (param == "source")
                prof->setSampleFreq(SOURCE);
            else if (param == "off")
                prof->setSampleFreq(OFF);
            else {
                int freq = std::stoi(param);
                if (freq <= 0)
                    throw std::runtime_error("Error in config file: incorrect "
                                     "parameter for <sample-frequency> "
                                     "tag");

                prof->setSampleFreq(freq);
            }
        }

        sub = child.child("audio-channels");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (param == "source")
                prof->setNumChannels(SOURCE);
            else if (param == "off")
                prof->setNumChannels(OFF);
            else {
                int chan = std::stoi(param);
                if (chan <= 0)
                    throw std::runtime_error("Error in config file: incorrect "
                                     "parameter for <number-of-channels> "
                                     "tag");
                prof->setNumChannels(chan);
            }
        }

        sub = child.child("hide-original-resource");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <hide-original-resource> tag");
            if (param == "yes")
                prof->setHideOriginalResource(true);
            else
                prof->setHideOriginalResource(false);
        }

        sub = child.child("thumbnail");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <thumbnail> tag");
            if (param == "yes")
                prof->setThumbnail(true);
            else
                prof->setThumbnail(false);
        }

        sub = child.child("first-resource");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for <profile first-resource=\"\" /> attribute");

            if (param == "yes")
                prof->setFirstResource(true);
            else
                prof->setFirstResource(false);
        }

        sub = child.child("use-chunked-encoding");
        if (sub != nullptr) {
            param = sub.text().as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                 "for use-chunked-encoding tag");

            if (param == "yes")
                prof->setChunked(true);
            else
                prof->setChunked(false);
        }

        sub = child.child("agent");
        if (sub == nullptr)
            throw std::runtime_error("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" is missing the <agent> option");
        else {
            param = sub.attribute("command").as_string();
            if (!string_ok(param))
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" has an invalid command setting");
            prof->setCommand(param);

            std::string tmp_path;
            if (fs::path(param).is_absolute()) {
                if (!fs::is_regular_file(param))
                    throw std::runtime_error("error in configuration, transcoding "
                                    "profile \""
                        + prof->getName() + "\" could not find transcoding command " + param);
                tmp_path = param;
            } else {
                tmp_path = find_in_path(param);
                if (!string_ok(tmp_path))
                    throw std::runtime_error("error in configuration, transcoding "
                                    "profile \""
                        + prof->getName() + "\" could not find transcoding command " + param + " in $PATH");
            }

            int err = 0;
            if (!is_executable(tmp_path, &err))
                throw std::runtime_error("error in configuration, transcoding "
                                "profile "
                    + prof->getName() + ": transcoder " + param + "is not executable - " + strerror(err));

            param = sub.attribute("arguments").as_string();
            if (!string_ok(param))
                throw std::runtime_error("error in configuration: transcoding profile " + prof->getName() + " has an empty argument string");

            prof->setArguments(param);
        }

        sub = child.child("buffer");
        if (sub == nullptr)
            throw std::runtime_error("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" is missing the <buffer> option");
        else {
            param_int = sub.attribute("size").as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" buffer size can not be negative");
            size_t bs = param_int;

            param_int = sub.attribute("chunk-size").as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" chunk size can not be negative");
            size_t cs = param_int;

            if (cs > bs)
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" chunk size can not be greater than "
                                        "buffer size");

            param_int = sub.attribute("fill-size").as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" fill size can not be negative");
            size_t fs = param_int;

            if (fs > bs)
                throw std::runtime_error("error in configuration: transcoding "
                                "profile \""
                    + prof->getName() + "\" fill size can not be greater than "
                                        "buffer size");

            prof->setBufferOptions(bs, cs, fs);

            if (mtype_profile == nullptr) {
                throw std::runtime_error("error in configuration: transcoding "
                                "profiles exist, but no mimetype to profile "
                                "mappings specified");
            }

            bool set = false;
            for (const auto& mt_mapping : mt_mappings) {
                if (mt_mapping.second == prof->getName()) {
                    list->add(mt_mapping.first, prof);
                    set = true;
                }
            }

            if (!set)
                throw std::runtime_error("error in configuration: you specified"
                                "a mimetype to transcoding profile mapping, "
                                "but no match for profile \""
                    + prof->getName() + "\" exists");
            else
                set = false;
        }
    }

    return list;
}

std::shared_ptr<AutoscanList> ConfigManager::createAutoscanListFromNode(std::shared_ptr<Storage> storage, const pugi::xml_node& element,
    ScanMode scanmode)
{
    auto list = std::make_shared<AutoscanList>(storage);

    if (element == nullptr)
        return list;

    for (pugi::xml_node child: element.children()) {

        // We only want directories
        if (std::string(child.name()) != "directory")
            continue;

        fs::path location = child.attribute("location").as_string();
        if (!string_ok(location)) {
            log_warning("Found an Autoscan directory with invalid location!");
            continue;
        }

        if (!fs::is_directory(location)) {
            log_warning("Autoscan path is not a directory: {}", location.string());
            continue;
        }

        ScanMode mode;
        std::string temp = child.attribute("mode").as_string();
        if (!string_ok(temp) || ((temp != "timed") && (temp != "inotify"))) {
            throw std::runtime_error("autoscan directory " + location.string() + ": mode attribute is missing or invalid");
        } else if (temp == "timed") {
            mode = ScanMode::Timed;
        } else {
            mode = ScanMode::INotify;
        }

        if (mode != scanmode) {
            continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
        }

        unsigned int interval = 0;
        ScanLevel level;

        if (mode == ScanMode::Timed) {
            temp = child.attribute("level").as_string();
            if (!string_ok(temp)) {
                throw std::runtime_error("autoscan directory " + location.string() + ": level attribute is missing or invalid");
            } else {
                if (temp == "basic")
                    level = ScanLevel::Basic;
                else if (temp == "full")
                    level = ScanLevel::Full;
                else {
                    throw std::runtime_error("autoscan directory " + location.string() + ": level attribute " + temp + "is invalid");
                }
            }

            temp = child.attribute("interval").as_string();
            if (!string_ok(temp)) {
                throw std::runtime_error("autoscan directory " + location.string() + ": interval attribute is required for timed mode");
            }

            interval = std::stoi(temp);

            if (interval == 0) {
                throw std::runtime_error("autoscan directory " + location.string() + ": invalid interval attribute");
                continue;
            }
        } else {
            // level is irrelevant for inotify scan, nevertheless we will set
            // it to something valid
            level = ScanLevel::Full;
        }

        temp = child.attribute("recursive").as_string();
        if (!string_ok(temp))
            throw std::runtime_error("autoscan directory " + location.string() + ": recursive attribute is missing or invalid");

        bool recursive;
        if (temp == "yes")
            recursive = true;
        else if (temp == "no")
            recursive = false;
        else {
            throw std::runtime_error("autoscan directory " + location.string() + ": recusrive attribute " + temp + " is invalid");
        }

        bool hidden;
        temp = child.attribute("hidden-files").as_string();
        if (!string_ok(temp))
            temp = getOption("/import/attribute::hidden-files");

        if (temp == "yes")
            hidden = true;
        else if (temp == "no")
            hidden = false;
        else
            throw std::runtime_error("autoscan directory " + location.string() + ": hidden attribute " + temp + " is invalid");

        auto dir = std::make_shared<AutoscanDirectory>(location, mode, level, recursive, true, -1, interval, hidden);
        try {
            list->add(dir);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Could not add " + location.string() + ": " + e.what());
        }
    }

    return list;
}

void ConfigManager::dumpOptions()
{
#ifdef TOMBDEBUG
    log_debug("Dumping options!");
    for (int i = 0; i < (int)CFG_MAX; i++) {
        try {
            std::string opt = getOption((config_option_t)i);
            log_debug("    Option {:02d} {}", i, opt.c_str());
        } catch (const std::runtime_error& e) {
        }
        try {
            int opt = getIntOption((config_option_t)i);
            log_debug(" IntOption {:02d} {}", i, opt);
        } catch (const std::runtime_error& e) {
        }
        try {
            bool opt = getBoolOption((config_option_t)i);
            log_debug("BoolOption {:02d} {}", i, opt ? "true" : "false");
        } catch (const std::runtime_error& e) {
        }
    }
#endif
}

std::vector<std::string> ConfigManager::createArrayFromNode(const pugi::xml_node& element, std::string nodeName, std::string attrName)
{
    std::vector<std::string> arr;

    if (element != nullptr) {
        for (pugi::xml_node child: element.children()) {
            if (child.name() == nodeName) {
                std::string attrValue = child.attribute(attrName.c_str()).as_string();

                if (string_ok(attrValue))
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

std::map<std::string,std::string> ConfigManager::getDictionaryOption(config_option_t option)
{
    return options->at(option)->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getStringArrayOption(config_option_t option)
{
    return options->at(option)->getStringArrayOption();
}

std::shared_ptr<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option)
{
    return options->at(option)->getAutoscanListOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option)
{
    return options->at(option)->getTranscodingProfileListOption();
}

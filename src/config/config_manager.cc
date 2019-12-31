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

#include "config_manager.h"
#include "common.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"
#ifdef BSD_NATIVE_UUID
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <clocale>
#include <langinfo.h>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>

#endif

using namespace zmm;
using namespace mxml;

bool ConfigManager::debug_logging = false;

ConfigManager::ConfigManager(std::string filename,
    std::string userhome, std::string config_dir,
    std::string prefix_dir, std::string magic,
    std::string ip, std::string interface, int port,
    bool debug_logging)
    :filename(filename)
    , prefix_dir(prefix_dir)
    , magic(magic)
    , ip(ip)
    , interface(interface)
    , port(port)
{
    this->debug_logging = debug_logging;
    options = Ref<Array<ConfigOption>>(new Array<ConfigOption>(CFG_MAX));

    std::string home = userhome + DIR_SEPARATOR + config_dir;

    if (filename.empty()) {
        // No config file path provided, so lets find one.
        if (check_path(home + DIR_SEPARATOR + DEFAULT_CONFIG_NAME)) {
            filename = home + DIR_SEPARATOR + DEFAULT_CONFIG_NAME;
        } else {
            std::ostringstream expErrMsg;
            expErrMsg << "\nThe server configuration file could not be found in: ";
            expErrMsg << home << "\n";
            expErrMsg << "Gerbera could not find a default configuration file.\n";
            expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
            expErrMsg << "For a list of options run: gerbera -h\n";

            throw _Exception(expErrMsg.str());
        }
    }

    log_info("Loading configuration from: %s\n", filename.c_str());
    load(filename);
    validate(home);
#ifdef TOMBDEBUG
    dumpOptions();
#endif
    // now the XML is no longer needed we can destroy it
    root = nullptr;
}

ConfigManager::~ConfigManager()
{
    log_debug("ConfigManager destroyed\n");
}

std::string ConfigManager::construct_path(std::string path)
{
    std::string home = getOption(CFG_SERVER_HOME);

    if (path.at(0) == '/')
        return path;
    if (home == "." && path.at(0) == '.')
        return path;

    if (home == "")
        return "." + DIR_SEPARATOR + path;
    else
        return home + DIR_SEPARATOR + path;
}

#define NEW_OPTION(optval) opt = Ref<Option>(new Option(optval));
#define SET_OPTION(opttype) options->set(RefCast(opt, ConfigOption), opttype);

#define NEW_INT_OPTION(optval) int_opt = Ref<IntOption>(new IntOption(optval));
#define SET_INT_OPTION(opttype) \
    options->set(RefCast(int_opt, ConfigOption), opttype);

#define NEW_BOOL_OPTION(optval) bool_opt = Ref<BoolOption>(new BoolOption(optval));
#define SET_BOOL_OPTION(opttype) \
    options->set(RefCast(bool_opt, ConfigOption), opttype);

#define NEW_DICT_OPTION(optval) dict_opt = Ref<DictionaryOption>(new DictionaryOption(optval));
#define SET_DICT_OPTION(opttype) \
    options->set(RefCast(dict_opt, ConfigOption), opttype);

#define NEW_STRARR_OPTION(optval) str_array_opt = Ref<StringArrayOption>(new StringArrayOption(optval));
#define SET_STRARR_OPTION(opttype) \
    options->set(RefCast(str_array_opt, ConfigOption), opttype);

#define NEW_AUTOSCANLIST_OPTION(optval) alist_opt = Ref<AutoscanListOption>(new AutoscanListOption(optval));
#define SET_AUTOSCANLIST_OPTION(opttype) \
    options->set(RefCast(alist_opt, ConfigOption), opttype);
#define NEW_TRANSCODING_PROFILELIST_OPTION(optval) trlist_opt = Ref<TranscodingProfileListOption>(new TranscodingProfileListOption(optval));
#define SET_TRANSCODING_PROFILELIST_OPTION(opttype) \
    options->set(RefCast(trlist_opt, ConfigOption), opttype);
#ifdef ONLINE_SERVICES
#define NEW_OBJARR_OPTION(optval) obj_array_opt = Ref<ObjectArrayOption>(new ObjectArrayOption(optval));
#define SET_OBJARR_OPTION(opttype) \
    options->set(RefCast(obj_array_opt, ConfigOption), opttype);
#endif //ONLINE_SERVICES

#define NEW_OBJDICT_OPTION(optval) obj_dict_opt = Ref<ObjectDictionaryOption>(new ObjectDictionaryOption(optval));
#define SET_OBJDICT_OPTION(opttype) \
    options->set(RefCast(obj_dict_opt, ConfigOption), opttype);

void ConfigManager::validate(std::string serverhome)
{
    std::string temp;
    int temp_int;
    Ref<Element> tmpEl;

    Ref<Option> opt;
    Ref<BoolOption> bool_opt;
    Ref<IntOption> int_opt;
    Ref<DictionaryOption> dict_opt;
    Ref<StringArrayOption> str_array_opt;
    Ref<AutoscanListOption> alist_opt;
    Ref<TranscodingProfileListOption> trlist_opt;
#ifdef ONLINE_SERVICES
    Ref<ObjectArrayOption> obj_array_opt;
#endif

    log_info("Checking configuration...\n");

    // first check if the config file itself looks ok, it must have a config
    // and a server tag
    if (root->getName() != "config")
        throw _Exception("Error in config file: <config> tag not found");

    if (root->getChildByName("server") == nullptr)
        throw _Exception("Error in config file: <server> tag not found");

    std::string version = root->getAttribute("version");
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw _Exception("Config version \"" + version + "\" does not yet exist!");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server

    /// \todo clean up the construct path / prepare path mess
    getOption("/server/home", serverhome);
    NEW_OPTION(getOption("/server/home"));
    SET_OPTION(CFG_SERVER_HOME);
    prepare_path("/server/home", true);
    NEW_OPTION(getOption("/server/home"));
    SET_OPTION(CFG_SERVER_HOME);

    prepare_path("/server/webroot", true);
    NEW_OPTION(getOption("/server/webroot"));
    SET_OPTION(CFG_SERVER_WEBROOT);

    temp = getOption("/server/tmpdir", DEFAULT_TMPDIR);
    if (!check_path(temp, true)) {
        throw _Exception("Temporary directory " + temp + " does not exist!");
    }
    temp = temp + "/";
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_TMPDIR);

    if (string_ok(getOption("/server/servedir", "")))
        prepare_path("/server/servedir", true);

    NEW_OPTION(getOption("/server/servedir"));
    SET_OPTION(CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    checkOptionString("/server/udn");
    NEW_OPTION(getOption("/server/udn"));
    SET_OPTION(CFG_SERVER_UDN);

    // checking database driver options
    std::string mysql_en = "no";
    std::string sqlite3_en = "no";

    tmpEl = getElement("/server/storage");
    if (tmpEl == nullptr)
        throw _Exception("Error in config file: <storage> tag not found");

    tmpEl = getElement("/server/storage/mysql");
    if (tmpEl != nullptr) {
        mysql_en = getOption("/server/storage/mysql/attribute::enabled",
            DEFAULT_MYSQL_ENABLED);
        if (!validateYesNo(mysql_en))
            throw _Exception("Invalid <mysql enabled=\"\"> value");
    }

    tmpEl = getElement("/server/storage/sqlite3");
    if (tmpEl != nullptr) {
        sqlite3_en = getOption("/server/storage/sqlite3/attribute::enabled",
            DEFAULT_SQLITE_ENABLED);
        if (!validateYesNo(sqlite3_en))
            throw _Exception("Invalid <sqlite3 enabled=\"\"> value");
    }

    if ((sqlite3_en == "yes") && (mysql_en == "yes"))
        throw _Exception("You enabled both, sqlite3 and mysql but "
                         "only one database driver may be active at "
                         "a time!");

    if ((sqlite3_en == "no") && (mysql_en == "no"))
        throw _Exception("You disabled both, sqlite3 and mysql but "
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
        throw _Exception("You enabled MySQL storage in configuration, "
                         "however this version of Gerbera was compiled "
                         "without MySQL support!");
    }
#endif // HAVE_MYSQL

#ifdef HAVE_SQLITE3

    if (sqlite3_en == "yes") {
        prepare_path("/server/storage/sqlite3/database-file", false, true);
        NEW_OPTION(getOption("/server/storage/sqlite3/database-file"));
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
            throw _Exception("Invalid <synchronous> value in sqlite3 "
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
            throw _Exception("Invalid <on-error> value in sqlite3 "
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
            throw _Exception("Error in config file: incorrect parameter "
                             "for <backup enabled=\"\" /> attribute");
        NEW_BOOL_OPTION(temp == "yes" ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);

        temp_int = getIntOption("/server/storage/sqlite3/backup/attribute::interval",
            DEFAULT_SQLITE_BACKUP_INTERVAL);
        if (temp_int < 1)
            throw _Exception("Error in config file: incorrect parameter for "
                             "<backup interval=\"\" /> attribute");
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
    }
#else
    if (sqlite3_en == "yes") {
        throw _Exception("You enabled sqlite3 storage in configuration, "
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

    //    temp = checkOption_("/server/storage/database-file");
    //    check_path_ex(construct_path(temp));

    // now go through the optional settings and fix them if anything is missing

    temp = getOption("/server/ui/attribute::enabled",
        DEFAULT_UI_EN_VALUE);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter "
                         "for <ui enabled=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_UI_ENABLED);

    temp = getOption("/server/ui/attribute::show-tooltips",
        DEFAULT_UI_SHOW_TOOLTIPS_VALUE);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter "
                         "for <ui show-tooltips=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_UI_SHOW_TOOLTIPS);

    temp = getOption("/server/ui/attribute::poll-when-idle",
        DEFAULT_POLL_WHEN_IDLE_VALUE);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter "
                         "for <ui poll-when-idle=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_UI_POLL_WHEN_IDLE);

    temp_int = getIntOption("/server/ui/attribute::poll-interval",
        DEFAULT_POLL_INTERVAL);
    if (temp_int < 1)
        throw _Exception("Error in config file: incorrect parameter for "
                         "<ui poll-interval=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_POLL_INTERVAL);

    temp_int = getIntOption("/server/ui/items-per-page/attribute::default",
        DEFAULT_ITEMS_PER_PAGE_2);
    if (temp_int < 1)
        throw _Exception("Error in config file: incorrect parameter for "
                         "<items-per-page default=\"\" /> attribute");
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

    // now get the option list for the drop down menu
    Ref<Element> element = getElement("/server/ui/items-per-page");
    // create default structure
    if (element->elementChildCount() == 0) {
        if ((temp_int != DEFAULT_ITEMS_PER_PAGE_1) && (temp_int != DEFAULT_ITEMS_PER_PAGE_2) && (temp_int != DEFAULT_ITEMS_PER_PAGE_3) && (temp_int != DEFAULT_ITEMS_PER_PAGE_4)) {
            throw _Exception("Error in config file: you specified an "
                             "<items-per-page default=\"\"> value that is "
                             "not listed in the options");
        }

        element->appendTextChild("option",
            std::to_string(DEFAULT_ITEMS_PER_PAGE_1));
        element->appendTextChild("option",
            std::to_string(DEFAULT_ITEMS_PER_PAGE_2));
        element->appendTextChild("option",
            std::to_string(DEFAULT_ITEMS_PER_PAGE_3));
        element->appendTextChild("option",
            std::to_string(DEFAULT_ITEMS_PER_PAGE_4));
    } else // validate user settings
    {
        int i;
        bool default_found = false;
        for (int j = 0; j < element->elementChildCount(); j++) {
            Ref<Element> child = element->getElementChild(j);
            if (child->getName() == "option") {
                i = std::stoi(child->getText());
                if (i < 1)
                    throw _Exception("Error in config file: incorrect "
                                    "<option> value for <items-per-page>");

                if (i == temp_int)
                    default_found = true;
            }
        }

        if (!default_found)
            throw _Exception("Error in config file: at least one <option> "
                             "under <items-per-page> must match the "
                             "<items-per-page default=\"\" /> attribute");
    }

    // create the array from either user or default settings
    std::vector<std::string> menu_opts(element->elementChildCount());
    for (int j = 0; j < element->elementChildCount(); j++) {
        Ref<Element> child = element->getElementChild(j);
        if (child->getName() == "option")
            menu_opts.push_back(child->getText());
    }
    NEW_STRARR_OPTION(menu_opts);
    SET_STRARR_OPTION(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);

    temp = getOption("/server/ui/accounts/attribute::enabled",
        DEFAULT_ACCOUNTS_EN_VALUE);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter for "
                         "<accounts enabled=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_UI_ACCOUNTS_ENABLED);

    tmpEl = getElement("/server/ui/accounts");
    NEW_DICT_OPTION(createDictionaryFromNodeset(tmpEl, "account", "user", "password"));
    SET_DICT_OPTION(CFG_SERVER_UI_ACCOUNT_LIST);

    temp_int = getIntOption("/server/ui/accounts/attribute::session-timeout",
        DEFAULT_SESSION_TIMEOUT);
    if (temp_int < 1) {
        throw _Exception("Error in config file: invalid session-timeout "
                         "(must be > 0)\n");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UI_SESSION_TIMEOUT);

    temp = getOption("/import/attribute::hidden-files",
        DEFAULT_HIDDEN_FILES_VALUE);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter for "
                         "<import hidden-files=\"\" /> attribute");
    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_IMPORT_HIDDEN_FILES);

    temp = getOption(
        "/import/mappings/extension-mimetype/attribute::ignore-unknown",
        DEFAULT_IGNORE_UNKNOWN_EXTENSIONS);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter for "
                         "<extension-mimetype ignore-unknown=\"\" /> attribute");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);

    temp = getOption(
        "/import/mappings/extension-mimetype/attribute::case-sensitive",
        DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter for "
                         "<extension-mimetype case-sensitive=\"\" /> attribute");

    bool csens = false;

    if (temp == "yes")
        csens = true;

    NEW_BOOL_OPTION(csens);
    SET_BOOL_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE);

    tmpEl = getElement("/import/mappings/extension-mimetype");
    NEW_DICT_OPTION(createDictionaryFromNodeset(tmpEl, "map",
        "from", "to", !csens));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    tmpEl = getElement("/import/mappings/mimetype-contenttype");
    if (tmpEl != nullptr) {
        mime_content = createDictionaryFromNodeset(tmpEl, "treat",
            "mimetype", "as");
    } else {
        mime_content = Ref<Dictionary>(new Dictionary());
        mime_content->put("audio/mpeg", CONTENT_TYPE_MP3);
        mime_content->put("audio/mp4", CONTENT_TYPE_MP4);
        mime_content->put("video/mp4", CONTENT_TYPE_MP4);
        mime_content->put("application/ogg", CONTENT_TYPE_OGG);
        mime_content->put("audio/x-flac", CONTENT_TYPE_FLAC);
        mime_content->put("audio/flac", CONTENT_TYPE_FLAC);
        mime_content->put("image/jpeg", CONTENT_TYPE_JPG);
        mime_content->put("audio/x-mpegurl", CONTENT_TYPE_PLAYLIST);
        mime_content->put("audio/x-scpls", CONTENT_TYPE_PLAYLIST);
        mime_content->put("audio/x-wav", CONTENT_TYPE_PCM);
        mime_content->put("audio/wave", CONTENT_TYPE_PCM);
        mime_content->put("audio/wav", CONTENT_TYPE_PCM);
        mime_content->put("audio/vnd.wave", CONTENT_TYPE_PCM);
        mime_content->put("audio/L16", CONTENT_TYPE_PCM);
        mime_content->put("audio/x-aiff", CONTENT_TYPE_AIFF);
        mime_content->put("audio/aiff", CONTENT_TYPE_AIFF);
        mime_content->put("video/x-msvideo", CONTENT_TYPE_AVI);
        mime_content->put("video/mpeg", CONTENT_TYPE_MPEG);
    }

    NEW_DICT_OPTION(mime_content);
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "") != nullptr) {
        temp = nl_langinfo(CODESET);
        log_debug("received %s from nl_langinfo\n", temp.c_str());
    }

    if (!string_ok(temp))
        temp = DEFAULT_FILESYSTEM_CHARSET;
#else
    temp = DEFAULT_FILESYSTEM_CHARSET;
#endif
    // check if the one we take as default is actually available
    try {
        Ref<StringConverter> conv(new StringConverter(temp,
            DEFAULT_INTERNAL_CHARSET));
    } catch (const Exception& e) {
        temp = DEFAULT_FALLBACK_CHARSET;
    }
    std::string charset = getOption("/import/filesystem-charset", temp);
    try {
        Ref<StringConverter> conv(new StringConverter(charset,
            DEFAULT_INTERNAL_CHARSET));
    } catch (const Exception& e) {
        throw _Exception("Error in config file: unsupported "
                         "filesystem-charset specified: "
            + charset);
    }

    log_info("Setting filesystem import charset to %s\n", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_FILESYSTEM_CHARSET);

    charset = getOption("/import/metadata-charset", temp);
    try {
        Ref<StringConverter> conv(new StringConverter(charset,
            DEFAULT_INTERNAL_CHARSET));
    } catch (const Exception& e) {
        throw _Exception("Error in config file: unsupported "
                         "metadata-charset specified: "
            + charset);
    }

    log_info("Setting metadata import charset to %s\n", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_METADATA_CHARSET);

    charset = getOption("/import/playlist-charset", temp);
    try {
        Ref<StringConverter> conv(new StringConverter(charset,
            DEFAULT_INTERNAL_CHARSET));
    } catch (const Exception& e) {
        throw _Exception("Error in config file: unsupported playlist-charset specified: " + charset);
    }

    log_info("Setting playlist charset to %s\n", charset.c_str());
    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_PLAYLIST_CHARSET);

    temp = getOption("/server/protocolInfo/attribute::extend",
        DEFAULT_EXTEND_PROTOCOLINFO);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: extend attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO);

    /*
    temp = getOption("/server/protocolInfo/attribute::ps3-hack",
                     DEFAULT_EXTEND_PROTOCOLINFO_CL_HACK);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: ps3-hack attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_CL_HACK);
*/
    temp = getOption("/server/protocolInfo/attribute::samsung-hack",
        DEFAULT_EXTEND_PROTOCOLINFO_SM_HACK);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: samsung-hack attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK);

    temp = getOption("/server/protocolInfo/attribute::dlna-seek",
        DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: dlna-seek attribute of the "
                         "protocolInfo tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK);

    temp = getOption("/server/pc-directory/attribute::upnp-hide",
        DEFAULT_HIDE_PC_DIRECTORY);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: hide attribute of the "
                         "pc-directory tag must be either \"yes\" or \"no\"");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_HIDE_PC_DIRECTORY);

    if (!string_ok(interface)) {
        temp = getOption("/server/interface", "");
    } else {
        temp = interface;
    }
    if (string_ok(temp) && string_ok(getOption("/server/ip", "")))
        throw _Exception("Error in config file: you can not specify interface and ip at the same time!");

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
        throw _Exception("Error in config file: "
                         "invalid \"append-to\" attribute value in "
                         "<presentationURL> tag");
    }

    if (((temp == "ip") || (temp == "port")) && !string_ok(getOption("/server/presentationURL"))) {
        throw _Exception("Error in config file: \"append-to\" attribute "
                         "value in <presentationURL> tag is set to \""
            + temp + "\" but no URL is specified");
    }
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_APPEND_PRESENTATION_URL_TO);

    temp_int = getIntOption("/server/upnp-string-limit",
        DEFAULT_UPNP_STRING_LIMIT);
    if ((temp_int != -1) && (temp_int < 4)) {
        throw _Exception("Error in config file: invalid value for "
                         "<upnp-string-limit>");
    }
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

#ifdef HAVE_JS
    temp = getOption("/import/scripting/playlist-script",
        prefix_dir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_PLAYLISTS_SCRIPT);
    if (!string_ok(temp))
        throw _Exception("playlist script location invalid");
    prepare_path("/import/scripting/playlist-script");
    NEW_OPTION(getOption("/import/scripting/playlist-script"));
    SET_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);

    temp = getOption("/import/scripting/common-script",
        prefix_dir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_COMMON_SCRIPT);
    if (!string_ok(temp))
        throw _Exception("common script location invalid");
    prepare_path("/import/scripting/common-script");
    NEW_OPTION(getOption("/import/scripting/common-script"));
    SET_OPTION(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);

    temp = getOption(
        "/import/scripting/playlist-script/attribute::create-link",
        DEFAULT_PLAYLIST_CREATE_LINK);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"create-link\" attribute value in "
                         "<playlist-script> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
#endif

    temp = getOption("/import/scripting/virtual-layout/attribute::type",
        DEFAULT_LAYOUT_TYPE);
    if ((temp != "js") && (temp != "builtin") && (temp != "disabled"))
        throw _Exception("Error in config file: invalid virtual layout "
                         "type specified!");
    NEW_OPTION(temp);
    SET_OPTION(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);

#ifndef HAVE_JS
    if (temp == "js")
        throw _Exception("Gerbera was compiled without JS support, "
                         "however you specified \"js\" to be used for the "
                         "virtual-layout.");
#else
    charset = getOption("/import/scripting/attribute::script-charset",
        DEFAULT_JS_CHARSET);
    if (temp == "js") {
        try {
            Ref<StringConverter> conv(new StringConverter(charset,
                DEFAULT_INTERNAL_CHARSET));
        } catch (const Exception& e) {
            throw _Exception("Error in config file: unsupported import script charset specified: " + charset);
        }
    }

    NEW_OPTION(charset);
    SET_OPTION(CFG_IMPORT_SCRIPTING_CHARSET);

    std::string script_path = getOption(
        "/import/scripting/virtual-layout/import-script",
        prefix_dir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_IMPORT_SCRIPT);
    if (temp == "js") {
        if (!string_ok(script_path))
            throw _Exception("Error in config file: you specified \"js\" to "
                             "be used for virtual layout, but script "
                             "location is invalid.");

        prepare_path("/import/scripting/virtual-layout/import-script");
        script_path = getOption(
            "/import/scripting/virtual-layout/import-script");
    }

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
        throw _Exception("Error in config file: incorrect parameter "
                         "for /server/alive, must be at least "
            + ALIVE_INTERVAL_MIN);
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_SERVER_ALIVE_INTERVAL);

    Ref<Element> el = getElement("/import/mappings/mimetype-upnpclass");
    if (el == nullptr) {
        getOption("/import/mappings/mimetype-upnpclass", "");
    }
    NEW_DICT_OPTION(createDictionaryFromNodeset(el, "map",
        "from", "to"));
    SET_DICT_OPTION(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    temp = getOption("/import/autoscan/attribute::use-inotify", "auto");
    if ((temp != "auto") && !validateYesNo(temp))
        throw _Exception("Error in config file: incorrect parameter for "
                         "\"<autoscan use-inotify=\" attribute");

    el = getElement("/import/autoscan");

    NEW_AUTOSCANLIST_OPTION(createAutoscanListFromNodeset(nullptr, el, ScanMode::Timed));
    SET_AUTOSCANLIST_OPTION(CFG_IMPORT_AUTOSCAN_TIMED_LIST);

#ifdef HAVE_INOTIFY
    bool inotify_supported = false;
    inotify_supported = Inotify::supported();
#endif

    if (temp == YES) {
#ifdef HAVE_INOTIFY
        if (!inotify_supported)
            throw _Exception("You specified "
                             "\"yes\" in \"<autoscan use-inotify=\"\">"
                             " however your system does not have "
                             "inotify support");
#else
        throw _Exception("You specified"
                         " \"yes\" in \"<autoscan use-inotify=\"\">"
                         " however this version of Gerbera was compiled "
                         "without inotify support");
#endif
    }

#ifdef HAVE_INOTIFY
    if (temp == "auto" || (temp == YES)) {
        if (inotify_supported) {
            NEW_AUTOSCANLIST_OPTION(createAutoscanListFromNodeset(nullptr, el, ScanMode::INotify));
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
        throw _Exception("Error in config file: incorrect parameter "
                         "for <transcoding enabled=\"\"> attribute");

    if (temp == "yes")
        el = getElement("/transcoding");
    else
        el = nullptr;

    NEW_TRANSCODING_PROFILELIST_OPTION(createTranscodingProfileListFromNodeset(el));
    SET_TRANSCODING_PROFILELIST_OPTION(CFG_TRANSCODING_PROFILE_LIST);

#ifdef HAVE_CURL
    if (temp == "yes") {
        temp_int = getIntOption(
            "/transcoding/attribute::fetch-buffer-size",
            DEFAULT_CURL_BUFFER_SIZE);
        if (temp_int < CURL_MAX_WRITE_SIZE)
            throw _Exception("Error in config file: incorrect parameter "
                             "for <transcoding fetch-buffer-size=\"\"> attribute, "
                             "must be at least "
                + CURL_MAX_WRITE_SIZE);
        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);

        temp_int = getIntOption(
            "/transcoding/attribute::fetch-buffer-fill-size",
            DEFAULT_CURL_INITIAL_FILL_SIZE);
        if (temp_int < 0)
            throw _Exception("Error in config file: incorrect parameter "
                             "for <transcoding fetch-buffer-fill-size=\"\"> attribute");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }

#endif //HAVE_CURL

    el = getElement("/server/custom-http-headers");
    NEW_STRARR_OPTION(createArrayFromNodeset(el, "add", "header"));
    SET_STRARR_OPTION(CFG_SERVER_CUSTOM_HTTP_HEADERS);

#ifdef HAVE_LIBEXIF

    el = getElement("/import/library-options/libexif/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/libexif/auxdata",
            "");
    }
    NEW_STRARR_OPTION(createArrayFromNodeset(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);

#endif // HAVE_LIBEXIF

#ifdef HAVE_EXIV2

    el = getElement("/import/library-options/exiv2/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/exiv2/auxdata",
            "");
    }
    NEW_STRARR_OPTION(createArrayFromNodeset(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);

#endif // HAVE_EXIV2

#if defined(HAVE_TAGLIB)
    el = getElement("/import/library-options/id3/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/id3/auxdata", "");
    }
    NEW_STRARR_OPTION(createArrayFromNodeset(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG)
    el = getElement("/import/library-options/ffmpeg/auxdata");
    if (el == nullptr) {
        getOption("/import/library-options/ffmpeg/auxdata", "");
    }
    NEW_STRARR_OPTION(createArrayFromNodeset(el, "add-data", "tag"));
    SET_STRARR_OPTION(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                     "attribute::enabled",
        DEFAULT_FFMPEGTHUMBNAILER_ENABLED);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<ffmpegthumbnailer> tag");

    NEW_BOOL_OPTION(temp == YES ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED);

    if (temp == YES) {
        temp_int = getIntOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                                "thumbnail-size",
            DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE);

        if (temp_int <= 0)
            throw _Exception("Error in config file: ffmpegthumbnailer - "
                             "invalid value attribute value in "
                             "<thumbnail-size> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);

        temp_int = getIntOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                                "seek-percentage",
            DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        if (temp_int < 0)
            throw _Exception("Error in config file: ffmpegthumbnailer - "
                             "invalid value attribute value in "
                             "<seek-percentage> tag");

        NEW_INT_OPTION(temp_int);
        SET_INT_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "filmstrip-overlay",
            DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        if (!validateYesNo(temp))
            throw _Exception("Error in config file: ffmpegthumbnailer - "
                             "invalid value in <filmstrip-overlay> tag");

        NEW_BOOL_OPTION(temp == YES ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);

        temp = getOption("/server/extended-runtime-options/ffmpegthumbnailer/"
                         "workaround-bugs",
            DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        if (!validateYesNo(temp))
            throw _Exception("Error in config file: ffmpegthumbnailer - "
                             "invalid value in <workaround-bugs> tag");

        NEW_BOOL_OPTION(temp == YES ? true : false);
        SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);

        temp_int = getIntOption("/server/extended-runtime-options/"
                                "ffmpegthumbnailer/image-quality",
            DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY);

        if (temp_int < 0)
            throw _Exception("Error in config file: ffmpegthumbnailer - "
                             "invalid value attribute value in "
                             "<image-quality> tag, allowed values: 0-10");

        if (temp_int > 10)
            throw _Exception("Error in config file: ffmpegthumbnailer - "
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
            throw _Exception("Error in config file: "
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
        throw _Exception("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<mark-played-items> tag");

    bool markingEnabled = temp == YES ? true : false;
    NEW_BOOL_OPTION(markingEnabled);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "attribute::suppress-cds-updates",
        DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \":suppress-cds-updates\" attribute "
                         "value in <mark-played-items> tag");

    NEW_BOOL_OPTION(temp == YES ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "string/attribute::mode",
        DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);

    if ((temp != "prepend") && (temp != "append"))
        throw _Exception("Error in config file: "
                         "invalid \"mode\" attribute value in "
                         "<string> tag in the <mark-played-items> section");

    NEW_BOOL_OPTION(temp == DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);

    temp = getOption("/server/extended-runtime-options/mark-played-items/"
                     "string",
        DEFAULT_MARK_PLAYED_ITEMS_STRING);
    if (!string_ok(temp))
        throw _Exception("Error in config file: "
                         "empty string given for the <string> tag in the "
                         "<mark-played-items> section");
    NEW_OPTION(temp);
    SET_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

    std::vector<std::string> mark_content_list(element->elementChildCount());
    tmpEl = getElement("/server/extended-runtime-options/mark-played-items/mark");

    int contentElementCount = 0;
    if (tmpEl != nullptr) {
        for (int m = 0; m < tmpEl->elementChildCount(); m++) {
            Ref<Element> content = tmpEl->getElementChild(m);
            if (content->getName() != "content")
                continue;

            contentElementCount++;

            std::string mark_content = content->getText();
            if (!string_ok(mark_content))
                throw _Exception("error in configuration, <mark-played-items>, empty <content> parameter!");

            if ((mark_content != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_IMAGE))
                throw _Exception("error in configuration, <mark-played-items>, invalid <content> parameter! Allowed values are \"video\", \"audio\", \"image\"");

            mark_content_list.push_back(mark_content);
            NEW_STRARR_OPTION(mark_content_list);
            SET_STRARR_OPTION(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);
        }
    }

    if (markingEnabled && contentElementCount == 0) {
        throw _Exception("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag!");
    }

#if defined(HAVE_LASTFMLIB)
    temp = getOption("/server/extended-runtime-options/lastfm/attribute::enabled", DEFAULT_LASTFM_ENABLED);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<lastfm> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_SERVER_EXTOPTS_LASTFM_ENABLED);

    if (temp == YES) {
        temp = getOption("/server/extended-runtime-options/lastfm/username",
            DEFAULT_LASTFM_USERNAME);

        if (!string_ok(temp))
            throw _Exception("Error in config file: lastfm - "
                             "invalid username value in "
                             "<username> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_USERNAME);

        temp = getOption("/server/extended-runtime-options/lastfm/password",
            DEFAULT_LASTFM_PASSWORD);

        if (!string_ok(temp))
            throw _Exception("Error in config file: lastfm - "
                             "invalid password value in "
                             "<password> tag");

        NEW_OPTION(temp);
        SET_OPTION(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

#ifdef HAVE_MAGIC
    std::string magic_file;
    if (!string_ok(magic)) {
        if (string_ok(getOption("/import/magic-file", ""))) {
            prepare_path("/import/magic-file");
        }

        magic_file = getOption("/import/magic-file");
    } else
        magic_file = magic;

    NEW_OPTION(magic_file);
    SET_OPTION(CFG_IMPORT_MAGIC_FILE);
#endif

#ifdef HAVE_INOTIFY
    tmpEl = getElement("/import/autoscan");
    Ref<AutoscanList> config_timed_list = createAutoscanListFromNodeset(nullptr, tmpEl, ScanMode::Timed);
    Ref<AutoscanList> config_inotify_list = createAutoscanListFromNodeset(nullptr, tmpEl, ScanMode::INotify);

    for (int i = 0; i < config_inotify_list->size(); i++) {
        Ref<AutoscanDirectory> i_dir = config_inotify_list->get(i);
        for (int j = 0; j < config_timed_list->size(); j++) {
            Ref<AutoscanDirectory> t_dir = config_timed_list->get(j);
            if (i_dir->getLocation() == t_dir->getLocation())
                throw _Exception("Error in config file: same path used in both inotify and timed scan modes");
        }
    }
#endif

#ifdef SOPCAST
    temp = getOption("/import/online-content/SopCast/attribute::enabled",
        DEFAULT_SOPCAST_ENABLED);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_ENABLED);

    temp_int = getIntOption("/import/online-content/SopCast/attribute::refresh", 0);
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_REFRESH);

    temp_int = getIntOption("/import/online-content/SopCast/attribute::purge-after", 0);
    if (getIntOption("/import/online-content/SopCast/attribute::refresh") >= temp_int) {
        if (temp_int != 0)
            throw _Exception("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }

    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER);

    temp = getOption("/import/online-content/SopCast/attribute::update-at-start",
        DEFAULT_SOPCAST_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"update-at-start\" attribute value in "
                         "<SopCast> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START);
#endif

#ifdef ATRAILERS
    temp = getOption("/import/online-content/AppleTrailers/attribute::enabled",
        DEFAULT_ATRAILERS_ENABLED);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"enabled\" attribute value in "
                         "<AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);

    temp_int = getIntOption("/import/online-content/AppleTrailers/attribute::refresh", DEFAULT_ATRAILERS_REFRESH);
    NEW_INT_OPTION(temp_int);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
    SET_INT_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);

    temp = getOption("/import/online-content/AppleTrailers/attribute::update-at-start",
        DEFAULT_ATRAILERS_UPDATE_AT_START);

    if (!validateYesNo(temp))
        throw _Exception("Error in config file: "
                         "invalid \"update-at-start\" attribute value in "
                         "<AppleTrailers> tag");

    NEW_BOOL_OPTION(temp == "yes" ? true : false);
    SET_BOOL_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);

    temp = getOption("/import/online-content/AppleTrailers/attribute::resolution",
        std::to_string(DEFAULT_ATRAILERS_RESOLUTION));
    if ((temp != "640") && (temp != "720p")) {
        throw _Exception("Error in config file: "
                         "invalid \"resolution\" attribute value in "
                         "<AppleTrailers> tag, only \"640\" and \"720p\" is "
                         "supported");
    }

    NEW_OPTION(temp);
    SET_OPTION(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.\n");

    //root->indent();

    log_debug("Config file dump after validation: \n%s\n", rootDoc->print().c_str());
}

void ConfigManager::prepare_path(std::string xpath, bool needDir, bool existenceUnneeded)
{
    std::string temp;

    temp = checkOptionString(xpath);

    temp = construct_path(temp);

    check_path_ex(temp, needDir, existenceUnneeded);

    Ref<Element> script = getElement(xpath);
    if (script != nullptr)
        script->setText(temp);
}

void ConfigManager::load(std::string filename)
{
    this->filename = filename;
    Ref<Parser> parser(new Parser());
    rootDoc = parser->parseFile(filename);
    root = rootDoc->getRoot();

    if (rootDoc == nullptr) {
        throw _Exception("Unable to parse server configuration!");
    }
}

std::string ConfigManager::getOption(std::string xpath, std::string def)
{
    Ref<XPath> rootXPath(new XPath(root));
    std::string value = rootXPath->getText(xpath);
    if (string_ok(value))
        return trim_string(value);

    log_debug("Config: option not found: '%s' using default value: '%s'\n",
        xpath.c_str(), def.c_str());

    std::string pathPart = XPath::getPathPart(xpath);
    std::string axisPart = XPath::getAxisPart(xpath);

    std::vector<std::string> parts = split_string(pathPart, '/');

    Ref<Element> cur = root;
    std::string attr = "";

    size_t i;
    Ref<Element> child;
    for (i = 0; i < parts.size(); i++) {
        child = cur->getChildByName(parts[i]);
        if (child == nullptr)
            break;
        cur = child;
    }
    // here cur is the last existing element in the path
    for (; i < parts.size(); i++) {
        child = Ref<Element>(new Element(parts[i]));
        cur->appendElementChild(child);
        cur = child;
    }

    if (!axisPart.empty()) {
        std::string axis = XPath::getAxis(axisPart);
        std::string spec = XPath::getSpec(axisPart);
        if (axis != "attribute") {
            throw _Exception("ConfigManager::getOption: only attribute:: axis supported");
        }
        cur->setAttribute(spec, def);
    } else
        cur->setText(def);

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
    Ref<XPath> rootXPath(new XPath(root));
    std::string value = rootXPath->getText(xpath);
    return trim_string(value);
}

int ConfigManager::getIntOption(std::string xpath)
{
    std::string sVal = getOption(xpath);
    int val = std::stoi(sVal);
    return val;
}

Ref<Element> ConfigManager::getElement(std::string xpath)
{
    Ref<XPath> rootXPath(new XPath(root));
    return rootXPath->getElement(xpath);
}

void ConfigManager::writeBookmark(std::string ip, std::string port)
{
    FILE* f;
    std::string filename;
    std::string path;
    std::string data;
    size_t size;

    if (!getBoolOption(CFG_SERVER_UI_ENABLED)) {
        data = http_redirect_to(ip, port, "disabled.html");
    } else {
        data = http_redirect_to(ip, port);
    }

    filename = getOption(CFG_SERVER_BOOKMARK_FILE);
    path = construct_path(filename);

    log_debug("Writing bookmark file to: %s\n", path.c_str());

    f = fopen(path.c_str(), "w");
    if (f == nullptr) {
        throw _Exception("writeBookmark: failed to open: " + path);
    }

    size = fwrite(data.c_str(), sizeof(char), data.length(), f);
    fclose(f);

    if (size < data.length())
        throw _Exception("write_Bookmark: failed to write to: " + path);
}

void ConfigManager::emptyBookmark()
{
    std::string data = "<html><body><h1>Gerbera Media Server is not running.</h1><p>Please start it and try again.</p></body></html>";

    std::string filename = getOption(CFG_SERVER_BOOKMARK_FILE);
    std::string path = construct_path(filename);

    log_debug("Clearing bookmark file at: %s\n", path.c_str());

    FILE* f = fopen(path.c_str(), "w");
    if (f == nullptr) {
        throw _Exception("emptyBookmark: failed to open: " + path);
    }

    size_t size = fwrite(data.c_str(), sizeof(char), data.length(), f);
    fclose(f);

    if (size < data.length())
        throw _Exception("emptyBookmark: failed to write to: " + path);
}

std::string ConfigManager::checkOptionString(std::string xpath)
{
    std::string temp = getOption(xpath);
    if (!string_ok(temp))
        throw _Exception("Config: value of " + xpath + " tag is invalid");

    return temp;
}

Ref<Dictionary> ConfigManager::createDictionaryFromNodeset(Ref<Element> element, std::string nodeName, std::string keyAttr, std::string valAttr, bool tolower)
{
    Ref<Dictionary> dict(new Dictionary());
    std::string key;
    std::string value;

    if (element != nullptr) {
        for (int i = 0; i < element->elementChildCount(); i++) {
            Ref<Element> child = element->getElementChild(i);
            if (child->getName() == nodeName) {
                key = child->getAttribute(keyAttr);
                value = child->getAttribute(valAttr);

                if (string_ok(key) && string_ok(value)) {
                    if (tolower) {
                        key = tolower_string(key);
                    }
                    dict->put(key, value);
                }
            }
        }
    }

    return dict;
}

Ref<TranscodingProfileList> ConfigManager::createTranscodingProfileListFromNodeset(Ref<Element> element)
{
    size_t bs;
    size_t cs;
    size_t fs;
    int itmp;
    transcoding_type_t tr_type;
    Ref<Element> mtype_profile;
    bool set = false;
    std::string param;

    Ref<TranscodingProfileList> list(new TranscodingProfileList());
    if (element == nullptr)
        return list;

    Ref<Array<DictionaryElement>> mt_mappings(new Array<DictionaryElement>());

    std::string mt;
    std::string pname;

    mtype_profile = element->getChildByName("mimetype-profile-mappings");
    if (mtype_profile != nullptr) {
        for (int e = 0; e < mtype_profile->elementChildCount(); e++) {
            Ref<Element> child = mtype_profile->getElementChild(e);
            if (child->getName() == "transcode") {
                mt = child->getAttribute("mimetype");
                pname = child->getAttribute("using");

                if (string_ok(mt) && string_ok(pname)) {
                    Ref<DictionaryElement> del(new DictionaryElement(mt, pname));
                    mt_mappings->append(del);
                } else {
                    throw _Exception("error in configuration: invalid or missing mimetype to profile mapping");
                }
            }
        }
    }

    Ref<Element> profiles = element->getChildByName("profiles");
    if (profiles == nullptr)
        return list;

    for (int i = 0; i < profiles->elementChildCount(); i++) {
        Ref<Element> child = profiles->getElementChild(i);
        if (child->getName() != "profile")
            continue;

        param = child->getAttribute("enabled");
        if (!validateYesNo(param))
            throw _Exception("Error in config file: incorrect parameter "
                             "for <profile enabled=\"\" /> attribute");

        if (param == "no")
            continue;

        param = child->getAttribute("type");
        if (!string_ok(param))
            throw _Exception("error in configuration: missing transcoding type in profile");

        if (param == "external")
            tr_type = TR_External;
        /* for the future...
        else if (param == "remote")
            tr_type = TR_Remote;
         */
        else
            throw _Exception("error in configuration: invalid transcoding type " + param + " in profile");

        param = child->getAttribute("name");
        if (!string_ok(param))
            throw _Exception("error in configuration: invalid transcoding profile name");

        Ref<TranscodingProfile> prof(new TranscodingProfile(tr_type, param));

        param = child->getChildText("mimetype");
        if (!string_ok(param))
            throw _Exception("error in configuration: invalid target mimetype in transcoding profile");
        prof->setTargetMimeType(param);

        if (child->getChildByName("resolution") != nullptr) {
            param = child->getChildText("resolution");
            if (string_ok(param)) {
                if (check_resolution(param))
                    prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
            }
        }

        Ref<Element> avi_fcc = child->getChildByName("avi-fourcc-list");
        if (avi_fcc != nullptr) {
            std::string mode = avi_fcc->getAttribute("mode");
            if (!string_ok(mode))
                throw _Exception("error in configuration: avi-fourcc-list requires a valid \"mode\" attribute");

            avi_fourcc_listmode_t fcc_mode;
            if (mode == "ignore")
                fcc_mode = FCC_Ignore;
            else if (mode == "process")
                fcc_mode = FCC_Process;
            else if (mode == "disabled")
                fcc_mode = FCC_None;
            else
                throw _Exception("error in configuration: invalid mode given for avi-fourcc-list: \"" + mode + "\"");

            if (fcc_mode != FCC_None) {
                std::vector<std::string> fcc_list(avi_fcc->elementChildCount());
                for (int f = 0; f < avi_fcc->elementChildCount(); f++) {
                    Ref<Element> fourcc = avi_fcc->getElementChild(f);
                    if (fourcc->getName() != "fourcc")
                        continue;

                    std::string fcc = fourcc->getText();
                    if (!string_ok(fcc))
                        throw _Exception("error in configuration: empty fourcc specified!");
                    fcc_list.push_back(fcc);
                }

                prof->setAVIFourCCList(fcc_list, fcc_mode);
            }
        }

        if (child->getChildByName("accept-url") != nullptr) {
            param = child->getChildText("accept-url");
            if (!validateYesNo(param))
                throw _Exception("Error in config file: incorrect parameter "
                                 "for <accept-url> tag");
            if (param == "yes")
                prof->setAcceptURL(true);
            else
                prof->setAcceptURL(false);
        }

        if (child->getChildByName("sample-frequency") != nullptr) {
            param = child->getChildText("sample-frequency");
            if (param == "source")
                prof->setSampleFreq(SOURCE);
            else if (param == "off")
                prof->setSampleFreq(OFF);
            else {
                int freq = std::stoi(param);
                if (freq <= 0)
                    throw _Exception("Error in config file: incorrect "
                                     "parameter for <sample-frequency> "
                                     "tag");

                prof->setSampleFreq(freq);
            }
        }

        if (child->getChildByName("audio-channels") != nullptr) {
            param = child->getChildText("audio-channels");
            if (param == "source")
                prof->setNumChannels(SOURCE);
            else if (param == "off")
                prof->setNumChannels(OFF);
            else {
                int chan = std::stoi(param);
                if (chan <= 0)
                    throw _Exception("Error in config file: incorrect "
                                     "parameter for <number-of-channels> "
                                     "tag");
                prof->setNumChannels(chan);
            }
        }

        if (child->getChildByName("hide-original-resource") != nullptr) {
            param = child->getChildText("hide-original-resource");
            if (!validateYesNo(param))
                throw _Exception("Error in config file: incorrect parameter "
                                 "for <hide-original-resource> tag");
            if (param == "yes")
                prof->setHideOriginalResource(true);
            else
                prof->setHideOriginalResource(false);
        }

        if (child->getChildByName("thumbnail") != nullptr) {
            param = child->getChildText("thumbnail");
            if (!validateYesNo(param))
                throw _Exception("Error in config file: incorrect parameter "
                                 "for <thumbnail> tag");
            if (param == "yes")
                prof->setThumbnail(true);
            else
                prof->setThumbnail(false);
        }

        if (child->getChildByName("first-resource") != nullptr) {
            param = child->getChildText("first-resource");
            if (!validateYesNo(param))
                throw _Exception("Error in config file: incorrect parameter "
                                 "for <profile first-resource=\"\" /> attribute");

            if (param == "yes")
                prof->setFirstResource(true);
            else
                prof->setFirstResource(false);
        }

        if (child->getChildByName("use-chunked-encoding") != nullptr) {
            param = child->getChildText("use-chunked-encoding");
            if (!validateYesNo(param))
                throw _Exception("Error in config file: incorrect parameter "
                                 "for use-chunked-encoding tag");

            if (param == "yes")
                prof->setChunked(true);
            else
                prof->setChunked(false);
        }

        Ref<Element> sub = child->getChildByName("agent");
        if (sub == nullptr)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" is missing the <agent> option");

        param = sub->getAttribute("command");
        if (!string_ok(param))
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" has an invalid command setting");
        prof->setCommand(param);

        std::string tmp_path;
        if (startswith(param, _DIR_SEPARATOR)) {
            if (!check_path(param))
                throw _Exception("error in configuration, transcoding "
                                 "profile \""
                    + prof->getName() + "\" could not find transcoding command " + param);
            tmp_path = param;
        } else {
            tmp_path = find_in_path(param);
            if (!string_ok(tmp_path))
                throw _Exception("error in configuration, transcoding "
                                 "profile \""
                    + prof->getName() + "\" could not find transcoding command " + param + " in $PATH");
        }

        int err = 0;
        if (!is_executable(tmp_path, &err))
            throw _Exception("error in configuration, transcoding "
                             "profile "
                + prof->getName() + ": transcoder " + param + "is not executable - " + strerror(err));

        param = sub->getAttribute("arguments");
        if (!string_ok(param))
            throw _Exception("error in configuration: transcoding profile " + prof->getName() + " has an empty argument string");

        prof->setArguments(param);

        sub = child->getChildByName("buffer");
        if (sub == nullptr)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" is missing the <buffer> option");

        param = sub->getAttribute("size");
        if (!string_ok(param))
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" <buffer> tag is missing the size attribute");
        itmp = std::stoi(param);
        if (itmp < 0)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" buffer size can not be negative");
        bs = itmp;

        param = sub->getAttribute("chunk-size");
        if (!string_ok(param))
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" <buffer> tag is missing the chunk-size "
                                    "attribute");
        itmp = std::stoi(param);
        if (itmp < 0)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" chunk size can not be negative");
        cs = itmp;

        if (cs > bs)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" chunk size can not be greater than "
                                    "buffer size");

        param = sub->getAttribute("fill-size");
        if (!string_ok(param))
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" <buffer> tag is missing the fill-size "
                                    "attribute");
        itmp = std::stoi(param);
        if (i < 0)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" fill size can not be negative");
        fs = itmp;

        if (fs > bs)
            throw _Exception("error in configuration: transcoding "
                             "profile \""
                + prof->getName() + "\" fill size can not be greater than "
                                    "buffer size");

        prof->setBufferOptions(bs, cs, fs);

        if (mtype_profile == nullptr) {
            throw _Exception("error in configuration: transcoding "
                             "profiles exist, but no mimetype to profile "
                             "mappings specified");
        }

        for (int k = 0; k < mt_mappings->size(); k++) {
            if (mt_mappings->get(k)->getValue() == prof->getName()) {
                list->add(mt_mappings->get(k)->getKey(), prof);
                set = true;
            }
        }

        if (!set)
            throw _Exception("error in configuration: you specified"
                             "a mimetype to transcoding profile mapping, "
                             "but no match for profile \""
                + prof->getName() + "\" exists");
        else
            set = false;
    }

    return list;
}

Ref<AutoscanList> ConfigManager::createAutoscanListFromNodeset(std::shared_ptr<Storage> storage, zmm::Ref<mxml::Element> element, ScanMode scanmode)
{
    Ref<AutoscanList> list(new AutoscanList(storage));

    if (element == nullptr)
        return list;

    for (int i = 0; i < element->elementChildCount(); i++) {

        Ref<Element> child = element->getElementChild(i);

        // We only want directories
        if (child->getName() != "directory")
            continue;

        std::string location = child->getAttribute("location");
        if (!string_ok(location)) {
            throw _Exception("autoscan directory with invalid location!\n");
        }

        try {
            location = normalizePath(location);
        } catch (const Exception& e) {
            throw _Exception("autoscan directory \"" + location + "\": " + e.getMessage());
        }

        if (check_path(location, false)) {
            throw _Exception("autoscan " + location + " - not a directory!");
        }

        ScanMode mode;
        std::string temp = child->getAttribute("mode");
        if (!string_ok(temp) || ((temp != "timed") && (temp != "inotify"))) {
            throw _Exception("autoscan directory " + location + ": mode attribute is missing or invalid");
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
            temp = child->getAttribute("level");
            if (!string_ok(temp)) {
                throw _Exception("autoscan directory " + location + ": level attribute is missing or invalid");
            } else {
                if (temp == "basic")
                    level = ScanLevel::Basic;
                else if (temp == "full")
                    level = ScanLevel::Full;
                else {
                    throw _Exception("autoscan directory "
                        + location + ": level attribute " + temp + "is invalid");
                }
            }

            temp = child->getAttribute("interval");
            if (!string_ok(temp)) {
                throw _Exception("autoscan directory "
                    + location + ": interval attribute is required for timed mode");
            }

            interval = std::stoi(temp);

            if (interval == 0) {
                throw _Exception("autoscan directory " + location + ": invalid interval attribute");
                continue;
            }
        } else {
            // level is irrelevant for inotify scan, nevertheless we will set
            // it to something valid
            level = ScanLevel::Full;
        }

        temp = child->getAttribute("recursive");
        if (!string_ok(temp))
            throw _Exception("autoscan directory " + location + ": recursive attribute is missing or invalid");

        bool recursive;
        if (temp == "yes")
            recursive = true;
        else if (temp == "no")
            recursive = false;
        else {
            throw _Exception("autoscan directory " + location
                + ": recusrive attribute " + temp + " is invalid");
        }

        bool hidden;
        temp = child->getAttribute("hidden-files");
        if (!string_ok(temp))
            temp = getOption("/import/attribute::hidden-files");

        if (temp == "yes")
            hidden = true;
        else if (temp == "no")
            hidden = false;
        else
            throw _Exception("autoscan directory " + location + ": hidden attribute " + temp + " is invalid");

        Ref<AutoscanDirectory> dir(new AutoscanDirectory(location, mode, level, recursive, true, -1, interval, hidden));
        try {
            list->add(dir);
        } catch (const Exception& e) {
            throw _Exception("Could not add " + location + ": "
                + e.getMessage());
        }
    }

    return list;
}

void ConfigManager::dumpOptions()
{
#ifdef TOMBDEBUG
    log_debug("Dumping options!\n");
    for (int i = 0; i < (int)CFG_MAX; i++) {
        try {
            log_debug("    Option %02d - %s\n", i,
                getOption((config_option_t)i).c_str());
        } catch (const Exception& e) {
        }
        try {
            log_debug(" IntOption %02d - %d\n", i,
                getIntOption((config_option_t)i));
        } catch (const Exception& e) {
        }
        try {
            log_debug("BoolOption %02d - %s\n", i,
                (getBoolOption((config_option_t)i) ? "true" : "false"));
        } catch (const Exception& e) {
        }
    }
#endif
}

std::vector<std::string> ConfigManager::createArrayFromNodeset(Ref<mxml::Element> element, std::string nodeName, std::string attrName)
{
    std::vector<std::string> arr;

    if (element != nullptr) {
        for (int i = 0; i < element->elementChildCount(); i++) {
            Ref<Element> child = element->getElementChild(i);
            if (child->getName() == nodeName) {
                std::string attrValue = child->getAttribute(attrName);

                if (string_ok(attrValue))
                    arr.push_back(attrValue);
            }
        }
    }

    return arr;
}

// The validate function ensures that the array is completely filled!
// None of the options->get() calls will ever return nullptr!
std::string ConfigManager::getOption(config_option_t option)
{
    Ref<ConfigOption> r = options->get(option);
    if (r.getPtr() == nullptr) {
        throw _Exception("option not set");
    }
    return r->getOption();
}

int ConfigManager::getIntOption(config_option_t option)
{
    Ref<ConfigOption> o = options->get(option);
    if (o.getPtr() == nullptr) {
        throw _Exception("option not set");
    }
    return o->getIntOption();
}

bool ConfigManager::getBoolOption(config_option_t option)
{
    Ref<ConfigOption> o = options->get(option);
    if (o.getPtr() == nullptr) {
        throw _Exception("option not set");
    }
    return o->getBoolOption();
}

Ref<Dictionary> ConfigManager::getDictionaryOption(config_option_t option)
{
    return options->get(option)->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getStringArrayOption(config_option_t option)
{
    return options->get(option)->getStringArrayOption();
}

Ref<ObjectDictionary<zmm::Object>> ConfigManager::getObjectDictionaryOption(config_option_t option)
{
    return options->get(option)->getObjectDictionaryOption();
}

#ifdef ONLINE_SERVICES
Ref<Array<Object>> ConfigManager::getObjectArrayOption(config_option_t option)
{
    return options->get(option)->getObjectArrayOption();
}
#endif

Ref<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option)
{
    return options->get(option)->getAutoscanListOption();
}

Ref<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option)
{
    return options->get(option)->getTranscodingProfileListOption();
}

#ifdef ONLINE_SERVICES
Ref<Array<Object>> ConfigManager::createServiceTaskList(service_type_t service,
    Ref<Element> element)
{
    Ref<Array<Object>> arr(new Array<Object>());

    if (element == nullptr)
        return arr;
    return arr;
}
#endif

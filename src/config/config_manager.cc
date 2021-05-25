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

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <clocale>
#include <langinfo.h>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "client_config.h"
#include "config_definition.h"
#include "config_options.h"
#include "config_setup.h"
#include "content/autoscan.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"
#include "util/string_converter.h"
#include "util/tools.h"

bool ConfigManager::debug = false;

ConfigManager::ConfigManager(fs::path filename,
    const fs::path& userHome, const fs::path& configDir,
    fs::path dataDir, fs::path magicFile,
    std::string ip, std::string interface, in_port_t port,
    bool debug)
    : filename(std::move(filename))
    , dataDir(std::move(dataDir))
    , magicFile(std::move(magicFile))
    , ip(std::move(ip))
    , interface(std::move(interface))
    , port(port)
    , xmlDoc(std::make_unique<pugi::xml_document>())
    , options(std::make_unique<std::vector<std::shared_ptr<ConfigOption>>>())
{
    ConfigManager::debug = debug;

    options->resize(CFG_MAX);

    if (this->filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userHome / configDir;
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

std::shared_ptr<ConfigOption> ConfigManager::setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments)
{
    auto co = ConfigDefinition::findConfigSetup(option);
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
    if (root.name() != ConfigSetup::ROOT_NAME)
        throw std::runtime_error("Error in config file: <config> tag not found");

    if (root.child("server") == nullptr)
        throw std::runtime_error("Error in config file: <server> tag not found");

    std::string version = root.attribute("version").as_string();
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw std::runtime_error("Config version \"" + version + "\" does not yet exist");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server
    co = ConfigDefinition::findConfigSetup(CFG_SERVER_HOME);
    if (!userHome.empty()) {
        // respect command line; ignore xml value
        temp = userHome;
    } else {
        temp = co->getXmlContent(root);
    }

    if (!fs::is_directory(temp))
        throw_std_runtime_error("Directory '{}' does not exist", temp);
    co->makeOption(temp, self);
    ConfigPathSetup::Home = temp;

    // read root options
    setOption(root, CFG_SERVER_WEBROOT);
    setOption(root, CFG_SERVER_TMPDIR);
    setOption(root, CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    setOption(root, CFG_SERVER_UDN);

    // checking database driver options
    bool mysql_en = false;
    bool sqlite3_en = false;

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE);
    co->getXmlElement(root); // fails if missing
    setOption(root, CFG_SERVER_STORAGE_USE_TRANSACTIONS);

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL);
    if (co->hasXmlElement(root)) {
        mysql_en = setOption(root, CFG_SERVER_STORAGE_MYSQL_ENABLED)->getBoolOption();
    }

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE);
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
    // read mysql options
    if (mysql_en) {
        setOption(root, CFG_SERVER_STORAGE_MYSQL_HOST);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_DATABASE);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_USERNAME);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PORT);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_SOCKET);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PASSWORD);

        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "mysql.sql");
        co->makeOption(root, self);
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

        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        fs::path dir = dataDir / "sqlite3.sql";
        co->setDefaultValue(dir.string());
        co->makeOption(root, self);
    }

    std::string dbDriver;
    if (sqlite3_en)
        dbDriver = "sqlite3";
    if (mysql_en)
        dbDriver = "mysql";

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, self);

    // now go through the optional settings and fix them if anything is missing
    setOption(root, CFG_SERVER_UI_ENABLED);
    setOption(root, CFG_SERVER_UI_SHOW_TOOLTIPS);
    setOption(root, CFG_SERVER_UI_POLL_WHEN_IDLE);
    setOption(root, CFG_SERVER_UI_POLL_INTERVAL);

    auto def_ipp = setOption(root, CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)->getIntOption();

    // now get the option list for the drop down menu
    auto menu_opts = setOption(root, CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN)->getArrayOption();
    if (std::find(menu_opts.begin(), menu_opts.end(), fmt::to_string(def_ipp)) == menu_opts.end())
        throw std::runtime_error("Error in config file: at least one <option> "
                                 "under <items-per-page> must match the "
                                 "<items-per-page default=\"\" /> attribute");

    // read account options
    setOption(root, CFG_SERVER_UI_ACCOUNTS_ENABLED);
    setOption(root, CFG_SERVER_UI_ACCOUNT_LIST);
    setOption(root, CFG_SERVER_UI_SESSION_TIMEOUT);

    // read upnp options
    setOption(root, CFG_UPNP_ALBUM_PROPERTIES);
    setOption(root, CFG_UPNP_ARTIST_PROPERTIES);
    setOption(root, CFG_UPNP_TITLE_PROPERTIES);
    setOption(root, CFG_THREAD_SCOPE_SYSTEM);

    bool cl_en = setOption(root, CFG_CLIENTS_LIST_ENABLED)->getBoolOption();
    args["isEnabled"] = cl_en ? "true" : "false";
    setOption(root, CFG_CLIENTS_LIST, &args);
    args.clear();

    setOption(root, CFG_IMPORT_HIDDEN_FILES);
    setOption(root, CFG_IMPORT_FOLLOW_SYMLINKS);
    setOption(root, CFG_IMPORT_READABLE_NAMES);
    setOption(root, CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    bool csens = setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE)->getBoolOption();
    args["tolower"] = fmt::to_string(!csens);
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
    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_FILESYSTEM_CHARSET);
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

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_METADATA_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported metadata-charset specified: " + charset);
    }
    log_debug("Setting metadata import charset to {}", charset.c_str());
    co->makeOption(charset, self);

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported playlist-charset specified: " + charset);
    }
    log_debug("Setting playlist charset to {}", charset.c_str());
    co->makeOption(charset, self);

    setOption(root, CFG_SERVER_HIDE_PC_DIRECTORY);

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_NETWORK_INTERFACE);
    if (interface.empty()) {
        temp = co->getXmlContent(root);
    } else {
        temp = interface;
    }
    co->makeOption(temp, self);

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_IP);
    if (ip.empty()) {
        temp = co->getXmlContent(root); // bind to any IP address
    } else {
        temp = ip;
    }
    co->makeOption(temp, self);

    if (!getOption(CFG_SERVER_NETWORK_INTERFACE).empty() && !getOption(CFG_SERVER_IP).empty())
        throw std::runtime_error("Error in config file: you can not specify interface and ip at the same time");

    // read server options
    setOption(root, CFG_SERVER_BOOKMARK_FILE);
    setOption(root, CFG_SERVER_NAME);
    setOption(root, CFG_SERVER_MODEL_NAME);
    setOption(root, CFG_SERVER_MODEL_DESCRIPTION);
    setOption(root, CFG_SERVER_MODEL_NUMBER);
    setOption(root, CFG_SERVER_MODEL_URL);
    setOption(root, CFG_SERVER_SERIAL_NUMBER);
    setOption(root, CFG_SERVER_MANUFACTURER);
    setOption(root, CFG_SERVER_MANUFACTURER_URL);
    setOption(root, CFG_VIRTUAL_URL);
    setOption(root, CFG_SERVER_PRESENTATION_URL);
    setOption(root, CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

    temp = setOption(root, CFG_SERVER_APPEND_PRESENTATION_URL_TO)->getOption();
    if (((temp == "ip") || (temp == "port")) && getOption(CFG_SERVER_PRESENTATION_URL).empty()) {
        throw_std_runtime_error("Error in config file: \"append-to\" attribute "
                                "value in <presentationURL> tag is set to \"{}\""
                                "but no URL is specified",
            temp.c_str());
    }

#ifdef HAVE_JS
    // read javascript options
    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT);
    co->makeOption(root, self);

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT);
    co->makeOption(root, self);

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_CUSTOM_SCRIPT);
    args["resolveEmpty"] = "false";
    co->makeOption(root, self, &args);
    args.clear();

    setOption(root, CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT_OPTIONS);
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_AUDIO);
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_VIDEO);
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_IMAGE);
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_LAYOUT_TRAILER);
#endif
    setOption(root, CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP);

    auto layoutType = setOption(root, CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE)->getOption();

#ifndef HAVE_JS
    if (layoutType == "js")
        throw std::runtime_error("Gerbera was compiled without JS support, "
                                 "however you specified \"js\" to be used for the "
                                 "virtual-layout.");
#else
    // read more javascript options
    charset = setOption(root, CFG_IMPORT_SCRIPTING_CHARSET)->getOption();
    if (layoutType == "js") {
        try {
            auto conv = std::make_unique<StringConverter>(charset,
                DEFAULT_INTERNAL_CHARSET);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Error in config file: unsupported import script charset specified: " + charset);
        }
    }

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
    args["mustExist"] = fmt::to_string(layoutType == "js");
    args["notEmpty"] = fmt::to_string(layoutType == "js");
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    co->makeOption(root, self, &args);
    args.clear();
    auto script_path = co->getValue()->getOption();

#endif
    co = ConfigDefinition::findConfigSetup(CFG_SERVER_PORT);
    // 0 means, that the SDK will any free port itself
    co->makeOption((port == 0) ? co->getXmlContent(root) : fmt::to_string(port), self);

    setOption(root, CFG_SERVER_ALIVE_INTERVAL);
    setOption(root, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    args["hiddenFiles"] = getBoolOption(CFG_IMPORT_HIDDEN_FILES) ? "true" : "false";
    setOption(root, CFG_IMPORT_AUTOSCAN_TIMED_LIST, &args);

#ifdef HAVE_INOTIFY
    auto useInotify = setOption(root, CFG_IMPORT_AUTOSCAN_USE_INOTIFY)->getBoolOption();

    if (useInotify) {
        setOption(root, CFG_IMPORT_AUTOSCAN_INOTIFY_LIST, &args);
    } else {
        setOption({}, CFG_IMPORT_AUTOSCAN_INOTIFY_LIST); // set empty list
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

    // read import options
    setOption(root, CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
    setOption(root, CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION);
    setOption(root, CFG_IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT);
    setOption(root, CFG_IMPORT_RESOURCES_CONTAINERART_MINDEPTH);
    setOption(root, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    setOption(root, CFG_IMPORT_DIRECTORIES_LIST);
    setOption(root, CFG_IMPORT_SYSTEM_DIRECTORIES);

    args["trim"] = "false";
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

    // read library options
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
    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_MAGIC_FILE);
    args["isFile"] = "true";
    args["resolveEmpty"] = "false";
    co->makeOption(!magicFile.empty() ? magicFile.string() : co->getXmlContent(root), self, &args);
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

    // read online content options
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

    co = ConfigDefinition::findConfigSetup(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
    co->makeOption(fmt::to_string(atrailers_refresh), self);

    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);
    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str().c_str());

    // now the XML is no longer needed we can destroy it
    xmlDoc = nullptr;
}

void ConfigManager::updateConfigFromDatabase(std::shared_ptr<Database> database)
{
    auto values = database->getConfigValues();
    auto self = getSelf();
    origValues.clear();
    log_info("Loading {} configuration items from database", values.size());

    for (auto&& cfgValue : values) {
        try {
            auto cs = ConfigDefinition::findConfigSetupByPath(cfgValue.key, true);

            if (cs != nullptr) {
                if (cfgValue.item == cs->xpath) {
                    origValues[cfgValue.item] = cs->getCurrentValue();
                    cs->makeOption(cfgValue.value, self);
                } else {
                    std::string parValue = cfgValue.value;
                    if (cfgValue.status == STATUS_CHANGED || cfgValue.status == STATUS_UNCHANGED) {
                        if (!cs->updateDetail(cfgValue.item, parValue, self)) {
                            log_error("unhandled {} option {} != {}", cfgValue.status, cfgValue.item, cs->xpath);
                        }
                    } else if (cfgValue.status == STATUS_REMOVED || cfgValue.status == STATUS_ADDED || cfgValue.status == STATUS_MANUAL) {
                        std::map<std::string, std::string> arguments = { { "status", cfgValue.status } };
                        if (!cs->updateDetail(cfgValue.item, parValue, self, &arguments)) {
                            log_error("unhandled {} option {} != {}", cfgValue.status, cfgValue.item, cs->xpath);
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
    origValues.try_emplace(item, value ? "true" : "false");
}

void ConfigManager::setOrigValue(const std::string& item, int value)
{
    origValues.try_emplace(item, fmt::format("{}", value));
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

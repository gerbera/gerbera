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
#include <sstream>
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
{
    ConfigManager::debug = debug;

    if (this->filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userHome / configDir;
        this->filename = home / DEFAULT_CONFIG_NAME;
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
    options.at(option) = optionValue;
}

void ConfigManager::load(const fs::path& userHome)
{
    std::map<std::string, std::string> args;
    auto self = getSelf();

    log_info("Loading configuration from: {}", filename.c_str());
    pugi::xml_parse_result result = xmlDoc->load_file(filename.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }

    log_info("Checking configuration...");

    auto root = xmlDoc->document_element();

    // first check if the config file itself looks ok,
    // it must have a config and a server tag
    if (root.name() != ConfigSetup::ROOT_NAME)
        throw_std_runtime_error("Error in config file: <config> tag not found");

    if (!root.child("server"))
        throw_std_runtime_error("Error in config file: <server> tag not found");

    std::string version = root.attribute("version").as_string();
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw_std_runtime_error("Config version \"{}\" does not yet exist", version);

    // now go through the mandatory parameters,
    // if something is missing we will not start the server
    auto co = ConfigDefinition::findConfigSetup(CFG_SERVER_HOME);
    // respect command line if available; ignore xml value
    {
        std::string temp = !userHome.empty() ? userHome.string() : co->getXmlContent(root);
        if (!fs::is_directory(temp))
            throw_std_runtime_error("Directory '{}' does not exist", temp);
        co->makeOption(temp, self);
        ConfigPathSetup::Home = temp;
    }

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
        throw_std_runtime_error("You enabled both, sqlite3 and mysql but "
                                "only one database driver may be active at a time");

    if (!sqlite3_en && !mysql_en)
        throw_std_runtime_error("You disabled both sqlite3 and mysql but "
                                "one database driver must be active");

    std::string dbDriver;

#ifdef HAVE_MYSQL
    if (mysql_en) {
        // read mysql options
        setOption(root, CFG_SERVER_STORAGE_MYSQL_HOST);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_DATABASE);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_USERNAME);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PORT);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_SOCKET);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PASSWORD);

        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "mysql.sql");
        co->makeOption(root, self);
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "mysql-upgrade.xml");
        co->makeOption(root, self);
        dbDriver = "mysql";
    }
#else
    if (mysql_en) {
        throw_std_runtime_error("You enabled MySQL database in configuration, "
                                "however this version of Gerbera was compiled "
                                "without MySQL support!");
    }
#endif // HAVE_MYSQL

    if (sqlite3_en) {
        // read sqlite options
        setOption(root, CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_RESTORE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);

        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "sqlite3.sql");
        co->makeOption(root, self);
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "sqlite3-upgrade.xml");
        co->makeOption(root, self);
        dbDriver = "sqlite3";
    }

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, self);

    // now go through the optional settings and fix them if anything is missing
    auto def_ipp = setOption(root, CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)->getIntOption();

    // now get the option list for the drop down menu
    auto menu_opts = setOption(root, CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN)->getArrayOption();
    if (std::find(menu_opts.begin(), menu_opts.end(), fmt::to_string(def_ipp)) == menu_opts.end())
        throw_std_runtime_error("Error in config file: at least one <option> "
                                "under <items-per-page> must match the "
                                "<items-per-page default=\"\" /> attribute");

    bool cl_en = setOption(root, CFG_CLIENTS_LIST_ENABLED)->getBoolOption();
    args["isEnabled"] = cl_en ? "true" : "false";
    setOption(root, CFG_CLIENTS_LIST, &args);
    args.clear();

    setOption(root, CFG_IMPORT_HIDDEN_FILES);
    bool csens = setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE)->getBoolOption();
    args["tolower"] = fmt::to_string(!csens);
    setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, &args);
    args.clear();

    std::string defaultCharSet = DEFAULT_FILESYSTEM_CHARSET;
#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "")) {
        defaultCharSet = nl_langinfo(CODESET);
        log_debug("received {} from nl_langinfo", defaultCharSet);
    }
#endif
    // check if the one we take as default is actually available
    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_FILESYSTEM_CHARSET);
    try {
        auto conv = std::make_unique<StringConverter>(defaultCharSet, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        defaultCharSet = DEFAULT_FALLBACK_CHARSET;
    }
    co->setDefaultValue(defaultCharSet);

    std::string charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: unsupported filesystem-charset specified: {}\n{}", charset, e.what());
    }
    log_debug("Setting filesystem import charset to {}", charset);
    co->makeOption(charset, self);

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_METADATA_CHARSET);
    co->setDefaultValue(defaultCharSet);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: unsupported metadata-charset specified: {}\n{}", charset, e.what());
    }
    log_debug("Setting metadata import charset to {}", charset);
    co->makeOption(charset, self);

    // read playlist options
    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(defaultCharSet);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: unsupported playlist-charset specified: {}\n{}", charset, e.what());
    }
    log_debug("Setting playlist charset to {}", charset);
    co->makeOption(charset, self);

    // read network options
    co = ConfigDefinition::findConfigSetup(CFG_SERVER_NETWORK_INTERFACE);
    co->makeOption((interface.empty()) ? co->getXmlContent(root) : interface, self);

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_IP);
    // bind to any IP address
    co->makeOption(ip.empty() ? co->getXmlContent(root) : ip, self);

    if (!getOption(CFG_SERVER_NETWORK_INTERFACE).empty() && !getOption(CFG_SERVER_IP).empty())
        throw_std_runtime_error("Error in config file: you can not specify interface and ip at the same time");

    // read server options
    {
        std::string temp = setOption(root, CFG_SERVER_APPEND_PRESENTATION_URL_TO)->getOption();
        if ((temp == "ip" || temp == "port") && getOption(CFG_SERVER_PRESENTATION_URL).empty()) {
            throw_std_runtime_error("Error in config file: \"append-to\" attribute "
                                    "value in <presentationURL> tag is set to \"{}\""
                                    "but no URL is specified",
                temp);
        }
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
#endif

    auto layoutType = setOption(root, CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE)->getOption();

#ifndef HAVE_JS
    if (layoutType == "js")
        throw_std_runtime_error("Gerbera was compiled without JS support, "
                                "however you specified \"js\" to be used for the "
                                "virtual-layout.");
#else
    // read more javascript options
    charset = setOption(root, CFG_IMPORT_SCRIPTING_CHARSET)->getOption();
    if (layoutType == "js") {
        try {
            auto conv = std::make_unique<StringConverter>(charset, DEFAULT_INTERNAL_CHARSET);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Error in config file: unsupported import script charset specified: {}\n{}", charset, e.what());
        }
    }

    co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
    args["mustExist"] = fmt::to_string(layoutType == "js");
    args["notEmpty"] = fmt::to_string(layoutType == "js");
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    co->makeOption(root, self, &args);
    args.clear();
#endif

    co = ConfigDefinition::findConfigSetup(CFG_SERVER_PORT);
    // 0 means, that the SDK will any free port itself
    co->makeOption((port == 0) ? co->getXmlContent(root) : fmt::to_string(port), self);

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

    // read transcoding options
    auto tr_en = setOption(root, CFG_TRANSCODING_TRANSCODING_ENABLED)->getBoolOption();
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
    args["trim"] = "false";
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, CFG_UPNP_SEARCH_SEPARATOR, &args);
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

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
    bool contentArrayEmpty = setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST)->getArrayOption().empty();
    if (markingEnabled && contentArrayEmpty) {
        throw_std_runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag");
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

    for (std::size_t i = 0; i < config_inotify_list->size(); i++) {
        auto i_dir = config_inotify_list->get(i);
        for (std::size_t j = 0; j < config_timed_list->size(); j++) {
            auto t_dir = config_timed_list->get(j);
            if (i_dir->getLocation() == t_dir->getLocation())
                throw_std_runtime_error("Error in config file: same path used in both inotify and timed scan modes");
        }
    }
#endif

    // read online content options
#ifdef SOPCAST
    int sopcast_refresh = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_REFRESH)->getIntOption();
    int sopcast_purge = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER)->getIntOption();

    if (sopcast_refresh >= sopcast_purge) {
        if (sopcast_purge != 0)
            throw_std_runtime_error("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }
#endif

#ifdef ATRAILERS
    int atrailers_refresh = setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_REFRESH)->getIntOption();

    co = ConfigDefinition::findConfigSetup(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
    co->makeOption(fmt::to_string(atrailers_refresh), self);
#endif

    // read options that do not have special requirement and that are not yet loaded
    for (auto&& optionKey : ConfigOptionIterator()) {
        if (!options.at(optionKey) && !ConfigDefinition::isDependent(optionKey)) {
            setOption(root, optionKey);
        }
    }

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str());

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

            if (cs) {
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
                        std::map<std::string, std::string> arguments { { "status", cfgValue.status } };
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
    origValues.try_emplace(item, fmt::to_string(value));
}

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getOption();
}

int ConfigManager::getIntOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getIntOption();
}

bool ConfigManager::getBoolOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getBoolOption();
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getArrayOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getArrayOption();
}

std::shared_ptr<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getAutoscanListOption();
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getClientConfigListOption();
}

std::shared_ptr<DirectoryConfigList> ConfigManager::getDirectoryTweakOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDirectoryTweakOption();
}

std::shared_ptr<DynamicContentList> ConfigManager::getDynamicContentListOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDynamicContentListOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option) const
{
    auto optionValue = options.at(option);
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getTranscodingProfileListOption();
}

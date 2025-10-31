/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_manager.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file config/config_manager.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_manager.h" // API

#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/result/client_config.h"
#include "config/result/transcoding.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_path.h"
#include "config_definition.h"
#include "config_option_enum.h"
#include "config_options.h"
#include "config_setup.h"
#include "config_val.h"
#include "database/database.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <numeric>
#include <sstream>

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <langinfo.h>
#endif

ConfigManager::ConfigManager(
    std::shared_ptr<ConfigDefinition> definition,
    fs::path filename,
    const fs::path& userHome,
    const fs::path& configDir,
    fs::path dataDir,
    bool debug)
    : definition(std::move(definition))
    , filename(std::move(filename))
    , dataDir(std::move(dataDir))
{
    options = std::vector<std::shared_ptr<ConfigOption>>(to_underlying(ConfigVal::MAX));
    GrbLogger::Logger.setDebugLogging(debug);

    if (this->filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userHome / configDir;
        this->filename = home / DEFAULT_CONFIG_NAME;
    }
    this->filename = fs::weakly_canonical(this->filename);
    std::error_code ec;
    if (!isRegularFile(this->filename, ec)) {
        std::ostringstream expErrMsg;
        expErrMsg << "\nThe server configuration file could not be found: ";
        expErrMsg << this->filename.string() << "\n";
        expErrMsg << "Gerbera could not find a default configuration file.\n";
        expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
        expErrMsg << "For a list of options run: gerbera -h\n";

        throw std::runtime_error(expErrMsg.str());
    }
}

std::shared_ptr<Config> ConfigManager::getSelf()
{
    return shared_from_this();
}

std::shared_ptr<ConfigOption> ConfigManager::setOption(const pugi::xml_node& root, ConfigVal option, const std::map<std::string, std::string>* arguments)
{
    auto co = definition->findConfigSetup(option);
    auto self = getSelf();
    co->makeOption(root, self, arguments);
    log_debug("Config: option set: '{}' = '{}'", co->xpath, co->getCurrentValue());
    return co->getValue();
}

void ConfigManager::addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue)
{
    options.at(to_underlying(option)) = optionValue;
}

void ConfigManager::getAllNodes(const pugi::xml_node& parent, bool getAttributes)
{
    for (auto&& node : parent.children()) {
        if (node.type() == pugi::xml_node_type::node_element) {
            if (!node.first_child() || node.first_child().type() != pugi::xml_node_type::node_element)
                knownNodes[node.path()] = false;
            getAllNodes(node);
        }
    }
    if (getAttributes)
        for (pugi::xml_attribute attr : parent.attributes()) {
            knownNodes[fmt::format("{}/attribute::{}", parent.path(), attr.name())] = false;
        }
}

void ConfigManager::registerNode(const std::string& xmlPath)
{
    knownNodes[xmlPath] = true;
}

void ConfigManager::load(const fs::path& userHome)
{
    std::map<std::string, std::string> args;
    auto self = getSelf();

    log_info("Loading configuration from: {}", filename.string());
    pugi::xml_parse_result result = xmlDoc->load_file(filename.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }

    log_info("Parsing configuration...");

    auto root = xmlDoc->document_element();

    getAllNodes(root, false);

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
    auto co = definition->findConfigSetup(ConfigVal::SERVER_HOME);
    // respect command line if available; ignore xml value
    {
        std::string activeHome = userHome.string();
        bool homeOverride = setOption(root, ConfigVal::SERVER_HOME_OVERRIDE)->getBoolOption();
        if (userHome.empty() || homeOverride) {
            activeHome = co->getXmlContent(root, self);
        } else {
            if (!fs::is_directory(activeHome))
                throw_std_runtime_error("Directory '{}' does not exist", activeHome);
            log_info("Using home directory '{}'", activeHome);
        }
        co->makeOption(activeHome, self);
        ConfigPathSetup::Home = activeHome;
    }

    // checking database driver options
    [[maybe_unused]] bool mysqlEn = false;
    [[maybe_unused]] bool pgsqlEn = false;
    bool sqlite3En = false;

    co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE);
    co->getXmlElement(root); // fails if missing
    setOption(root, ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS);

    co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL);
    if (co->hasXmlElement(root)) {
        mysqlEn = setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED)->getBoolOption();
    }

    co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL);
    if (co->hasXmlElement(root)) {
        pgsqlEn = setOption(root, ConfigVal::SERVER_STORAGE_PGSQL_ENABLED)->getBoolOption();
    }

    co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE);
    if (co->hasXmlElement(root)) {
        sqlite3En = setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED)->getBoolOption();
    }

    std::string dbDriver;

#ifdef HAVE_MYSQL
    if (mysqlEn) {
        // read mysql options
        for (auto&& option : definition->getDependencies(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED))
            setOption(root, option);

        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "mysql.sql");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "mysql-upgrade.xml");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE);
        co->setDefaultValue(dataDir / "mysql-drop.sql");
        co->makeOption(root, self);
        dbDriver = DB_DRIVER_MYSQL;
    }
#endif // HAVE_MYSQL

#ifdef HAVE_PGSQL
    if (pgsqlEn) {
        // read pgsql options
        for (auto&& option : definition->getDependencies(ConfigVal::SERVER_STORAGE_PGSQL_ENABLED))
            setOption(root, option);

        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "postgres.sql");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "postgres-upgrade.xml");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE);
        co->setDefaultValue(dataDir / "postgres-drop.sql");
        co->makeOption(root, self);
        dbDriver = DB_DRIVER_POSTGRES;
    }
#endif // HAVE_PGSQL

    if (sqlite3En) {
        // read sqlite options
        for (auto&& option : definition->getDependencies(ConfigVal::SERVER_STORAGE_SQLITE_ENABLED))
            setOption(root, option);

        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "sqlite3.sql");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "sqlite3-upgrade.xml");
        co->makeOption(root, self);
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE);
        co->setDefaultValue(dataDir / "sqlite3-drop.sql");
        co->makeOption(root, self);
        dbDriver = DB_DRIVER_SQLITE;
    }

    co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, self);

    bool multiValue = setOption(root, ConfigVal::UPNP_MULTI_VALUES_ENABLED)->getBoolOption();
    co = definition->findConfigSetup(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE);
    co->setDefaultValue(multiValue ? YES : NO);

    bool clEn = setOption(root, ConfigVal::CLIENTS_LIST_ENABLED)->getBoolOption();
    args["isEnabled"] = clEn ? "true" : "false";
    setOption(root, ConfigVal::CLIENTS_LIST, &args);
    args.clear();

    setOption(root, ConfigVal::IMPORT_HIDDEN_FILES);
    setOption(root, ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    bool csens = setOption(root, ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE)->getBoolOption();
    args["tolower"] = fmt::to_string(!csens);
    setOption(root, ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, &args);
    args.clear();

    std::string defaultCharSet = DEFAULT_FILESYSTEM_CHARSET;
#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "")) {
        defaultCharSet = nl_langinfo(CODESET);
        log_debug("received {} from nl_langinfo", defaultCharSet);
    }
#endif
    // check if the one we take as default is actually available
    co = definition->findConfigSetup(ConfigVal::IMPORT_FILESYSTEM_CHARSET);
    try {
        auto conv = std::make_unique<StringConverter>(defaultCharSet, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error&) {
        defaultCharSet = DEFAULT_FALLBACK_CHARSET;
    }
    co->setDefaultValue(defaultCharSet);

    co = definition->findConfigSetup(ConfigVal::IMPORT_METADATA_CHARSET);
    co->setDefaultValue(defaultCharSet);

    co = definition->findConfigSetup(ConfigVal::IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(defaultCharSet);

    co = definition->findConfigSetup(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
    co->setDefaultValue(defaultCharSet);

#ifdef HAVE_JS
    // read javascript options
    co = definition->findConfigSetup(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER);
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR);
    co->makeOption(root, self);

    // read more javascript options
    co = definition->findConfigSetup(ConfigVal::IMPORT_SCRIPTING_CHARSET);
    co->setDefaultValue(defaultCharSet);
#endif

    args["hiddenFiles"] = getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES) ? "true" : "false";
    args["followSymlinks"] = getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS) ? "true" : "false";
    setOption(root, ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST, &args);

#ifdef HAVE_INOTIFY
    auto useInotify = setOption(root, ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)->getBoolOption();

    if (useInotify) {
        setOption(root, ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST, &args);
    } else {
        setOption({}, ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST); // set empty list
    }
#endif
    args.clear();

    // read transcoding options
    auto trEn = setOption(root, ConfigVal::TRANSCODING_TRANSCODING_ENABLED)->getBoolOption();
    args["isEnabled"] = trEn ? "true" : "false";
    setOption(root, ConfigVal::TRANSCODING_PROFILE_LIST, &args);
    args.clear();

#ifdef HAVE_CURL
    if (trEn) {
        for (auto&& option : definition->getDependencies(ConfigVal::TRANSCODING_TRANSCODING_ENABLED)) {
            setOption(root, option);
        }
    }
#endif // HAVE_CURL

    // read import options
    args["trim"] = "false";
    setOption(root, ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, ConfigVal::UPNP_SEARCH_SEPARATOR, &args);
    setOption(root, ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

#ifdef HAVE_FFMPEGTHUMBNAILER
    auto ffmpEn = setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)->getBoolOption();
    if (ffmpEn) {
        for (auto&& option : definition->getDependencies(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)) {
            setOption(root, option);
        }
    }
#endif

#ifdef HAVE_LASTFM
    auto lfmEn = setOption(root, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED)->getBoolOption();
    if (lfmEn) {
        for (auto&& option : definition->getDependencies(ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED)) {
            setOption(root, option);
        }
    }
#endif

    // read options that do not have special requirement and that are not yet loaded
    for (auto&& optionKey : ConfigOptionIterator()) {
        if (!options.at(to_underlying(optionKey)) && !definition->isDependent(optionKey)) {
            setOption(root, optionKey);
        }
    }

    log_info("Configuration load succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after loading: {}", buf.str());

    // now the XML is no longer needed we can destroy it
    xmlDoc = nullptr;
}

/// @brief Validate that correlated options have correct values
bool ConfigManager::validate()
{
    log_info("Validating configuration...");
    for (auto&& [path, found] : knownNodes) {
        if (!found)
            log_warning("Found extra config file entry '{}'", path);
    }
    auto self = getSelf();

    if (!getOption(ConfigVal::SERVER_NETWORK_INTERFACE).empty() && !getOption(ConfigVal::SERVER_IP).empty())
        throw_std_runtime_error("Error in config file: you can not specify interface and ip at the same time");

    // checking database driver options
    bool mysqlEn = getBoolOption(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED);
    bool pgsqlEn = getBoolOption(ConfigVal::SERVER_STORAGE_PGSQL_ENABLED);
    bool sqlite3En = getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_ENABLED);
    int dbCnt = 0;
    std::vector<std::string> dbList;
    if (mysqlEn) {
        dbCnt++;
        dbList.emplace_back(DB_DRIVER_MYSQL);
    }
    if (pgsqlEn) {
        dbCnt++;
        dbList.emplace_back(DB_DRIVER_POSTGRES);
    }
    if (sqlite3En) {
        dbCnt++;
        dbList.emplace_back(DB_DRIVER_SQLITE);
    }

    if (dbCnt > 1)
        throw_std_runtime_error("You enabled {} but "
                                "only one database driver may be active at a time",
            fmt::join(dbList, ", "));

    if (dbCnt == 0)
        throw_std_runtime_error("You disabled sqlite3, mysql and postgres but "
                                "one database driver must be active");

#ifndef HAVE_MYSQL
    if (mysqlEn) {
        throw_std_runtime_error("You enabled MySQL database in configuration, "
                                "however this version of Gerbera was compiled "
                                "without MySQL support!");
    }
#endif

#ifndef HAVE_PGSQL
    if (pgsqlEn) {
        throw_std_runtime_error("You enabled PostgreSQL database in configuration, "
                                "however this version of Gerbera was compiled "
                                "without PostgreSQL support!");
    }
#endif

    auto appendto = EnumOption<UrlAppendMode>::getEnumOption(self, ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO);
    if ((appendto == UrlAppendMode::ip || appendto == UrlAppendMode::port) && getOption(ConfigVal::SERVER_PRESENTATION_URL).empty()) {
        throw_std_runtime_error("Error in config file: \"append-to\" attribute "
                                "value in <presentationURL> tag is set to \"{}\""
                                "but no URL is specified",
            appendto);
    }

#ifdef HAVE_INOTIFY
    auto configTimedList = getAutoscanListOption(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST);
    auto configInotifyList = getAutoscanListOption(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST);

    for (const auto& iDir : configInotifyList) {
        for (const auto& tDir : configTimedList) {
            if (iDir->getLocation() == tDir->getLocation())
                throw_std_runtime_error("Error in config file: same path used in both inotify and timed scan modes");
        }
    }
#endif

    // now go through the optional settings and fix them if anything is missing
    auto defIpp = getIntOption(ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE);

    // now get the option list for the drop-down menu
    auto menuOpts = getArrayOption(ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN);
    if (std::find(menuOpts.begin(), menuOpts.end(), fmt::to_string(defIpp)) == menuOpts.end())
        throw_std_runtime_error("Error in config file: at least one <option> "
                                "under <items-per-page> must match the "
                                "<items-per-page default=\"\" /> attribute");

    bool markingEnabled = getBoolOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED);
    bool contentArrayEmpty = getArrayOption(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST).empty();
    if (markingEnabled && contentArrayEmpty) {
        throw_std_runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag");
    }

#ifndef HAVE_JS
    auto layoutType = EnumOption<LayoutType>::getEnumOption(self, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);

    if (layoutType == LayoutType::Js)
        throw_std_runtime_error("Gerbera was compiled without JS support, "
                                "however you specified \"js\" to be used for the "
                                "virtual-layout.");
#endif
    auto co = definition->findConfigSetup<ConfigBoxLayoutSetup>(ConfigVal::BOXLAYOUT_LIST);
    if (!co->validate(getSelf(), getBoxLayoutListOption(ConfigVal::BOXLAYOUT_LIST)))
        throw_std_runtime_error("Validation of {} failed", co->xpath);

    auto helpRoot = getOption(ConfigVal::SERVER_UI_DOCUMENTATION_USER);
    for (auto&& optionKey : ConfigOptionIterator()) {
        if (options.at(to_underlying(optionKey))) {
            auto cs = definition->findConfigSetup(optionKey);
            if (cs->isOptionDeprecated() && !cs->isDefaultValueUsed()) {
                log_warning("Option {} is deprecated, see {}/{} and update your config.xml ", cs->xpath, helpRoot, cs->getHelp());
            }
        }
    }

    log_info("Configuration check succeeded.");
    return true;
}

void ConfigManager::updateConfigFromDatabase(const std::shared_ptr<Database>& database)
{
    auto values = database->getConfigValues();
    auto self = getSelf();
    origValues.clear();
    log_info("Loading {} configuration items from database", values.size());

    for (auto&& cfgValue : values) {
        try {
            auto cs = definition->findConfigSetupByPath(cfgValue.key, true);

            if (cs) {
                if (cfgValue.item == cs->xpath) {
                    origValues[cfgValue.item] = cs->getCurrentValue();
                    cs->makeOption(cfgValue.value, self);
                } else {
                    std::string parValue = cfgValue.value;
                    auto statusList = splitString(cfgValue.status, ',');
                    if (statusList.back() == STATUS_CHANGED || statusList.back() == STATUS_UNCHANGED) {
                        if (!cs->updateDetail(cfgValue.item, parValue, self)) {
                            log_error("unhandled {} option {} != {}", cfgValue.status, cfgValue.item, cs->xpath);
                        }
                    } else if (statusList.back() == STATUS_REMOVED || statusList.back() == STATUS_ADDED || statusList.back() == STATUS_MANUAL) {
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

std::string ConfigManager::generateUDN(const std::shared_ptr<Database>& database)
{
    auto serverUDN = fmt::format("uuid:{}", generateRandomId());
    auto self = getSelf();
    auto cs = definition->findConfigSetup(ConfigVal::SERVER_UDN);
    cs->makeOption(serverUDN, self);
    database->updateConfigValue(cs->getUniquePath(), cs->getItemPath({}, {}), serverUDN, "added");
    log_info("Generated UDN '{}' and saved in database", serverUDN);

    return serverUDN;
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

void ConfigManager::setOrigValue(const std::string& item, IntOptionType value)
{
    origValues.try_emplace(item, fmt::to_string(value));
}

void ConfigManager::setOrigValue(const std::string& item, UIntOptionType value)
{
    origValues.try_emplace(item, fmt::to_string(value));
}

void ConfigManager::setOrigValue(const std::string& item, LongOptionType value)
{
    origValues.try_emplace(item, fmt::to_string(value));
}

void ConfigManager::setOrigValue(const std::string& item, ULongOptionType value)
{
    origValues.try_emplace(item, fmt::to_string(value));
}

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("string option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset string option {}='{}'", option, cs->getItemPathRoot());
    }
}

IntOptionType ConfigManager::getIntOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("int option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getIntOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset int option {}='{}'", option, cs->getItemPathRoot());
    }
}

UIntOptionType ConfigManager::getUIntOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("uint option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getUIntOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset uint option {}='{}'", option, cs->getItemPathRoot());
    }
}

LongOptionType ConfigManager::getLongOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("long option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getLongOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset long option {}='{}'", option, cs->getItemPathRoot());
    }
}

ULongOptionType ConfigManager::getULongOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("long option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getULongOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset long option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<ConfigOption> ConfigManager::getConfigOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("option {}='{}'not set", option, cs->getItemPathRoot());
        }
        return optionValue;
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset option option {}='{}'", option, cs->getItemPathRoot());
    }
}

bool ConfigManager::getBoolOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("bool option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getBoolOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset bool option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("dictionary option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getDictionaryOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset dictionary option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::vector<std::vector<std::pair<std::string, std::string>>> ConfigManager::getVectorOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("vector option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getVectorOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset vector option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::vector<std::string> ConfigManager::getArrayOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("array option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getArrayOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset array option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::vector<std::shared_ptr<AutoscanDirectory>> ConfigManager::getAutoscanListOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("autoscan option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getAutoscanListOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset autoscan option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<BoxLayoutList> ConfigManager::getBoxLayoutListOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("box layout option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getBoxLayoutListOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset box layout option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("client config option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getClientConfigListOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset client config option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<DirectoryConfigList> ConfigManager::getDirectoryTweakOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("directory tweak option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getDirectoryTweakOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset directory tweak option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<DynamicContentList> ConfigManager::getDynamicContentListOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("dynamic folder option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getDynamicContentListOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset dynamic folder option {}='{}'", option, cs->getItemPathRoot());
    }
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(ConfigVal option) const
{
    try {
        auto optionValue = options.at(to_underlying(option));
        if (!optionValue) {
            auto cs = definition->findConfigSetup(option);
            throw_std_runtime_error("transcoding option {}='{}' not set", option, cs->getItemPathRoot());
        }
        return optionValue->getTranscodingProfileListOption();
    } catch (const std::out_of_range& oor) {
        log_error("{}", oor.what());
        auto cs = definition->findConfigSetup(option);
        throw_std_runtime_error("unset transcoding option {}='{}'", option, cs->getItemPathRoot());
    }
}

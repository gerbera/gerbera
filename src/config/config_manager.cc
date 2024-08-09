/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_manager.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_manager.h" // API

#include "config/result/autoscan.h"
#include "config/result/client_config.h"
#include "config/result/transcoding.h"
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

ConfigManager::ConfigManager(fs::path filename,
    const fs::path& userHome, const fs::path& configDir,
    fs::path dataDir, bool debug)
    : filename(std::move(filename))
    , dataDir(std::move(dataDir))
{
    options = std::vector<std::shared_ptr<ConfigOption>>(to_underlying(ConfigVal::MAX));
    GrbLogger::Logger.setDebugLogging(debug);

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

std::shared_ptr<Config> ConfigManager::getSelf()
{
    return shared_from_this();
}

std::shared_ptr<ConfigOption> ConfigManager::setOption(const pugi::xml_node& root, ConfigVal option, const std::map<std::string, std::string>* arguments)
{
    auto co = ConfigDefinition::findConfigSetup(option);
    auto self = getSelf();
    co->makeOption(root, self, arguments);
    log_debug("Config: option set: '{}' = '{}'", co->xpath, co->getCurrentValue());
    return co->getValue();
}

void ConfigManager::addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue)
{
    options.at(to_underlying(option)) = optionValue;
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

    log_info("Parsing configuration...");

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
    auto co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_HOME);
    // respect command line if available; ignore xml value
    {
        std::string activeHome = userHome.string();
        bool homeOverride = setOption(root, ConfigVal::SERVER_HOME_OVERRIDE)->getBoolOption();
        if (userHome.empty() || homeOverride) {
            activeHome = co->getXmlContent(root);
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
    bool sqlite3En = false;

    co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE);
    co->getXmlElement(root); // fails if missing
    setOption(root, ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS);

    co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL);
    if (co->hasXmlElement(root)) {
        mysqlEn = setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_ENABLED)->getBoolOption();
    }

    co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE);
    if (co->hasXmlElement(root)) {
        sqlite3En = setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_ENABLED)->getBoolOption();
    }

    std::string dbDriver;

#ifdef HAVE_MYSQL
    if (mysqlEn) {
        // read mysql options
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_HOST);
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_DATABASE);
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_USERNAME);
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_PORT);
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_SOCKET);
        setOption(root, ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD);

        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "mysql.sql");
        co->makeOption(root, self);
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "mysql-upgrade.xml");
        co->makeOption(root, self);
        dbDriver = "mysql";
    }
#endif // HAVE_MYSQL

    if (sqlite3En) {
        // read sqlite options
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE);
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS);
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE);
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_RESTORE);
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED);
        setOption(root, ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);

        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(dataDir / "sqlite3.sql");
        co->makeOption(root, self);
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(dataDir / "sqlite3-upgrade.xml");
        co->makeOption(root, self);
        dbDriver = "sqlite3";
    }

    co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, self);

    bool multiValue = setOption(root, ConfigVal::UPNP_MULTI_VALUES_ENABLED)->getBoolOption();
    co = ConfigDefinition::findConfigSetup(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE);
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
    co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_FILESYSTEM_CHARSET);
    try {
        auto conv = std::make_unique<StringConverter>(defaultCharSet, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error&) {
        defaultCharSet = DEFAULT_FALLBACK_CHARSET;
    }
    co->setDefaultValue(defaultCharSet);

    co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_METADATA_CHARSET);
    co->setDefaultValue(defaultCharSet);

    co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(defaultCharSet);

    co = ConfigDefinition::findConfigSetup(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
    co->setDefaultValue(defaultCharSet);

#ifdef HAVE_JS
    // read javascript options
    co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER);
    co->setDefaultValue(dataDir / DEFAULT_JS_DIR);
    co->makeOption(root, self);

    // read more javascript options
    co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_SCRIPTING_CHARSET);
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
        setOption(root, ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);
        setOption(root, ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }
#endif // HAVE_CURL

    // read import options
    args["trim"] = "false";
    setOption(root, ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, ConfigVal::UPNP_SEARCH_SEPARATOR, &args);
    setOption(root, ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    auto ffmpEn = setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)->getBoolOption();
    if (ffmpEn) {
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
        setOption(root, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
    }
#endif

#if defined(HAVE_LASTFMLIB)
    auto lfmEn = setOption(root, ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED)->getBoolOption();
    if (lfmEn) {
        setOption(root, ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME);
        setOption(root, ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

    // read options that do not have special requirement and that are not yet loaded
    for (auto&& optionKey : ConfigOptionIterator()) {
        if (!options.at(to_underlying(optionKey)) && !ConfigDefinition::isDependent(optionKey)) {
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

/// \brief: Validate that correlated options have correct values
bool ConfigManager::validate()
{
    log_info("Validating configuration...");
    auto self = getSelf();

    if (!getOption(ConfigVal::SERVER_NETWORK_INTERFACE).empty() && !getOption(ConfigVal::SERVER_IP).empty())
        throw_std_runtime_error("Error in config file: you can not specify interface and ip at the same time");

    // checking database driver options
    bool mysqlEn = getBoolOption(ConfigVal::SERVER_STORAGE_MYSQL_ENABLED);
    bool sqlite3En = getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_ENABLED);

    if (sqlite3En && mysqlEn)
        throw_std_runtime_error("You enabled both, sqlite3 and mysql but "
                                "only one database driver may be active at a time");

    if (!sqlite3En && !mysqlEn)
        throw_std_runtime_error("You disabled both sqlite3 and mysql but "
                                "one database driver must be active");

#ifndef HAVE_MYSQL
    if (mysqlEn) {
        throw_std_runtime_error("You enabled MySQL database in configuration, "
                                "however this version of Gerbera was compiled "
                                "without MySQL support!");
    }
#endif // HAVE_MYSQL

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

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getOption();
}

IntOptionType ConfigManager::getIntOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getIntOption();
}

UIntOptionType ConfigManager::getUIntOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getUIntOption();
}

LongOptionType ConfigManager::getLongOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getLongOption();
}

std::shared_ptr<ConfigOption> ConfigManager::getConfigOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue;
}

bool ConfigManager::getBoolOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getBoolOption();
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDictionaryOption();
}

std::vector<std::vector<std::pair<std::string, std::string>>> ConfigManager::getVectorOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getVectorOption();
}

std::vector<std::string> ConfigManager::getArrayOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getArrayOption();
}

std::vector<std::shared_ptr<AutoscanDirectory>> ConfigManager::getAutoscanListOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getAutoscanListOption();
}

std::shared_ptr<BoxLayoutList> ConfigManager::getBoxLayoutListOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getBoxLayoutListOption();
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getClientConfigListOption();
}

std::shared_ptr<DirectoryConfigList> ConfigManager::getDirectoryTweakOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDirectoryTweakOption();
}

std::shared_ptr<DynamicContentList> ConfigManager::getDynamicContentListOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getDynamicContentListOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(ConfigVal option) const
{
    auto optionValue = options.at(to_underlying(option));
    if (!optionValue) {
        throw_std_runtime_error("option {} not set", option);
    }
    return optionValue->getTranscodingProfileListOption();
}

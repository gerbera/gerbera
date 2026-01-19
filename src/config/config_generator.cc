/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file config/config_generator.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_generator.h" // API

#include "config/config_definition.h"
#include "config/config_setup.h"
#include "config/result/box_layout.h"
#include "config/setup/config_setup_array.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_vector.h"
#include "config_val.h"
#include "util/tools.h"

#include <numeric>
#include <sstream>

#define XML_XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XMLNS "http://gerbera.io/config/"

using GeneratorSectionsIterator = EnumIterator<GeneratorSections, GeneratorSections::Server, GeneratorSections::All>;
using GeneratorModulesIterator = EnumIterator<GeneratorModules, GeneratorModules::Autoscan, GeneratorModules::None>;

std::shared_ptr<pugi::xml_node> ConfigGenerator::init()
{
    if (generated.find("") == generated.end()) {
        auto config = doc.append_child("config");
        generated[""] = std::make_shared<pugi::xml_node>(config);
    }
    return generated[""];
}

std::string ConfigGenerator::printSections(int section)
{
    std::vector<std::string> myFlags;

    for (auto [bit, label] : sections) {
        if (section & (1 << to_underlying(bit))) {
            myFlags.emplace_back(label);
            section &= ~(1 << to_underlying(bit));
        }
    }

    if (section) {
        myFlags.push_back(fmt::format("{:#04x}", section));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

int ConfigGenerator::makeSections(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|');
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ConfigGenerator::remapGeneratorSections(trimString(i)); });
}

std::map<GeneratorSections, std::string> ConfigGenerator::sections = {
    { GeneratorSections::Server, "Server" },
    { GeneratorSections::Ui, "Ui" },
    { GeneratorSections::ExtendedRuntime, "ExtendedRuntime" },
    { GeneratorSections::DynamicContainer, "DynamicContainer" },
    { GeneratorSections::Database, "Database" },
    { GeneratorSections::Import, "Import" },
    { GeneratorSections::Mappings, "Mappings" },
    { GeneratorSections::Boxlayout, "Boxlayout" },
    { GeneratorSections::Transcoding, "Transcoding" },
    { GeneratorSections::OnlineContent, "OnlineContent" },

    { GeneratorSections::All, "All" },
};

int ConfigGenerator::remapGeneratorSections(const std::string& arg)
{
    for (auto&& bit : GeneratorSectionsIterator()) {
        if (toLower(ConfigGenerator::sections[bit]) == toLower(arg)) {
            return 1 << to_underlying(bit);
        }
    }

    if (toLower(ConfigGenerator::sections[GeneratorSections::All]) == toLower(arg)) {
        int result = 0;
        for (auto&& bit : GeneratorSectionsIterator())
            result |= 1 << to_underlying(bit);
        return result;
    }
    return stoiString(arg, 0, 0);
}

int ConfigGenerator::makeModules(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|');
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ConfigGenerator::remapGeneratorModules(trimString(i)); });
}

std::map<GeneratorModules, std::string> ConfigGenerator::modules = {
    { GeneratorModules::Autoscan, "Autoscan" },
    { GeneratorModules::None, "None" },
};

int ConfigGenerator::remapGeneratorModules(const std::string& arg)
{
    for (auto&& bit : GeneratorModulesIterator()) {
        if (toLower(ConfigGenerator::modules[bit]) == toLower(arg)) {
            return 1 << to_underlying(bit);
        }
    }

    if (toLower(ConfigGenerator::modules[GeneratorModules::None]) == toLower(arg)) {
        return 0;
    }
    return stoiString(arg, 0, 0);
}

bool ConfigGenerator::isGenerated(GeneratorSections section) const
{
    return this->generateSections == 0 || (this->generateSections & (1 << to_underlying(section)));
}

bool ConfigGenerator::isGenerated(GeneratorModules module) const
{
    return (this->doModules & (1 << to_underlying(module)));
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::getNode(const std::string& tag) const
{
    log_debug("reading '{}' -> {}", tag, generated.find(tag) == generated.end());
    return generated.at(tag);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(
    const std::string& tag,
    const std::shared_ptr<ConfigSetup>& cs,
    const std::string& value,
    bool makeLastChild)
{
    auto split = splitString(tag, '/');
    std::shared_ptr<pugi::xml_node> result;
    if (!split.empty()) {
        std::string parent;
        std::string nodeKey;
        std::string attribute;

        for (auto&& part : split) {
            nodeKey += "/" + part;
            if (generated.find(nodeKey) == generated.end()) {
                if (startswith(part, ConfigDefinition::ATTRIBUTE)) {
                    attribute = part.substr(ConfigDefinition::ATTRIBUTE.size()); // last attribute gets the value
                } else {
                    auto newNode = generated[parent]->append_child(part.c_str());
                    generated[nodeKey] = std::make_shared<pugi::xml_node>(newNode);
                    result = generated[nodeKey]; // returns the last generated node even if there is an attribute
                    log_debug("{}: added node '{}' to '{}'", nodeKey, part, parent);
                    parent = nodeKey;
                }
            } else {
                if (makeLastChild && tag == nodeKey + "/") {
                    auto newNode = generated[parent]->append_child(part.c_str());
                    generated[nodeKey] = std::make_shared<pugi::xml_node>(newNode);
                    log_debug("{}: added multi-node '{}' to '{}'", nodeKey, part, parent);
                }
                result = generated[nodeKey]; // returns the last generated node even if there is an attribute
                parent = nodeKey;
            }
        }
        if (!cs || !cs->createNodeFromDefaults(generated[parent])) {
            if (tag.back() != '/') { // create entries without content
                if (attribute.empty()) {
                    log_debug("setting value '{}' to {}", value, nodeKey);
                    generated[nodeKey]->append_child(pugi::node_pcdata).set_value(value.c_str());
                } else {
                    log_debug("setting value '{}' to {}:{}", value, parent, attribute);
                    generated[parent]->append_attribute(attribute.c_str()) = value.c_str();
                }
            }
        }
    }
    return result;
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setXmlValue(
    const std::shared_ptr<pugi::xml_node>& parent,
    ConfigVal option,
    const std::string& value)
{
    auto cs = std::string(definition->mapConfigOption(option));
    if (startswith(cs, ConfigDefinition::ATTRIBUTE)) {
        cs = definition->removeAttribute(option);
        parent->append_attribute(cs.c_str()) = value.c_str();
    } else {
        parent->append_child(cs.c_str()).append_child(pugi::node_pcdata).set_value(value.c_str());
    }
    return parent;
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, const std::string& value)
{
    auto cs = definition->findConfigSetup(option);
    return setValue(cs->xpath, cs, value.empty() ? cs->getDefaultValue() : value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, ConfigVal attr, const std::string& value)
{
    auto cs = definition->findConfigSetup(option);
    return setValue(fmt::format("{}/{}", cs->xpath, definition->mapConfigOption(attr)), {}, value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, const std::string& key, const std::string& value)
{
    auto cs = std::dynamic_pointer_cast<ConfigDictionarySetup>(definition->findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = definition->mapConfigOption(cs->nodeOption);
    setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), {}, "", true);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, definition->ensureAttribute(cs->keyOption)), {}, key);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, definition->ensureAttribute(cs->valOption)), {}, value);
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setDictionary(ConfigVal option)
{
    auto cs = std::dynamic_pointer_cast<ConfigDictionarySetup>(definition->findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = definition->mapConfigOption(cs->nodeOption);
    for (auto&& [key, value] : cs->getXmlContent({}, nullptr)) {
        setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), {}, "", true);
        setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, definition->ensureAttribute(cs->keyOption)), {}, key);
        setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, definition->ensureAttribute(cs->valOption)), {}, value);
    }
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setVector(ConfigVal option)
{
    auto cs = std::dynamic_pointer_cast<ConfigVectorSetup>(definition->findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = definition->mapConfigOption(cs->nodeOption);
    for (auto&& value : cs->getXmlContent({}, nullptr)) {
        setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), {}, "", true);
        for (auto&& [key, val] : value)
            setValue(fmt::format("{}/{}/attribute::{}", cs->xpath, nodeKey, key), {}, val);
    }
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, ConfigVal dict, ConfigVal attr, const std::string& value)
{
    auto cs = definition->findConfigSetup(option);
    return setValue(fmt::format("{}/{}/{}", cs->xpath, definition->mapConfigOption(dict), definition->ensureAttribute(attr)), {}, value);
}

std::string ConfigGenerator::generate(
    const fs::path& userHome,
    const fs::path& configDir,
    const fs::path& dataDir,
    const fs::path& magicFile,
    const fs::path& customJsFolder)
{
    auto decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto config = init();

    config->append_attribute("version") = CONFIG_XML_VERSION;
    config->append_attribute("xmlns") = fmt::format("{}{}", XML_XMLNS, CONFIG_XML_VERSION).c_str();
    config->append_attribute("xmlns:xsi") = XML_XMLNS_XSI;
    config->append_attribute("xsi:schemaLocation") = fmt::format("{}{} {}{}.xsd", XML_XMLNS, CONFIG_XML_VERSION, XML_XMLNS, CONFIG_XML_VERSION).c_str();

    auto docinfo = config->append_child(pugi::node_comment);
    std::string headInfo = R"(
     See https://gerbera.io or read the docs for more
     information on creating and using config.xml configuration files.
     This file was generated by Gerbera )";
    headInfo += version + R"(
  )";
    docinfo.set_value(headInfo.c_str());

    generateServer(userHome, configDir, dataDir);
    generateImport(dataDir, userHome / configDir, magicFile, customJsFolder);
    generateTranscoding();

    std::ostringstream buf;
    doc.print(buf, "  ");
    return buf.str();
}

void ConfigGenerator::generateOptions(const std::vector<std::pair<ConfigVal, ConfigLevel>>& options)
{
    for (auto&& [opt, optLevel] : options) {
        if (optLevel <= level)
            setValue(opt);
    }
}

void ConfigGenerator::generateServerOptions(
    std::shared_ptr<pugi::xml_node>& server,
    const fs::path& userHome,
    const fs::path& configDir,
    const fs::path& dataDir)
{
    if (!isGenerated(GeneratorSections::Server))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::SERVER_NAME, ConfigLevel::Base },
        { ConfigVal::SERVER_UDN, ConfigLevel::Base },
        { ConfigVal::SERVER_HOME, ConfigLevel::Base },
        { ConfigVal::SERVER_WEBROOT, ConfigLevel::Base },
        { ConfigVal::SERVER_PORT, ConfigLevel::Example },
        { ConfigVal::SERVER_IP, ConfigLevel::Example },
        { ConfigVal::SERVER_NETWORK_INTERFACE, ConfigLevel::Example },
        { ConfigVal::SERVER_MANUFACTURER, ConfigLevel::Example },
        { ConfigVal::SERVER_MANUFACTURER_URL, ConfigLevel::Example },
        { ConfigVal::SERVER_MODEL_NAME, ConfigLevel::Example },
        { ConfigVal::SERVER_MODEL_DESCRIPTION, ConfigLevel::Example },
        { ConfigVal::SERVER_MODEL_NUMBER, ConfigLevel::Example },
        { ConfigVal::SERVER_MODEL_URL, ConfigLevel::Example },
        { ConfigVal::SERVER_SERIAL_NUMBER, ConfigLevel::Example },
        { ConfigVal::SERVER_PRESENTATION_URL, ConfigLevel::Example },
        { ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO, ConfigLevel::Example },
        { ConfigVal::SERVER_HOME_OVERRIDE, ConfigLevel::Example },
        { ConfigVal::SERVER_TMPDIR, ConfigLevel::Example },
        { ConfigVal::SERVER_HIDE_PC_DIRECTORY, ConfigLevel::Example },
        { ConfigVal::SERVER_HIDE_PC_DIRECTORY_WEB, ConfigLevel::Example },
        { ConfigVal::SERVER_BOOKMARK_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT, ConfigLevel::Example },
        { ConfigVal::VIRTUAL_URL, ConfigLevel::Example },
        { ConfigVal::EXTERNAL_URL, ConfigLevel::Example },
        { ConfigVal::UPNP_LITERAL_HOST_REDIRECTION, ConfigLevel::Example },
        { ConfigVal::UPNP_MULTI_VALUES_ENABLED, ConfigLevel::Example },
        { ConfigVal::UPNP_SEARCH_SEPARATOR, ConfigLevel::Example },
        { ConfigVal::UPNP_SEARCH_FILENAME, ConfigLevel::Example },
        { ConfigVal::UPNP_SEARCH_ITEM_SEGMENTS, ConfigLevel::Example },
        { ConfigVal::UPNP_SEARCH_CONTAINER_FLAG, ConfigLevel::Example },
        { ConfigVal::UPNP_ALBUM_PROPERTIES, ConfigLevel::Example },
        { ConfigVal::UPNP_ARTIST_PROPERTIES, ConfigLevel::Example },
        { ConfigVal::UPNP_GENRE_PROPERTIES, ConfigLevel::Example },
        { ConfigVal::UPNP_PLAYLIST_PROPERTIES, ConfigLevel::Example },
        { ConfigVal::UPNP_TITLE_PROPERTIES, ConfigLevel::Example },
        { ConfigVal::UPNP_ALBUM_NAMESPACES, ConfigLevel::Example },
        { ConfigVal::UPNP_ARTIST_NAMESPACES, ConfigLevel::Example },
        { ConfigVal::UPNP_GENRE_NAMESPACES, ConfigLevel::Example },
        { ConfigVal::UPNP_PLAYLIST_NAMESPACES, ConfigLevel::Example },
        { ConfigVal::UPNP_TITLE_NAMESPACES, ConfigLevel::Example },
        { ConfigVal::UPNP_RESOURCE_PROPERTY_DEFAULTS, ConfigLevel::Example },
        { ConfigVal::UPNP_OBJECT_PROPERTY_DEFAULTS, ConfigLevel::Example },
        { ConfigVal::UPNP_CONTAINER_PROPERTY_DEFAULTS, ConfigLevel::Example },
        { ConfigVal::UPNP_CAPTION_COUNT, ConfigLevel::Example },
#ifdef GRBDEBUG
        { ConfigVal::SERVER_LOG_DEBUG_MODE, ConfigLevel::Example },
#endif
    };

    generateUdn(false);

    {
        auto co = definition->findConfigSetup(ConfigVal::SERVER_HOME);
        co->setDefaultValue(userHome / configDir);
        co = definition->findConfigSetup(ConfigVal::SERVER_WEBROOT);
        co->setDefaultValue(dataDir / DEFAULT_WEB_DIR);
        co = definition->findConfigSetup(ConfigVal::SERVER_MODEL_NUMBER);
        co->setDefaultValue("42");
    }
    generateOptions(options);

    auto aliveinfo = server->append_child(pugi::node_comment);
    auto aliveText = fmt::format(R"(
        How frequently (in seconds) to send ssdp:alive advertisements.
        Minimum alive value accepted is: {}

        The advertisement will be sent every (A/2)-30 seconds,
        and will have a cache-control max-age of A where A is
        the value configured here. Ex: A value of 62 will result
        in an SSDP advertisement being sent every second.
    )",
        ALIVE_INTERVAL_MIN);
    aliveinfo.set_value(aliveText.c_str());
    setValue(ConfigVal::SERVER_ALIVE_INTERVAL);
}

void ConfigGenerator::generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir)
{
    auto server = setValue("/server/");
    generateUi();
    generateServerOptions(server, userHome, configDir, dataDir);
    generateDatabase(dataDir);
    generateDynamics();
    generateExtendedRuntime();
}

void ConfigGenerator::generateUi()
{
    if (!isGenerated(GeneratorSections::Ui))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::SERVER_UI_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_UI_SHOW_TOOLTIPS, ConfigLevel::Base },
        { ConfigVal::SERVER_UI_ACCOUNTS_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_UI_SESSION_TIMEOUT, ConfigLevel::Base },
        { ConfigVal::SERVER_UI_POLL_INTERVAL, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_POLL_WHEN_IDLE, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_ENABLE_NUMBERING, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_ENABLE_THUMBNAIL, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_ENABLE_VIDEO, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_EDIT_SORTKEY, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_FS_SUPPORT_ADD_ITEM, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_CONTENT_SECURITY_POLICY, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_MAPPING, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_EXTENSION_MIMETYPE_DEFAULT, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_DOCUMENTATION_SOURCE, ConfigLevel::Example },
        { ConfigVal::SERVER_UI_DOCUMENTATION_USER, ConfigLevel::Example },
    };
    generateOptions(options);
    setValue(ConfigVal::SERVER_UI_ACCOUNT_LIST, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER, DEFAULT_ACCOUNT_USER);
    setValue(ConfigVal::SERVER_UI_ACCOUNT_LIST, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD, DEFAULT_ACCOUNT_PASSWORD);
}

void ConfigGenerator::generateDynamics()
{
    if (!isGenerated(GeneratorSections::DynamicContainer))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST_ENABLED, ConfigLevel::Base },
    };
    generateOptions(options);

    setDictionary(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);

    auto&& containersTag = definition->mapConfigOption(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);
    auto&& containerTag = definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER);

    auto container = setValue(fmt::format("{}/{}/", containersTag, containerTag), {}, "", true);
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, "/LastAdded");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, "Recently Added");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_SORT, "-last_updated");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_updated > "@last7")");

    container = setValue(fmt::format("{}/{}/", containersTag, containerTag), {}, "", true);
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, "/LastModified");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, "Recently Modified");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_SORT, "-last_modified");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_modified > "@last7")");

    container = setValue(fmt::format("{}/{}/", containersTag, containerTag), {}, "", true);
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, "/LastPlayed");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, "Music Recently Played");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_SORT, "-upnp:lastPlaybackTime");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT, "MUSIC_LAST_PLAYED");
    setXmlValue(container, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item.audioItem" and upnp:lastPlaybackTime > "@last7")");
}

void ConfigGenerator::generateDatabase(const fs::path& prefixDir)
{
    if (!isGenerated(GeneratorSections::Database))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::SERVER_STORAGE_SQLITE_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SORT_KEY_ENABLED, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_STRING_LIMIT, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_RESTORE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, ConfigLevel::Base },
#ifdef HAVE_MYSQL
        { ConfigVal::SERVER_STORAGE_MYSQL_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_MYSQL_HOST, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_MYSQL_USERNAME, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_MYSQL_DATABASE, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_MYSQL_PORT, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_SOCKET, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_ENGINE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_CHARSET, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_MYSQL_COLLATION, ConfigLevel::Example },
#endif
#ifdef HAVE_PGSQL
        { ConfigVal::SERVER_STORAGE_PGSQL_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_PGSQL_HOST, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_PGSQL_USERNAME, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_PGSQL_DATABASE, ConfigLevel::Base },
        { ConfigVal::SERVER_STORAGE_PGSQL_PORT, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_PGSQL_PASSWORD, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE, ConfigLevel::Example },
        { ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE, ConfigLevel::Example },
#endif
    };
    {
        auto co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "sqlite3.sql");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "sqlite3-upgrade.xml");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE);
        co->setDefaultValue(prefixDir / "sqlite3-drop.sql");
    }

#ifdef HAVE_MYSQL
    {
        auto co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "mysql.sql");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "mysql-upgrade.xml");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE);
        co->setDefaultValue(prefixDir / "mysql-drop.sql");
    }
#endif

#ifdef HAVE_PGSQL
    {
        auto co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "postgres.sql");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "postgres-upgrade.xml");
        co = definition->findConfigSetup(ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE);
        co->setDefaultValue(prefixDir / "postgres-drop.sql");
    }
#endif

    generateOptions(options);
}

void ConfigGenerator::generateExtendedRuntime()
{
    if (!isGenerated(GeneratorSections::ExtendedRuntime))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
#ifdef HAVE_FFMPEGTHUMBNAILER
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, ConfigLevel::Base }, // clang does require additional indentation
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ROTATE, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, ConfigLevel::Example },
#endif
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, ConfigLevel::Base },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, ConfigLevel::Base },
#ifdef HAVE_LASTFM
        { ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD, ConfigLevel::Example },
#ifndef HAVE_LASTFMLIB
        { ConfigVal::SERVER_EXTOPTS_LASTFM_SESSIONKEY, ConfigLevel::Example },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_AUTHURL, ConfigLevel::Advanced },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_SCROBBLEURL, ConfigLevel::Advanced },
#endif
#endif
    };
    generateOptions(options);

    setValue(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
}

void ConfigGenerator::generateImportOptions(
    const fs::path& prefixDir,
    const fs::path& configDir,
    const fs::path& magicFile,
    const fs::path& customJsFolder)
{
    if (!isGenerated(GeneratorSections::Import))
        return;

    // Simple Import options
    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
#ifdef HAVE_MAGIC
        { ConfigVal::IMPORT_MAGIC_FILE, ConfigLevel::Base },
#endif
        { ConfigVal::IMPORT_HIDDEN_FILES, ConfigLevel::Base },
        { ConfigVal::IMPORT_FOLLOW_SYMLINKS, ConfigLevel::Example },
        { ConfigVal::IMPORT_DEFAULT_DATE, ConfigLevel::Example },
        { ConfigVal::IMPORT_LAYOUT_MODE, ConfigLevel::Example },
        { ConfigVal::IMPORT_NOMEDIA_FILE, ConfigLevel::Example },
        { ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS, ConfigLevel::Example },
        { ConfigVal::IMPORT_FILESYSTEM_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_METADATA_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_PLAYLIST_CHARSET, ConfigLevel::Example },
#ifdef HAVE_JS
        { ConfigVal::IMPORT_SCRIPTING_CHARSET, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_SCAN_MODE, ConfigLevel::Example },
        { ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE, ConfigLevel::Base },
#ifdef ONLINESERVICES
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER, ConfigLevel::Base },
#endif
        { ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER, customJsFolder.empty() ? ConfigLevel::Example : ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS, ConfigLevel::Example },
        { ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS, ConfigLevel::Example },
        { ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR, ConfigLevel::Example },
#endif // HAVE_JS
#ifdef HAVE_LIBEXIF
        { ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_ENABLED, ConfigLevel::Example },
#endif
#ifdef HAVE_EXIV2
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_ENABLED, ConfigLevel::Example },
#endif
#ifdef HAVE_TAGLIB
        { ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_ENABLED, ConfigLevel::Example },
#endif
#ifdef HAVE_FFMPEG
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_ARTWORK_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_SUBTITLE_SEEK_SIZE, ConfigLevel::Example },
#endif
#ifdef HAVE_MATROSKA
        { ConfigVal::IMPORT_LIBOPTS_MKV_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_MKV_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_MKV_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_MKV_COMMENT_LIST, ConfigLevel::Example },
#endif
#ifdef HAVE_WAVPACK
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_ENABLED, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST, ConfigLevel::Example },
#endif
        { ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE, ConfigLevel::Base },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP, ConfigLevel::Example },
        { ConfigVal::IMPORT_SYSTEM_DIRECTORIES, ConfigLevel::Example },
        { ConfigVal::IMPORT_VISIBLE_DIRECTORIES, ConfigLevel::Example },
        { ConfigVal::IMPORT_READABLE_NAMES, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_ORDER, ConfigLevel::Example },
        { ConfigVal::IMPORT_LAYOUT_PARENT_PATH, ConfigLevel::Example },
        { ConfigVal::IMPORT_LAYOUT_MAPPING, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP, ConfigLevel::Example },
        { ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, ConfigLevel::Example },
        { ConfigVal::IMPORT_DIRECTORIES_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST, ConfigLevel::Advanced },
        { ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST, ConfigLevel::Advanced },
        { ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST, ConfigLevel::Advanced },
        { ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST, ConfigLevel::Advanced },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST, ConfigLevel::Advanced },
        { ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT, ConfigLevel::Example },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_MINDEPTH, ConfigLevel::Example },
#ifdef HAVE_INOTIFY
        { ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY, ConfigLevel::Example },
#endif
    };

    if (level > ConfigLevel::Example) {
        /// @brief default values for ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST
        static const std::vector<std::string> defaultFanArtFile {
            "%title%.jpg",
            "%filename%.jpg",
            "folder.jpg",
            "poster.jpg",
            "cover.jpg",
            "albumartsmall.jpg",
            "%album%.jpg",
        };
        auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST);
        cs->setDefaultValue(defaultFanArtFile);
    }
    if (level > ConfigLevel::Example) {
        /// @brief default values for ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST
        static const std::vector<std::string> defaultContainerArtFile {
            "folder.jpg",
            "poster.jpg",
            "cover.jpg",
            "albumartsmall.jpg",
        };
        auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST);
        cs->setDefaultValue(defaultContainerArtFile);
    }
    if (level > ConfigLevel::Example) {
        /// @brief default values for ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST
        static const std::vector<std::string> defaultSubtitleFile {
            "%title%.srt",
            "%filename%.srt"
        };
        auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
        cs->setDefaultValue(defaultSubtitleFile);
    }
    if (level > ConfigLevel::Example) {
        /// @brief default values for ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST
        static const std::vector<std::string> defaultResourceFile {
            "%filename%.srt",
            "cover.jpg",
            "%album%.jpg",
        };
        auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST);
        cs->setDefaultValue(defaultResourceFile);
    }
    if (level > ConfigLevel::Example) {
        /// @brief default values for ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST
        static const std::vector<std::string> defaultMetadataFile {
            "%filename%.nfo"
        };
        auto cs = definition->findConfigSetup<ConfigArraySetup>(ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST);
        cs->setDefaultValue(defaultMetadataFile);
    }

#ifdef HAVE_JS
    // Set Script Folders
    {
        std::string scriptDir;
        scriptDir = prefixDir / DEFAULT_JS_DIR;
        auto co = definition->findConfigSetup(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER);
        co->setDefaultValue(scriptDir);
    }
    {
        std::string scriptDir = customJsFolder;
        if (customJsFolder.empty())
            scriptDir = configDir / DEFAULT_JS_DIR;
        auto co = definition->findConfigSetup(ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER);
        co->setDefaultValue(scriptDir);
    }
#endif

#ifdef HAVE_MAGIC
    if (!magicFile.empty()) {
        auto co = definition->findConfigSetup(ConfigVal::IMPORT_MAGIC_FILE);
        co->setDefaultValue(magicFile);
    }
#endif

    if (isGenerated(GeneratorModules::Autoscan)) {
        auto&& autoscanTag = definition->mapConfigOption(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST);
        auto as = setValue(fmt::format("{}/", autoscanTag), {}, "", true);
        setXmlValue(as, ConfigVal::A_LOAD_SECTION_FROM_FILE, "autoscan.xml");
    }

    if (level > ConfigLevel::Base) {
        // Generate Autoscan Example
        auto&& directoryTag = definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY);
#ifdef HAVE_INOTIFY
        auto&& autoscanTag = definition->mapConfigOption(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), {}, "", true);
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, "inotify");
#else
        auto&& autoscanTag = definition->mapConfigOption(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), {}, "", true);
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, "timed");
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, "1000");
#endif
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, "yes");
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, "yes");
        setXmlValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, "Any");
    }

    {
        // Generate Charsets
        auto co = definition->findConfigSetup(ConfigVal::IMPORT_FILESYSTEM_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = definition->findConfigSetup(ConfigVal::IMPORT_METADATA_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = definition->findConfigSetup(ConfigVal::IMPORT_PLAYLIST_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#ifdef HAVE_LIBEXIF
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_EXIV2
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_TAGLIB
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_FFMPEG
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_MATROSKA
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_WAVPACK
        co = definition->findConfigSetup(ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
    }
    generateOptions(options);
}

void ConfigGenerator::generateImport(
    const fs::path& prefixDir,
    const fs::path& configDir,
    const fs::path& magicFile,
    const fs::path& customJsFolder)
{
    generateImportOptions(prefixDir, configDir, magicFile, customJsFolder);
    generateMappings();
    generateBoxlayout(ConfigVal::BOXLAYOUT_LIST);
#ifdef ONLINE_SERVICES
    generateOnlineContent();
#endif
}

void ConfigGenerator::generateMappings()
{
    if (!isGenerated(GeneratorSections::Mappings))
        return;

    auto ext2mt = setValue(ConfigVal::IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    setDictionary(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for PS3 divx support ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/divx" /> )");

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/avi" /> )");

    setDictionary(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    setDictionary(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    setDictionary(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST);
    setVector(ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST);

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE, ConfigLevel::Example },
        { ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS, ConfigLevel::Example },
    };
    generateOptions(options);
}

void ConfigGenerator::generateBoxlayout(ConfigVal option)
{
    if (!isGenerated(GeneratorSections::Boxlayout))
        return;

    auto cs = definition->findConfigSetup<ConfigBoxLayoutSetup>(option);

    const auto boxListTag = definition->mapConfigOption(option);
    const auto boxTag = definition->mapConfigOption(ConfigVal::A_BOXLAYOUT_BOX);

    for (auto&& bl : cs->getDefault()) {
        auto box = setValue(fmt::format("{}/{}/", boxListTag, boxTag), {}, "", true);
        setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_KEY, bl.getKey());
        setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_TITLE, bl.getTitle());
        setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_CLASS, bl.getClass());
        if (!bl.getUpnpShortcut().empty())
            setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT, bl.getUpnpShortcut());
        if (!bl.getSortKey().empty())
            setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY, bl.getSortKey());
        if (bl.getSize() != 1)
            setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_SIZE, fmt::to_string(bl.getSize()));
        if (!bl.getEnabled())
            setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, fmt::to_string(bl.getEnabled()));
        if (!bl.getSearchable())
            setXmlValue(box, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, fmt::to_string(bl.getSearchable()));
    }
}

void ConfigGenerator::generateOnlineContent()
{
    if (!isGenerated(GeneratorSections::OnlineContent))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {};
    generateOptions(options);

#ifdef ONLINE_SERVICES
    setValue("/import/online-content/");
#endif
}

void ConfigGenerator::generateTranscoding()
{
    if (!isGenerated(GeneratorSections::Transcoding))
        return;

    auto options = std::vector<std::pair<ConfigVal, ConfigLevel>> {
        { ConfigVal::TRANSCODING_TRANSCODING_ENABLED, ConfigLevel::Base },
        { ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED, ConfigLevel::Example },
        { ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED, ConfigLevel::Example },
#ifdef HAVE_CURL
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, ConfigLevel::Example },
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE, ConfigLevel::Example },
#endif // HAVE_CURL
    };
    generateOptions(options);
    setDictionary(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP);

    const auto profileTag = definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE);

    auto oggmp3 = setValue(fmt::format("{}/", profileTag), {}, "", true);
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME, "ogg2mp3");
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "audio/mpeg");
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, NO);
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setXmlValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG, NO);

    auto agent = setValue(fmt::format("{}/{}/", profileTag, definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)), {}, "", true);
    setXmlValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "ffmpeg");
    setXmlValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-y -i %in -f mp3 %out");

    auto buffer = setValue(fmt::format("{}/{}/", profileTag, definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)), {}, "", true);
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_AUDIO_BUFFER_SIZE));
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_AUDIO_CHUNK_SIZE));
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_AUDIO_FILL_SIZE));

    auto vlcmpeg = setValue(fmt::format("{}/", profileTag), {}, "", true);
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME, "vlcmpeg");
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "video/mpeg");
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, YES);
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG, YES);

    if (level > ConfigLevel::Base) {
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_SAMPFREQ, GRB_STRINGIZE(SOURCE));
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NRCHAN, GRB_STRINGIZE(SOURCE));
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_THUMB, NO);
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_HIDEORIG, NO);
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NOTRANSCODING, "");
        setXmlValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_DLNAPROF, "MP4");
        auto fourCc = setValue(fmt::format("{}/{}/", profileTag, definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC)), {}, "", true);
        setXmlValue(fourCc, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE, "Ignore");
        setXmlValue(fourCc, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, "XVID");
        setXmlValue(fourCc, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, "DX50");
    }

    agent = setValue(fmt::format("{}/{}/", profileTag, definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)), {}, "", true);
    setXmlValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "vlc");
    setXmlValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc://quit");

    buffer = setValue(fmt::format("{}/{}/", profileTag, definition->mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)), {}, "", true);
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
    setXmlValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_VIDEO_FILL_SIZE));
}

void ConfigGenerator::generateUdn(bool doExport)
{
    auto co = definition->findConfigSetup(ConfigVal::SERVER_UDN);
    co->setDefaultValue(fmt::format("uuid:{}", generateRandomId()));
    if (doExport)
        setValue(ConfigVal::SERVER_UDN);
}

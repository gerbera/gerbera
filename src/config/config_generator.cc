/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file config_generator.cc

#include "config_generator.h"

#include <sstream>

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_setup.h"
#include "config/result/box_layout.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_vector.h"
#include "content/autoscan.h"
#include "util/grb_time.h"
#include "util/tools.h"

#define XML_XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XMLNS "http://mediatomb.cc/config/"

std::shared_ptr<pugi::xml_node> ConfigGenerator::init()
{
    if (generated.find("") == generated.end()) {
        auto config = doc.append_child("config");
        generated[""] = std::make_shared<pugi::xml_node>(config);
    }
    return generated[""];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::getNode(const std::string& tag) const
{
    log_debug("reading '{}' -> {}", tag, generated.find(tag) == generated.end());
    return generated.at(tag);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(const std::string& tag, const std::string& value, bool makeLastChild)
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
    return result;
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, const std::string& value)
{
    auto cs = ConfigDefinition::findConfigSetup(option);
    return setValue(cs->xpath, value.empty() ? cs->getDefaultValue() : value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(const std::shared_ptr<pugi::xml_node>& parent, config_option_t option, const std::string& value)
{
    auto cs = std::string(ConfigDefinition::mapConfigOption(option));
    if (startswith(cs, ConfigDefinition::ATTRIBUTE)) {
        cs = ConfigDefinition::removeAttribute(option);
        parent->append_attribute(cs.c_str()) = value.c_str();
    } else {
        parent->append_child(cs.c_str()).append_child(pugi::node_pcdata).set_value(value.c_str());
    }
    return parent;
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, config_option_t attr, const std::string& value)
{
    auto cs = ConfigDefinition::findConfigSetup(option);
    return setValue(fmt::format("{}/{}", cs->xpath, ConfigDefinition::mapConfigOption(attr)), value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, const std::string& key, const std::string& value)
{
    auto cs = std::dynamic_pointer_cast<ConfigDictionarySetup>(ConfigDefinition::findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = ConfigDefinition::mapConfigOption(cs->nodeOption);
    setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), "", true);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigDefinition::ensureAttribute(cs->keyOption)), key);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigDefinition::ensureAttribute(cs->valOption)), value);
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setDictionary(config_option_t option)
{
    auto cs = std::dynamic_pointer_cast<ConfigDictionarySetup>(ConfigDefinition::findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = ConfigDefinition::mapConfigOption(cs->nodeOption);
    for (auto&& [key, value] : cs->getXmlContent({})) {
        setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), "", true);
        setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigDefinition::ensureAttribute(cs->keyOption)), key);
        setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigDefinition::ensureAttribute(cs->valOption)), value);
    }
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setVector(config_option_t option)
{
    auto cs = std::dynamic_pointer_cast<ConfigVectorSetup>(ConfigDefinition::findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = ConfigDefinition::mapConfigOption(cs->nodeOption);
    for (auto&& value : cs->getXmlContent({})) {
        setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), "", true);
        std::size_t index = 0;
        for (auto&& [key, val] : value) {
            setValue(fmt::format("{}/{}/attribute::{}", cs->xpath, nodeKey, key), val);
            ++index;
        }
    }
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, config_option_t dict, config_option_t attr, const std::string& value)
{
    auto cs = ConfigDefinition::findConfigSetup(option);
    return setValue(fmt::format("{}/{}/{}", cs->xpath, ConfigDefinition::mapConfigOption(dict), ConfigDefinition::ensureAttribute(attr)), value);
}

std::string ConfigGenerator::generate(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir, const fs::path& magicFile)
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
    docinfo.set_value(R"(
     See https://gerbera.io or read the docs for more
     information on creating and using config.xml configuration files.
    )");

    generateServer(userHome, configDir, dataDir);
    generateImport(dataDir, userHome / configDir, magicFile);
    generateTranscoding();

    std::ostringstream buf;
    doc.print(buf, "  ");
    return buf.str();
}

void ConfigGenerator::generateOptions(const std::vector<std::pair<config_option_t, bool>>& options)
{
    for (auto&& [opt, isDefault] : options) {
        if (isDefault || example)
            setValue(opt);
    }
}

void ConfigGenerator::generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir)
{
    auto server = setValue("/server/");
    generateUi();

    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_SERVER_NAME, true },
        { CFG_SERVER_UDN, true },
        { CFG_SERVER_HOME, true },
        { CFG_SERVER_WEBROOT, true },
        { CFG_SERVER_PORT, false },
        { CFG_SERVER_IP, false },
        { CFG_SERVER_NETWORK_INTERFACE, false },
        { CFG_SERVER_MANUFACTURER, false },
        { CFG_SERVER_MANUFACTURER_URL, false },
        { CFG_SERVER_MODEL_NAME, false },
        { CFG_SERVER_MODEL_DESCRIPTION, false },
        { CFG_SERVER_MODEL_NUMBER, false },
        { CFG_SERVER_MODEL_URL, false },
        { CFG_SERVER_SERIAL_NUMBER, false },
        { CFG_SERVER_PRESENTATION_URL, false },
        { CFG_SERVER_APPEND_PRESENTATION_URL_TO, false },
        { CFG_SERVER_HOME_OVERRIDE, false },
        { CFG_SERVER_TMPDIR, false },
        { CFG_SERVER_HIDE_PC_DIRECTORY, false },
        { CFG_SERVER_BOOKMARK_FILE, false },
        { CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT, false },
        { CFG_VIRTUAL_URL, false },
        { CFG_EXTERNAL_URL, false },
        { CFG_UPNP_LITERAL_HOST_REDIRECTION, false },
        { CFG_UPNP_MULTI_VALUES_ENABLED, false },
        { CFG_UPNP_SEARCH_SEPARATOR, false },
        { CFG_UPNP_SEARCH_FILENAME, false },
        { CFG_UPNP_SEARCH_ITEM_SEGMENTS, false },
        { CFG_UPNP_SEARCH_CONTAINER_FLAG, false },
        { CFG_UPNP_ALBUM_PROPERTIES, false },
        { CFG_UPNP_ARTIST_PROPERTIES, false },
        { CFG_UPNP_GENRE_PROPERTIES, false },
        { CFG_UPNP_PLAYLIST_PROPERTIES, false },
        { CFG_UPNP_TITLE_PROPERTIES, false },
        { CFG_UPNP_ALBUM_NAMESPACES, false },
        { CFG_UPNP_ARTIST_NAMESPACES, false },
        { CFG_UPNP_GENRE_NAMESPACES, false },
        { CFG_UPNP_PLAYLIST_NAMESPACES, false },
        { CFG_UPNP_TITLE_NAMESPACES, false },
        { CFG_UPNP_CAPTION_COUNT, false },
#ifdef GRBDEBUG
        { CFG_SERVER_DEBUG_MODE, false },
#endif
    };

    generateUdn(false);

    {
        auto co = ConfigDefinition::findConfigSetup(CFG_SERVER_HOME);
        co->setDefaultValue(userHome / configDir);
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_WEBROOT);
        co->setDefaultValue(dataDir / DEFAULT_WEB_DIR);
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_MODEL_NUMBER);
        co->setDefaultValue("42");
    }
    generateOptions(options);

    auto aliveinfo = server->append_child(pugi::node_comment);
    aliveinfo.set_value(fmt::format(R"(
        How frequently (in seconds) to send ssdp:alive advertisements.
        Minimum alive value accepted is: {}

        The advertisement will be sent every (A/2)-30 seconds,
        and will have a cache-control max-age of A where A is
        the value configured here. Ex: A value of 62 will result
        in an SSDP advertisement being sent every second.
    )",
        ALIVE_INTERVAL_MIN)
                            .c_str());
    setValue(CFG_SERVER_ALIVE_INTERVAL);

    generateDatabase(dataDir);
    generateDynamics();
    generateExtendedRuntime();
}

void ConfigGenerator::generateUi()
{
    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_SERVER_UI_ENABLED, true },
        { CFG_SERVER_UI_SHOW_TOOLTIPS, true },
        { CFG_SERVER_UI_ACCOUNTS_ENABLED, true },
        { CFG_SERVER_UI_SESSION_TIMEOUT, true },
        { CFG_SERVER_UI_POLL_INTERVAL, false },
        { CFG_SERVER_UI_POLL_WHEN_IDLE, false },
        { CFG_SERVER_UI_ENABLE_NUMBERING, false },
        { CFG_SERVER_UI_ENABLE_THUMBNAIL, false },
        { CFG_SERVER_UI_ENABLE_VIDEO, false },
        { CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE, false },
        { CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, false },
    };
    generateOptions(options);
    setValue(CFG_SERVER_UI_ACCOUNT_LIST, ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, DEFAULT_ACCOUNT_USER);
    setValue(CFG_SERVER_UI_ACCOUNT_LIST, ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD, DEFAULT_ACCOUNT_PASSWORD);
}

void ConfigGenerator::generateDynamics()
{
    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_SERVER_DYNAMIC_CONTENT_LIST_ENABLED, true },
    };
    generateOptions(options);

    setDictionary(CFG_SERVER_DYNAMIC_CONTENT_LIST);

    auto&& containersTag = ConfigDefinition::mapConfigOption(CFG_SERVER_DYNAMIC_CONTENT_LIST);
    auto&& containerTag = ConfigDefinition::mapConfigOption(ATTR_DYNAMIC_CONTAINER);

    auto container = setValue(fmt::format("{}/{}/", containersTag, containerTag), "", true);
    setValue(container, ATTR_DYNAMIC_CONTAINER_LOCATION, "/LastAdded");
    setValue(container, ATTR_DYNAMIC_CONTAINER_TITLE, "Recently Added");
    setValue(container, ATTR_DYNAMIC_CONTAINER_SORT, "-last_updated");
    setValue(container, ATTR_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_updated > "@last7")");

    container = setValue(fmt::format("{}/{}/", containersTag, containerTag), "", true);
    setValue(container, ATTR_DYNAMIC_CONTAINER_LOCATION, "/LastModified");
    setValue(container, ATTR_DYNAMIC_CONTAINER_TITLE, "Recently Modified");
    setValue(container, ATTR_DYNAMIC_CONTAINER_SORT, "-last_modified");
    setValue(container, ATTR_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_modified > "@last7")");
}

void ConfigGenerator::generateDatabase(const fs::path& prefixDir)
{
    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_SERVER_STORAGE_SQLITE_ENABLED, true },
        { CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE, true },
        { CFG_SERVER_STORAGE_USE_TRANSACTIONS, false },
        { CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS, false },
        { CFG_SERVER_STORAGE_SQLITE_JOURNALMODE, false },
        { CFG_SERVER_STORAGE_SQLITE_RESTORE, false },
        { CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE, false },
        { CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE, false },
#ifdef SQLITE_BACKUP_ENABLED
        { CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED, true },
        { CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, true },
#endif
#ifdef HAVE_MYSQL
        { CFG_SERVER_STORAGE_MYSQL_ENABLED, true },
        { CFG_SERVER_STORAGE_MYSQL_HOST, true },
        { CFG_SERVER_STORAGE_MYSQL_USERNAME, true },
        { CFG_SERVER_STORAGE_MYSQL_DATABASE, true },
        { CFG_SERVER_STORAGE_MYSQL_PORT, false },
        { CFG_SERVER_STORAGE_MYSQL_SOCKET, false },
        { CFG_SERVER_STORAGE_MYSQL_PASSWORD, false },
        { CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE, false },
        { CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE, false },
#endif
    };
    {
        auto co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "sqlite3.sql");
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "sqlite3-upgrade.xml");
    }

#ifdef HAVE_MYSQL
    {
        auto co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "mysql.sql");
        co = ConfigDefinition::findConfigSetup(CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "mysql-upgrade.xml");
    }
#endif

    generateOptions(options);
}

void ConfigGenerator::generateExtendedRuntime()
{
    auto options = std::vector<std::pair<config_option_t, bool>>
    {
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
        { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, true }, // clang does require additional indentation
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, true },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, true },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, true },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, true },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED, false },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED, false },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, false },
            { CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, false },
#endif
            { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED, true },
            { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES, true },
            { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, true },
            { CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, true },
#ifdef HAVE_LASTFMLIB
            { CFG_SERVER_EXTOPTS_LASTFM_ENABLED, false },
            { CFG_SERVER_EXTOPTS_LASTFM_USERNAME, false },
            { CFG_SERVER_EXTOPTS_LASTFM_PASSWORD, false },
#endif
    };
    generateOptions(options);

    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
}

void ConfigGenerator::generateImport(const fs::path& prefixDir, const fs::path& configDir, const fs::path& magicFile)
{
    // Simple Import options
    auto options = std::vector<std::pair<config_option_t, bool>> {
#ifdef HAVE_MAGIC
        { CFG_IMPORT_MAGIC_FILE, true },
#endif
        { CFG_IMPORT_HIDDEN_FILES, true },
        { CFG_IMPORT_FOLLOW_SYMLINKS, false },
        { CFG_IMPORT_DEFAULT_DATE, false },
        { CFG_IMPORT_LAYOUT_MODE, false },
        { CFG_IMPORT_NOMEDIA_FILE, false },
        { CFG_IMPORT_VIRTUAL_DIRECTORY_KEYS, false },
        { CFG_IMPORT_FILESYSTEM_CHARSET, false },
        { CFG_IMPORT_METADATA_CHARSET, false },
        { CFG_IMPORT_PLAYLIST_CHARSET, false },
#ifdef HAVE_JS
        { CFG_IMPORT_SCRIPTING_CHARSET, true },
        { CFG_IMPORT_SCRIPTING_COMMON_FOLDER, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER, true },
        { CFG_IMPORT_SCRIPTING_CUSTOM_FOLDER, false },
        { CFG_IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS, false },
        { CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS, false },
        { CFG_IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR, false },
#endif // HAVE_JS
#ifdef HAVE_LIBEXIF
        { CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_EXIF_CHARSET, false },
#endif
#ifdef HAVE_EXIV2
        { CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_EXIV2_CHARSET, false },
#endif
#ifdef HAVE_TAGLIB
        { CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_ID3_CHARSET, false },
#endif
#ifdef HAVE_FFMPEG
        { CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST, false },
        { CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET, false },
#endif
        { CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE, true },
        { CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP, false },
        { CFG_IMPORT_SYSTEM_DIRECTORIES, false },
        { CFG_IMPORT_VISIBLE_DIRECTORIES, false },
        { CFG_IMPORT_READABLE_NAMES, false },
        { CFG_IMPORT_RESOURCES_ORDER, false },
        { CFG_IMPORT_LAYOUT_PARENT_PATH, false },
        { CFG_IMPORT_LAYOUT_MAPPING, false },
        { CFG_IMPORT_LIBOPTS_ENTRY_SEP, false },
        { CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, false },
        { CFG_IMPORT_DIRECTORIES_LIST, false },
        { CFG_IMPORT_RESOURCES_CASE_SENSITIVE, false },
        { CFG_IMPORT_RESOURCES_FANART_FILE_LIST, false },
        { CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, false },
        { CFG_IMPORT_RESOURCES_METAFILE_FILE_LIST, false },
        { CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, false },
        { CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST, false },
        { CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION, false },
        { CFG_IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT, false },
        { CFG_IMPORT_RESOURCES_CONTAINERART_MINDEPTH, false },
        { CFG_IMPORT_RESOURCES_FANART_DIR_LIST, false },
        { CFG_IMPORT_RESOURCES_SUBTITLE_DIR_LIST, false },
        { CFG_IMPORT_RESOURCES_METAFILE_DIR_LIST, false },
        { CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST, false },
        { CFG_IMPORT_RESOURCES_CONTAINERART_DIR_LIST, false },
#ifdef HAVE_INOTIFY
        { CFG_IMPORT_AUTOSCAN_USE_INOTIFY, false },
#endif
    };

#ifdef HAVE_JS
    // Set Script Folders
    {
        std::string scriptDir;
        scriptDir = prefixDir / DEFAULT_JS_DIR;
        auto co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_COMMON_FOLDER);
        co->setDefaultValue(scriptDir);
    }
    {
        std::string scriptDir;
        scriptDir = configDir / DEFAULT_JS_DIR;
        auto co = ConfigDefinition::findConfigSetup(CFG_IMPORT_SCRIPTING_CUSTOM_FOLDER);
        co->setDefaultValue(scriptDir);
    }
#endif

#ifdef HAVE_MAGIC
    if (!magicFile.empty()) {
        auto co = ConfigDefinition::findConfigSetup(CFG_IMPORT_MAGIC_FILE);
        co->setDefaultValue(magicFile);
    }
#endif
    if (example) {
        // Generate Autoscan Example
        auto&& directoryTag = ConfigDefinition::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY);
#ifdef HAVE_INOTIFY
        auto&& autoscanTag = ConfigDefinition::mapConfigOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), "", true);
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_MODE, "inotify");
#else
        auto&& autoscanTag = ConfigDefinition::mapConfigOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), "", true);
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_MODE, "timed");
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_INTERVAL, "1000");
#endif
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE, "yes");
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, "yes");
        setValue(as, ATTR_AUTOSCAN_DIRECTORY_MEDIATYPE, "Any");
    }
    {
        // Generate Charsets
        auto co = ConfigDefinition::findConfigSetup(CFG_IMPORT_FILESYSTEM_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_METADATA_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_PLAYLIST_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#ifdef HAVE_LIBEXIF
        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_LIBOPTS_EXIF_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_EXIV2
        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_LIBOPTS_EXIV2_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_TAGLIB
        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_LIBOPTS_ID3_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_FFMPEG
        co = ConfigDefinition::findConfigSetup(CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
    }
    generateOptions(options);

    generateMappings();
    generateBoxlayout(CFG_BOXLAYOUT_BOX);
#ifdef ONLINE_SERVICES
    generateOnlineContent();
#endif
}

void ConfigGenerator::generateMappings()
{
    auto ext2mt = setValue(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    setDictionary(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for PS3 divx support ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/divx" /> )");

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/avi" /> )");

    setDictionary(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);
    setDictionary(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    setDictionary(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST);
    setVector(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST);

    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE, false },
        { CFG_IMPORT_MAPPINGS_IGNORED_EXTENSIONS, false },
    };
    generateOptions(options);
}

void ConfigGenerator::generateBoxlayout(config_option_t option)
{
    auto cs = ConfigDefinition::findConfigSetup<ConfigBoxLayoutSetup>(option);

    const auto boxTag = ConfigDefinition::mapConfigOption(option);

    for (auto&& bl : cs->getDefault()) {
        auto box = setValue(fmt::format("{}/", boxTag), "", true);
        setValue(box, ATTR_BOXLAYOUT_BOX_KEY, bl.getKey());
        setValue(box, ATTR_BOXLAYOUT_BOX_TITLE, bl.getTitle());
        setValue(box, ATTR_BOXLAYOUT_BOX_CLASS, bl.getClass());
        if (bl.getSize() != 1)
            setValue(box, ATTR_BOXLAYOUT_BOX_SIZE, fmt::to_string(bl.getSize()));
        if (!bl.getEnabled())
            setValue(box, ATTR_BOXLAYOUT_BOX_ENABLED, fmt::to_string(bl.getEnabled()));
    }
}

void ConfigGenerator::generateOnlineContent()
{
    auto options = std::vector<std::pair<config_option_t, bool>> {
#ifdef ATRAILERS
        { CFG_ONLINE_CONTENT_ATRAILERS_ENABLED, true },
        { CFG_ONLINE_CONTENT_ATRAILERS_REFRESH, true },
        { CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START, true },
        { CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION, true },
        { CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER, false },
#endif
    };
    generateOptions(options);

#ifndef ATRAILERS
    setValue("/import/online-content/");
#endif
}

void ConfigGenerator::generateTranscoding()
{
    auto options = std::vector<std::pair<config_option_t, bool>> {
        { CFG_TRANSCODING_TRANSCODING_ENABLED, true },
        { CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED, false },
        { CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED, false },
#ifdef HAVE_CURL
        { CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, false },
        { CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE, false },
#endif // HAVE_CURL
    };
    generateOptions(options);
    setDictionary(ATTR_TRANSCODING_MIMETYPE_PROF_MAP);

    const auto profileTag = ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE);

    auto oggmp3 = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "ogg2mp3");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "audio/mpeg");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, NO);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, NO);

    auto agent = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "ffmpeg");
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-y -i %in -f mp3 %out");

    auto buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_AUDIO_BUFFER_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_AUDIO_CHUNK_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_AUDIO_FILL_SIZE));

    auto vlcmpeg = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "vlcmpeg");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "video/mpeg");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, YES);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, YES);

    agent = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "vlc");
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc://quit");

    buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_VIDEO_FILL_SIZE));
}

void ConfigGenerator::generateUdn(bool doExport)
{
    auto co = ConfigDefinition::findConfigSetup(CFG_SERVER_UDN);
    co->setDefaultValue(fmt::format("uuid:{}", generateRandomId()));
    if (doExport)
        setValue(CFG_SERVER_UDN);
}

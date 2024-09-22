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
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_generator.h"

#include <sstream>

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config/setup/config_setup_boxlayout.h"
#include "config/setup/config_setup_dictionary.h"
#include "config/setup/config_setup_vector.h"
#include "config_val.h"
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

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, const std::string& value)
{
    auto cs = ConfigDefinition::findConfigSetup(option);
    return setValue(cs->xpath, value.empty() ? cs->getDefaultValue() : value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(const std::shared_ptr<pugi::xml_node>& parent, ConfigVal option, const std::string& value)
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

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, ConfigVal attr, const std::string& value)
{
    auto cs = ConfigDefinition::findConfigSetup(option);
    return setValue(fmt::format("{}/{}", cs->xpath, ConfigDefinition::mapConfigOption(attr)), value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, const std::string& key, const std::string& value)
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

std::shared_ptr<pugi::xml_node> ConfigGenerator::setDictionary(ConfigVal option)
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

std::shared_ptr<pugi::xml_node> ConfigGenerator::setVector(ConfigVal option)
{
    auto cs = std::dynamic_pointer_cast<ConfigVectorSetup>(ConfigDefinition::findConfigSetup(option));
    if (!cs)
        return nullptr;

    auto nodeKey = ConfigDefinition::mapConfigOption(cs->nodeOption);
    for (auto&& value : cs->getXmlContent({})) {
        setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), "", true);
        for (auto&& [key, val] : value)
            setValue(fmt::format("{}/{}/attribute::{}", cs->xpath, nodeKey, key), val);
    }
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(ConfigVal option, ConfigVal dict, ConfigVal attr, const std::string& value)
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

void ConfigGenerator::generateOptions(const std::vector<std::pair<ConfigVal, bool>>& options)
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

    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::SERVER_NAME, true },
        { ConfigVal::SERVER_UDN, true },
        { ConfigVal::SERVER_HOME, true },
        { ConfigVal::SERVER_WEBROOT, true },
        { ConfigVal::SERVER_PORT, false },
        { ConfigVal::SERVER_IP, false },
        { ConfigVal::SERVER_NETWORK_INTERFACE, false },
        { ConfigVal::SERVER_MANUFACTURER, false },
        { ConfigVal::SERVER_MANUFACTURER_URL, false },
        { ConfigVal::SERVER_MODEL_NAME, false },
        { ConfigVal::SERVER_MODEL_DESCRIPTION, false },
        { ConfigVal::SERVER_MODEL_NUMBER, false },
        { ConfigVal::SERVER_MODEL_URL, false },
        { ConfigVal::SERVER_SERIAL_NUMBER, false },
        { ConfigVal::SERVER_PRESENTATION_URL, false },
        { ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO, false },
        { ConfigVal::SERVER_HOME_OVERRIDE, false },
        { ConfigVal::SERVER_TMPDIR, false },
        { ConfigVal::SERVER_HIDE_PC_DIRECTORY, false },
        { ConfigVal::SERVER_BOOKMARK_FILE, false },
        { ConfigVal::SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT, false },
        { ConfigVal::VIRTUAL_URL, false },
        { ConfigVal::EXTERNAL_URL, false },
        { ConfigVal::UPNP_LITERAL_HOST_REDIRECTION, false },
        { ConfigVal::UPNP_MULTI_VALUES_ENABLED, false },
        { ConfigVal::UPNP_SEARCH_SEPARATOR, false },
        { ConfigVal::UPNP_SEARCH_FILENAME, false },
        { ConfigVal::UPNP_SEARCH_ITEM_SEGMENTS, false },
        { ConfigVal::UPNP_SEARCH_CONTAINER_FLAG, false },
        { ConfigVal::UPNP_ALBUM_PROPERTIES, false },
        { ConfigVal::UPNP_ARTIST_PROPERTIES, false },
        { ConfigVal::UPNP_GENRE_PROPERTIES, false },
        { ConfigVal::UPNP_PLAYLIST_PROPERTIES, false },
        { ConfigVal::UPNP_TITLE_PROPERTIES, false },
        { ConfigVal::UPNP_ALBUM_NAMESPACES, false },
        { ConfigVal::UPNP_ARTIST_NAMESPACES, false },
        { ConfigVal::UPNP_GENRE_NAMESPACES, false },
        { ConfigVal::UPNP_PLAYLIST_NAMESPACES, false },
        { ConfigVal::UPNP_TITLE_NAMESPACES, false },
        { ConfigVal::UPNP_CAPTION_COUNT, false },
#ifdef GRBDEBUG
        { ConfigVal::SERVER_LOG_DEBUG_MODE, false },
#endif
    };

    generateUdn(false);

    {
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_HOME);
        co->setDefaultValue(userHome / configDir);
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_WEBROOT);
        co->setDefaultValue(dataDir / DEFAULT_WEB_DIR);
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_MODEL_NUMBER);
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
    setValue(ConfigVal::SERVER_ALIVE_INTERVAL);

    generateDatabase(dataDir);
    generateDynamics();
    generateExtendedRuntime();
}

void ConfigGenerator::generateUi()
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::SERVER_UI_ENABLED, true },
        { ConfigVal::SERVER_UI_SHOW_TOOLTIPS, true },
        { ConfigVal::SERVER_UI_ACCOUNTS_ENABLED, true },
        { ConfigVal::SERVER_UI_SESSION_TIMEOUT, true },
        { ConfigVal::SERVER_UI_POLL_INTERVAL, false },
        { ConfigVal::SERVER_UI_POLL_WHEN_IDLE, false },
        { ConfigVal::SERVER_UI_ENABLE_NUMBERING, false },
        { ConfigVal::SERVER_UI_ENABLE_THUMBNAIL, false },
        { ConfigVal::SERVER_UI_ENABLE_VIDEO, false },
        { ConfigVal::SERVER_UI_DEFAULT_ITEMS_PER_PAGE, false },
        { ConfigVal::SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, false },
    };
    generateOptions(options);
    setValue(ConfigVal::SERVER_UI_ACCOUNT_LIST, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_USER, DEFAULT_ACCOUNT_USER);
    setValue(ConfigVal::SERVER_UI_ACCOUNT_LIST, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ConfigVal::A_SERVER_UI_ACCOUNT_LIST_PASSWORD, DEFAULT_ACCOUNT_PASSWORD);
}

void ConfigGenerator::generateDynamics()
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::SERVER_DYNAMIC_CONTENT_LIST_ENABLED, true },
    };
    generateOptions(options);

    setDictionary(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);

    auto&& containersTag = ConfigDefinition::mapConfigOption(ConfigVal::SERVER_DYNAMIC_CONTENT_LIST);
    auto&& containerTag = ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER);

    auto container = setValue(fmt::format("{}/{}/", containersTag, containerTag), "", true);
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, "/LastAdded");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, "Recently Added");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_SORT, "-last_updated");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_updated > "@last7")");

    container = setValue(fmt::format("{}/{}/", containersTag, containerTag), "", true);
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION, "/LastModified");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_TITLE, "Recently Modified");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_SORT, "-last_modified");
    setValue(container, ConfigVal::A_DYNAMIC_CONTAINER_FILTER, R"(upnp:class derivedfrom "object.item" and last_modified > "@last7")");
}

void ConfigGenerator::generateDatabase(const fs::path& prefixDir)
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::SERVER_STORAGE_SQLITE_ENABLED, true },
        { ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE, true },
        { ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS, false },
        { ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS, false },
        { ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE, false },
        { ConfigVal::SERVER_STORAGE_SQLITE_RESTORE, false },
        { ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE, false },
        { ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE, false },
#ifdef SQLITE_BACKUP_ENABLED
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED, true },
        { ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, true },
#endif
#ifdef HAVE_MYSQL
        { ConfigVal::SERVER_STORAGE_MYSQL_ENABLED, true },
        { ConfigVal::SERVER_STORAGE_MYSQL_HOST, true },
        { ConfigVal::SERVER_STORAGE_MYSQL_USERNAME, true },
        { ConfigVal::SERVER_STORAGE_MYSQL_DATABASE, true },
        { ConfigVal::SERVER_STORAGE_MYSQL_PORT, false },
        { ConfigVal::SERVER_STORAGE_MYSQL_SOCKET, false },
        { ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD, false },
        { ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE, false },
        { ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE, false },
#endif
    };
    {
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "sqlite3.sql");
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "sqlite3-upgrade.xml");
    }

#ifdef HAVE_MYSQL
    {
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        co->setDefaultValue(prefixDir / "mysql.sql");
        co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE);
        co->setDefaultValue(prefixDir / "mysql-upgrade.xml");
    }
#endif

    generateOptions(options);
}

void ConfigGenerator::generateExtendedRuntime()
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {
#ifdef HAVE_FFMPEGTHUMBNAILER
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, true }, // clang does require additional indentation
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, true },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, true },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, true },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, true },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED, false },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED, false },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, false },
        { ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, false },
#endif
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED, true },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES, true },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, true },
        { ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, true },
#ifdef HAVE_LASTFMLIB
        { ConfigVal::SERVER_EXTOPTS_LASTFM_ENABLED, false },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_USERNAME, false },
        { ConfigVal::SERVER_EXTOPTS_LASTFM_PASSWORD, false },
#endif
    };
    generateOptions(options);

    setValue(ConfigVal::SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, ConfigVal::A_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
}

void ConfigGenerator::generateImport(const fs::path& prefixDir, const fs::path& configDir, const fs::path& magicFile)
{
    // Simple Import options
    auto options = std::vector<std::pair<ConfigVal, bool>> {
#ifdef HAVE_MAGIC
        { ConfigVal::IMPORT_MAGIC_FILE, true },
#endif
        { ConfigVal::IMPORT_HIDDEN_FILES, true },
        { ConfigVal::IMPORT_FOLLOW_SYMLINKS, false },
        { ConfigVal::IMPORT_DEFAULT_DATE, false },
        { ConfigVal::IMPORT_LAYOUT_MODE, false },
        { ConfigVal::IMPORT_NOMEDIA_FILE, false },
        { ConfigVal::IMPORT_VIRTUAL_DIRECTORY_KEYS, false },
        { ConfigVal::IMPORT_FILESYSTEM_CHARSET, false },
        { ConfigVal::IMPORT_METADATA_CHARSET, false },
        { ConfigVal::IMPORT_PLAYLIST_CHARSET, false },
#ifdef HAVE_JS
        { ConfigVal::IMPORT_SCRIPTING_CHARSET, true },
        { ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_PLAYLIST, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_METAFILE, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_AUDIOFILE, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_VIDEOFILE, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_IMAGEFILE, true },
#ifdef ONLINESERVICES
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_FUNCTION_TRAILER, true },
#endif
        { ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER, false },
        { ConfigVal::IMPORT_SCRIPTING_PLAYLIST_LINK_OBJECTS, false },
        { ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_SKIPCHARS, false },
        { ConfigVal::IMPORT_SCRIPTING_STRUCTURED_LAYOUT_DIVCHAR, false },
#endif // HAVE_JS
#ifdef HAVE_LIBEXIF
        { ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET, false },
#endif
#ifdef HAVE_EXIV2
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET, false },
#endif
#ifdef HAVE_TAGLIB
        { ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET, false },
#endif
#ifdef HAVE_FFMPEG
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST, false },
        { ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET, false },
#endif
        { ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE, true },
        { ConfigVal::IMPORT_SCRIPTING_IMPORT_GENRE_MAP, false },
        { ConfigVal::IMPORT_SYSTEM_DIRECTORIES, false },
        { ConfigVal::IMPORT_VISIBLE_DIRECTORIES, false },
        { ConfigVal::IMPORT_READABLE_NAMES, false },
        { ConfigVal::IMPORT_RESOURCES_ORDER, false },
        { ConfigVal::IMPORT_LAYOUT_PARENT_PATH, false },
        { ConfigVal::IMPORT_LAYOUT_MAPPING, false },
        { ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP, false },
        { ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, false },
        { ConfigVal::IMPORT_DIRECTORIES_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE, false },
        { ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION, false },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT, false },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_MINDEPTH, false },
        { ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST, false },
        { ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST, false },
#ifdef HAVE_INOTIFY
        { ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY, false },
#endif
    };

#ifdef HAVE_JS
    // Set Script Folders
    {
        std::string scriptDir;
        scriptDir = prefixDir / DEFAULT_JS_DIR;
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_SCRIPTING_COMMON_FOLDER);
        co->setDefaultValue(scriptDir);
    }
    {
        std::string scriptDir;
        scriptDir = configDir / DEFAULT_JS_DIR;
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_SCRIPTING_CUSTOM_FOLDER);
        co->setDefaultValue(scriptDir);
    }
#endif

#ifdef HAVE_MAGIC
    if (!magicFile.empty()) {
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_MAGIC_FILE);
        co->setDefaultValue(magicFile);
    }
#endif
    if (example) {
        // Generate Autoscan Example
        auto&& directoryTag = ConfigDefinition::mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY);
#ifdef HAVE_INOTIFY
        auto&& autoscanTag = ConfigDefinition::mapConfigOption(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), "", true);
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, "inotify");
#else
        auto&& autoscanTag = ConfigDefinition::mapConfigOption(ConfigVal::IMPORT_AUTOSCAN_TIMED_LIST);
        auto as = setValue(fmt::format("{}/{}/", autoscanTag, directoryTag), "", true);
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION, "/media");
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MODE, "timed");
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL, "1000");
#endif
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE, "yes");
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES, "yes");
        setValue(as, ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE, "Any");
    }
    {
        // Generate Charsets
        auto co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_FILESYSTEM_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_METADATA_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);

        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_PLAYLIST_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#ifdef HAVE_LIBEXIF
        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_EXIV2
        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_TAGLIB
        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_LIBOPTS_ID3_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
#ifdef HAVE_FFMPEG
        co = ConfigDefinition::findConfigSetup(ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET);
        co->setDefaultValue(DEFAULT_INTERNAL_CHARSET);
#endif
    }
    generateOptions(options);

    generateMappings();
    generateBoxlayout(ConfigVal::BOXLAYOUT_BOX);
#ifdef ONLINE_SERVICES
    generateOnlineContent();
#endif
}

void ConfigGenerator::generateMappings()
{
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

    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE, false },
        { ConfigVal::IMPORT_MAPPINGS_IGNORED_EXTENSIONS, false },
    };
    generateOptions(options);
}

void ConfigGenerator::generateBoxlayout(ConfigVal option)
{
    auto cs = ConfigDefinition::findConfigSetup<ConfigBoxLayoutSetup>(option);

    const auto boxTag = ConfigDefinition::mapConfigOption(option);

    for (auto&& bl : cs->getDefault()) {
        auto box = setValue(fmt::format("{}/", boxTag), "", true);
        setValue(box, ConfigVal::A_BOXLAYOUT_BOX_KEY, bl.getKey());
        setValue(box, ConfigVal::A_BOXLAYOUT_BOX_TITLE, bl.getTitle());
        setValue(box, ConfigVal::A_BOXLAYOUT_BOX_CLASS, bl.getClass());
        if (bl.getSize() != 1)
            setValue(box, ConfigVal::A_BOXLAYOUT_BOX_SIZE, fmt::to_string(bl.getSize()));
        if (!bl.getEnabled())
            setValue(box, ConfigVal::A_BOXLAYOUT_BOX_ENABLED, fmt::to_string(bl.getEnabled()));
    }
}

void ConfigGenerator::generateOnlineContent()
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {};
    generateOptions(options);

#ifdef ONLINE_SERVICES
    setValue("/import/online-content/");
#endif
}

void ConfigGenerator::generateTranscoding()
{
    auto options = std::vector<std::pair<ConfigVal, bool>> {
        { ConfigVal::TRANSCODING_TRANSCODING_ENABLED, true },
        { ConfigVal::TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED, false },
        { ConfigVal::TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED, false },
#ifdef HAVE_CURL
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, false },
        { ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE, false },
#endif // HAVE_CURL
    };
    generateOptions(options);
    setDictionary(ConfigVal::A_TRANSCODING_MIMETYPE_PROF_MAP);

    const auto profileTag = ConfigDefinition::mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE);

    auto oggmp3 = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME, "ogg2mp3");
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "audio/mpeg");
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, NO);
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(oggmp3, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG, NO);

    auto agent = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "ffmpeg");
    setValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-y -i %in -f mp3 %out");

    auto buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_AUDIO_BUFFER_SIZE));
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_AUDIO_CHUNK_SIZE));
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_AUDIO_FILL_SIZE));

    auto vlcmpeg = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_NAME, "vlcmpeg");
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "video/mpeg");
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCURL, YES);
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(vlcmpeg, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_ACCOGG, YES);

    agent = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "vlc");
    setValue(agent, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc://quit");

    buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
    setValue(buffer, ConfigVal::A_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_VIDEO_FILL_SIZE));
}

void ConfigGenerator::generateUdn(bool doExport)
{
    auto co = ConfigDefinition::findConfigSetup(ConfigVal::SERVER_UDN);
    co->setDefaultValue(fmt::format("uuid:{}", generateRandomId()));
    if (doExport)
        setValue(ConfigVal::SERVER_UDN);
}

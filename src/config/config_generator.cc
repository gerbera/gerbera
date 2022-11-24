/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2022 Gerbera Contributors

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
#include "config/config_setup.h"
#include "util/grb_time.h"
#include "util/tools.h"

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
    generateImport(dataDir, magicFile);
    generateTranscoding();

    std::ostringstream buf;
    doc.print(buf, "  ");
    return buf.str();
}

void ConfigGenerator::generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir)
{
    auto server = setValue("/server/");
    generateUi();
    setValue(CFG_SERVER_NAME);

    generateUdn();

    fs::path homepath = userHome / configDir;
    setValue(CFG_SERVER_HOME, homepath);

    fs::path webRoot = dataDir / DEFAULT_WEB_DIR;
    setValue(CFG_SERVER_WEBROOT, webRoot);

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

    generateDatabase();
    generateDynamics();
    generateExtendedRuntime();
}

void ConfigGenerator::generateUi()
{
    setValue(CFG_SERVER_UI_ENABLED);
    setValue(CFG_SERVER_UI_SHOW_TOOLTIPS);
    setValue(CFG_SERVER_UI_ACCOUNTS_ENABLED);
    setValue(CFG_SERVER_UI_SESSION_TIMEOUT);
    setValue(CFG_SERVER_UI_ACCOUNT_LIST, ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, DEFAULT_ACCOUNT_USER);
    setValue(CFG_SERVER_UI_ACCOUNT_LIST, ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD, DEFAULT_ACCOUNT_PASSWORD);
}

void ConfigGenerator::generateDynamics()
{
    setValue(CFG_SERVER_DYNAMIC_CONTENT_LIST_ENABLED);
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

void ConfigGenerator::generateDatabase()
{
    setValue(CFG_SERVER_STORAGE_SQLITE_ENABLED);
    setValue(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);

#ifdef SQLITE_BACKUP_ENABLED
    setValue(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);
    setValue(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
#endif

#ifdef HAVE_MYSQL
    setValue(CFG_SERVER_STORAGE_MYSQL_ENABLED, NO);
    setValue(CFG_SERVER_STORAGE_MYSQL_HOST);
    setValue(CFG_SERVER_STORAGE_MYSQL_USERNAME);
    setValue(CFG_SERVER_STORAGE_MYSQL_DATABASE);
#endif
}

void ConfigGenerator::generateExtendedRuntime()
{
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED);
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
#endif

    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED);
    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);
    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);
    setValue(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
}

void ConfigGenerator::generateImport(const fs::path& prefixDir, const fs::path& magicFile)
{
    setValue(CFG_IMPORT_HIDDEN_FILES);

#ifdef HAVE_MAGIC
    if (!magicFile.empty()) {
        setValue(CFG_IMPORT_MAGIC_FILE, magicFile);
    }
#endif

#ifdef HAVE_JS
    setValue(CFG_IMPORT_SCRIPTING_CHARSET);

    std::string script;
    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT, script);

    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT, script);

    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_METAFILE_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_METAFILE_SCRIPT, script);
#endif

    setValue(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);

#ifdef HAVE_JS
    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT, script);
#endif

    generateMappings();
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
}

void ConfigGenerator::generateOnlineContent()
{
#ifdef ATRAILERS
    setValue(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);
    setValue(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH);
    setValue(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);
    setValue(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#else
    setValue("/import/online-content/");
#endif
}

void ConfigGenerator::generateTranscoding()
{
    setValue(CFG_TRANSCODING_TRANSCODING_ENABLED);
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
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit");

    buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigDefinition::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, fmt::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, fmt::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, fmt::to_string(DEFAULT_VIDEO_FILL_SIZE));
}

void ConfigGenerator::generateUdn()
{
    setValue(CFG_SERVER_UDN, fmt::format("uuid:{}", generateRandomId()));
}

/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2021 Gerbera Contributors

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

#include <string>
#include <utility>

#include "config/config_setup.h"
#include "metadata/metadata_handler.h"
#include "util/tools.h"

ConfigGenerator::ConfigGenerator() = default;

std::map<std::string, std::shared_ptr<pugi::xml_node>> ConfigGenerator::generated;
pugi::xml_document ConfigGenerator::doc;

std::shared_ptr<pugi::xml_node> ConfigGenerator::init()
{
    if (generated.find("") == generated.end()) {
        auto config = doc.append_child("config");
        generated[""] = std::make_shared<pugi::xml_node>(config);
    }
    return generated[""];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::getNode(const std::string& tag)
{
    log_debug("reading '{}' -> {}", tag, generated.find(tag) == generated.end());
    return generated[tag];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(const std::string& tag, const std::string& value, bool makeLastChild)
{
    auto split = splitString(tag, '/');
    std::shared_ptr<pugi::xml_node> result = nullptr;
    if (split.size() > 0) {
        std::string parent = "";
        std::string nodeKey = "";
        std::string attribute = "";

        for (const auto& part : split) {
            nodeKey += "/" + part;
            if (generated.find(nodeKey) == generated.end()) {
                if (part.substr(0, ConfigSetup::ATTRIBUTE.size()) == ConfigSetup::ATTRIBUTE) {
                    attribute = part.substr(ConfigSetup::ATTRIBUTE.size()); // last attribute gets the value
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
    auto cs = ConfigManager::findConfigSetup(option);
    return setValue(cs->xpath, value.empty() ? cs->getDefaultValue() : value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(std::shared_ptr<pugi::xml_node>& parent, config_option_t option, const std::string& value)
{
    auto cs = std::string(ConfigManager::mapConfigOption(option));
    if (cs.substr(0, ConfigSetup::ATTRIBUTE.size()) == ConfigSetup::ATTRIBUTE) {
        cs = ConfigSetup::removeAttribute(option);
        parent->append_attribute(cs.c_str()) = value.c_str();
    } else {
        parent->append_child(cs.c_str()).append_child(pugi::node_pcdata).set_value(value.c_str());
    }
    return parent;
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, config_option_t attr, const std::string& value)
{
    auto cs = ConfigManager::findConfigSetup(option);
    return setValue(fmt::format("{}/{}", cs->xpath, ConfigManager::mapConfigOption(attr)), value);
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, const std::string& key, const std::string& value)
{
    auto cs = std::dynamic_pointer_cast<ConfigDictionarySetup>(ConfigManager::findConfigSetup(option));
    if (cs == nullptr)
        return nullptr;

    auto nodeKey = ConfigManager::mapConfigOption(cs->nodeOption);
    setValue(fmt::format("{}/{}/", cs->xpath, nodeKey), "", true);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigSetup::ensureAttribute(cs->keyOption)), key);
    setValue(fmt::format("{}/{}/{}", cs->xpath, nodeKey, ConfigSetup::ensureAttribute(cs->valOption)), value);
    return generated[cs->xpath];
}

std::shared_ptr<pugi::xml_node> ConfigGenerator::setValue(config_option_t option, config_option_t dict, config_option_t attr, const std::string& value)
{
    auto cs = ConfigManager::findConfigSetup(option);
    return setValue(fmt::format("{}/{}/{}", cs->xpath, ConfigManager::mapConfigOption(dict), ConfigSetup::ensureAttribute(attr)), value);
}

std::string ConfigGenerator::generate(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir, const fs::path& magicFile)
{
    auto decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto config = init();

    config->append_attribute("version") = CONFIG_XML_VERSION;
    config->append_attribute("xmlns") = (XML_XMLNS + std::to_string(CONFIG_XML_VERSION)).c_str();
    config->append_attribute("xmlns:xsi") = XML_XMLNS_XSI;
    config->append_attribute("xsi:schemaLocation") = (std::string(XML_XMLNS) + std::to_string(CONFIG_XML_VERSION) + " " + XML_XMLNS + std::to_string(CONFIG_XML_VERSION) + ".xsd").c_str();

    auto docinfo = config->append_child(pugi::node_comment);
    docinfo.set_value("\n\
     See http://gerbera.io or read the docs for more\n\
     information on creating and using config.xml configuration files.\n\
    ");

    generateServer(userHome, configDir, prefixDir);
    generateImport(prefixDir, magicFile);
    generateTranscoding();

    std::ostringstream buf;
    doc.print(buf, "  ");
    return buf.str();
}

void ConfigGenerator::generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir)
{
    auto server = setValue("/server/");
    generateUi();
    setValue(CFG_SERVER_NAME);

    generateUdn();

    fs::path homepath = userHome / configDir;
    setValue(CFG_SERVER_HOME, homepath.string());

    std::string webRoot = prefixDir / DEFAULT_WEB_DIR;
    setValue(CFG_SERVER_WEBROOT, webRoot);

    auto aliveinfo = server->append_child(pugi::node_comment);
    aliveinfo.set_value(
        ("\n\
        How frequently (in seconds) to send ssdp:alive advertisements.\n\
        Minimum alive value accepted is: "
            + std::to_string(ALIVE_INTERVAL_MIN) + "\n\n\
        The advertisement will be sent every (A/2)-30 seconds,\n\
        and will have a cache-control max-age of A where A is\n\
        the value configured here. Ex: A value of 62 will result\n\
        in an SSDP advertisement being sent every second.\n\
    ")
            .c_str());
    setValue(CFG_SERVER_ALIVE_INTERVAL);

    generateDatabase();
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
    setValue(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);
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
        setValue(CFG_IMPORT_MAGIC_FILE, magicFile.string());
    }
#endif

    setValue(CFG_IMPORT_SCRIPTING_CHARSET);

#ifdef HAVE_JS
    std::string script;
    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT, script);

    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT;
    setValue(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT, script);
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
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "asf", "video/x-ms-asf");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "asx", "video/x-ms-asf");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "dff", "audio/x-dsd");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "dsf", "audio/x-dsd");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "flv", "video/x-flv");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "m2ts", "video/mp2t"); // LibMagic fails to identify MPEG2 Transport Streams
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "m3u", "audio/x-mpegurl");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "mka", "audio/x-matroska");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "mkv", "video/x-matroska");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "mp3", "audio/mpeg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "mts", "video/mp2t"); // LibMagic fails to identify MPEG2 Transport Streams
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "oga", "audio/ogg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "ogg", "audio/ogg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "ogm", "video/ogg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "ogv", "video/ogg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "ogx", "application/ogg");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "pls", "audio/x-scpls");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "ts", "video/mp2t"); // LibMagic fails to identify MPEG2 Transport Streams
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "tsa", "audio/mp2t"); // LibMagic fails to identify MPEG2 Transport Streams
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "tsv", "video/mp2t"); // LibMagic fails to identify MPEG2 Transport Streams
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wax", "audio/x-ms-wax");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wm", "video/x-ms-wm");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wma", "audio/x-ms-wma");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wmv", "video/x-ms-wmv");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wmx", "video/x-ms-wmx");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wv", "audio/x-wavpack");
    setValue(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "wvx", "video/x-ms-wvx");

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for PS3 divx support ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/divx" /> )");

    ext2mt->append_child(pugi::node_comment).set_value(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 ");
    ext2mt->append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/avi" /> )");

    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "audio/*", UPNP_CLASS_MUSIC_TRACK);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "video/*", UPNP_CLASS_VIDEO_ITEM);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "image/*", UPNP_CLASS_IMAGE_ITEM);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "application/ogg", UPNP_CLASS_MUSIC_TRACK);

    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/mpeg", CONTENT_TYPE_MP3);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "application/ogg", CONTENT_TYPE_OGG);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/ogg", CONTENT_TYPE_OGG);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-flac", CONTENT_TYPE_FLAC);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/flac", CONTENT_TYPE_FLAC);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-ms-wma", CONTENT_TYPE_WMA);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-wavpack", CONTENT_TYPE_WAVPACK);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "image/jpeg", CONTENT_TYPE_JPG);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-mpegurl", CONTENT_TYPE_PLAYLIST);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-scpls", CONTENT_TYPE_PLAYLIST);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-wav", CONTENT_TYPE_PCM);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/L16", CONTENT_TYPE_PCM);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "video/x-msvideo", CONTENT_TYPE_AVI);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "video/mp4", CONTENT_TYPE_MP4);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/mp4", CONTENT_TYPE_MP4);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "video/x-matroska", CONTENT_TYPE_MKV);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-matroska", CONTENT_TYPE_MKA);
    setValue(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "audio/x-dsd", CONTENT_TYPE_DSD);
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
    auto transcoding = setValue(CFG_TRANSCODING_TRANSCODING_ENABLED);
    setValue(ATTR_TRANSCODING_MIMETYPE_PROF_MAP, "video/x-flv", "vlcmpeg");
    setValue(ATTR_TRANSCODING_MIMETYPE_PROF_MAP, "application/ogg", "vlcmpeg");
    setValue(ATTR_TRANSCODING_MIMETYPE_PROF_MAP, "audio/ogg", "ogg2mp3");

    const auto profileTag = ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE);

    auto oggmp3 = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "ogg2mp3");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "audio/mpeg");
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, NO);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(oggmp3, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, NO);

    auto agent = setValue(fmt::format("{}/{}/", profileTag, ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "ffmpeg");
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-y -i %in -f mp3 %out");

    auto buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, std::to_string(DEFAULT_AUDIO_BUFFER_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, std::to_string(DEFAULT_AUDIO_CHUNK_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, std::to_string(DEFAULT_AUDIO_FILL_SIZE));

    auto vlcmpeg = setValue(fmt::format("{}/", profileTag), "", true);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "vlcmpeg");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, NO);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "external");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "video/mpeg");
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, YES);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, YES);
    setValue(vlcmpeg, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, YES);

    agent = setValue(fmt::format("{}/{}/", profileTag, ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)), "", true);
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "vlc");
    setValue(agent, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit");

    buffer = setValue(fmt::format("{}/{}/", profileTag, ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)), "", true);
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, std::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, std::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
    setValue(buffer, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, std::to_string(DEFAULT_VIDEO_FILL_SIZE));
}

void ConfigGenerator::generateUdn()
{
    setValue(CFG_SERVER_UDN, fmt::format("uuid:{}", generateRandomId()));
}

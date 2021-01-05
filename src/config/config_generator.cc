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

#include "metadata/metadata_handler.h"
#include "util/tools.h"

ConfigGenerator::ConfigGenerator() = default;

std::string ConfigGenerator::generate(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir, const fs::path& magicFile)
{
    pugi::xml_document doc;

    auto decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto config = doc.append_child("config");
    config.append_attribute("version") = CONFIG_XML_VERSION;
    config.append_attribute("xmlns") = (XML_XMLNS + std::to_string(CONFIG_XML_VERSION)).c_str();
    config.append_attribute("xmlns:xsi") = XML_XMLNS_XSI;
    config.append_attribute("xsi:schemaLocation") = (std::string(XML_XMLNS) + std::to_string(CONFIG_XML_VERSION) + " " + XML_XMLNS + std::to_string(CONFIG_XML_VERSION) + ".xsd").c_str();

    auto docinfo = config.append_child(pugi::node_comment);
    docinfo.set_value("\n\
     See http://gerbera.io or read the docs for more\n\
     information on creating and using config.xml configuration files.\n\
    ");

    generateServer(userHome, configDir, prefixDir, &config);
    generateImport(prefixDir, magicFile, &config);
    generateTranscoding(&config);

    std::ostringstream buf;
    doc.print(buf, "  ");
    return buf.str();
}

void ConfigGenerator::generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir, pugi::xml_node* config)
{
    auto server = config->append_child("server");

    generateUi(&server);
    server.append_child("name").append_child(pugi::node_pcdata).set_value("Gerbera");

    generateUdn(&server);

    fs::path homepath = userHome / configDir;
    server.append_child("home").append_child(pugi::node_pcdata).set_value(homepath.c_str());

    std::string webRoot = prefixDir / DEFAULT_WEB_DIR;
    server.append_child("webroot").append_child(pugi::node_pcdata).set_value(webRoot.c_str());

    auto aliveinfo = server.append_child(pugi::node_comment);
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
    server.append_child("alive").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_ALIVE_INTERVAL).c_str());

    generateDatabase(&server);
    generateExtendedRuntime(&server);
}

void ConfigGenerator::generateUi(pugi::xml_node* server)
{
    auto ui = server->append_child("ui");
    ui.append_attribute("enabled") = DEFAULT_UI_EN_VALUE;
    ui.append_attribute("show-tooltips") = DEFAULT_UI_SHOW_TOOLTIPS_VALUE;

    auto accounts = ui.append_child("accounts");
    accounts.append_attribute("enabled") = DEFAULT_ACCOUNTS_EN_VALUE;
    accounts.append_attribute("session-timeout") = DEFAULT_SESSION_TIMEOUT;

    auto account = accounts.append_child("account");
    account.append_attribute("user") = DEFAULT_ACCOUNT_USER;
    account.append_attribute("password") = DEFAULT_ACCOUNT_PASSWORD;
}

void ConfigGenerator::generateDatabase(pugi::xml_node* server)
{
    auto database = server->append_child("storage");

    auto sqlite3 = database.append_child("sqlite3");
    sqlite3.append_attribute("enabled") = DEFAULT_SQLITE_ENABLED;
    sqlite3.append_child("database-file").append_child(pugi::node_pcdata).set_value(DEFAULT_SQLITE3_DB_FILENAME);
#ifdef SQLITE_BACKUP_ENABLED
    //    <backup enabled="no" interval="6000" />
    auto backup = sqlite3->append_child("backup");
    backup.append_attribute("enabled") = YES;
    backup.append_attribute("interval") = DEFAULT_SQLITE_BACKUP_INTERVAL;
#endif

#ifdef HAVE_MYSQL
    auto mysql = database.append_child("mysql");
    mysql.append_attribute("enabled") = NO;
    mysql.append_child("host").append_child(pugi::node_pcdata).set_value(DEFAULT_MYSQL_HOST);
    mysql.append_child("username").append_child(pugi::node_pcdata).set_value(DEFAULT_MYSQL_USER);
    mysql.append_child("database").append_child(pugi::node_pcdata).set_value(DEFAULT_MYSQL_DB);
#endif
}

void ConfigGenerator::generateExtendedRuntime(pugi::xml_node* server)
{
    auto extended = server->append_child("extended-runtime-options");

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    auto ffth = extended.append_child("ffmpegthumbnailer");
    ffth.append_attribute("enabled") = DEFAULT_FFMPEGTHUMBNAILER_ENABLED;
    ffth.append_child("thumbnail-size").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE).c_str());
    ffth.append_child("seek-percentage").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE).c_str());
    ffth.append_child("filmstrip-overlay").append_child(pugi::node_pcdata).set_value(DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
    ffth.append_child("workaround-bugs").append_child(pugi::node_pcdata).set_value(DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);
    ffth.append_child("image-quality").append_child(pugi::node_pcdata).set_value(std::to_string(DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY).c_str());
#endif

    auto mark = extended.append_child("mark-played-items");
    mark.append_attribute("enabled") = DEFAULT_MARK_PLAYED_ITEMS_ENABLED;
    mark.append_attribute("suppress-cds-updates") = DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES;
    auto mark_string = mark.append_child("string");
    mark_string.append_attribute("mode") = DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE;
    mark_string.append_child(pugi::node_pcdata).set_value(DEFAULT_MARK_PLAYED_ITEMS_STRING);

    auto mark_content_section = mark.append_child("mark");
    auto content_video = mark_content_section.append_child("content");
    content_video.append_child(pugi::node_pcdata).set_value(DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
}

void ConfigGenerator::generateImport(const fs::path& prefixDir, const fs::path& magicFile, pugi::xml_node* config)
{
    auto import = config->append_child("import");
    import.append_attribute("hidden-files") = DEFAULT_HIDDEN_FILES_VALUE;

#ifdef HAVE_MAGIC
    if (!magicFile.empty()) {
        import.append_child("magic-file").append_child(pugi::node_pcdata).set_value(magicFile.c_str());
    }
#endif

    auto scripting = import.append_child("scripting");
    scripting.append_attribute("script-charset") = DEFAULT_JS_CHARSET;

#ifdef HAVE_JS
    std::string script;
    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT;
    scripting.append_child("common-script").append_child(pugi::node_pcdata).set_value(script.c_str());

    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT;
    scripting.append_child("playlist-script").append_child(pugi::node_pcdata).set_value(script.c_str());
#endif

    auto layout = scripting.append_child("virtual-layout");
    layout.append_attribute("type") = DEFAULT_LAYOUT_TYPE;

#ifdef HAVE_JS
    script = prefixDir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT;
    layout.append_child("import-script").append_child(pugi::node_pcdata).set_value(script.c_str());
#endif

    generateMappings(&import);
#ifdef ONLINE_SERVICES
    generateOnlineContent(&import);
#endif
}

void ConfigGenerator::generateMappings(pugi::xml_node* import)
{
    auto mappings = import->append_child("mappings");

    auto ext2mt = mappings.append_child("extension-mimetype");
    ext2mt.append_attribute("ignore-unknown") = DEFAULT_IGNORE_UNKNOWN_EXTENSIONS;
    map_from_to("mp3", "audio/mpeg", &ext2mt);
    map_from_to("ogx", "application/ogg", &ext2mt);
    map_from_to("ogv", "video/ogg", &ext2mt);
    map_from_to("oga", "audio/ogg", &ext2mt);
    map_from_to("ogg", "audio/ogg", &ext2mt);
    map_from_to("ogm", "video/ogg", &ext2mt);
    map_from_to("asf", "video/x-ms-asf", &ext2mt);
    map_from_to("asx", "video/x-ms-asf", &ext2mt);
    map_from_to("wma", "audio/x-ms-wma", &ext2mt);
    map_from_to("wax", "audio/x-ms-wax", &ext2mt);
    map_from_to("wmv", "video/x-ms-wmv", &ext2mt);
    map_from_to("wvx", "video/x-ms-wvx", &ext2mt);
    map_from_to("wm", "video/x-ms-wm", &ext2mt);
    map_from_to("wmx", "video/x-ms-wmx", &ext2mt);
    map_from_to("m3u", "audio/x-mpegurl", &ext2mt);
    map_from_to("pls", "audio/x-scpls", &ext2mt);
    map_from_to("flv", "video/x-flv", &ext2mt);
    map_from_to("mkv", "video/x-matroska", &ext2mt);
    map_from_to("mka", "audio/x-matroska", &ext2mt);
    map_from_to("dsf", "audio/x-dsd", &ext2mt);
    map_from_to("dff", "audio/x-dsd", &ext2mt);
    map_from_to("wv", "audio/x-wavpack", &ext2mt);

    ext2mt.append_child(pugi::node_comment).set_value(" Uncomment the line below for PS3 divx support ");
    ext2mt.append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/divx" /> )");

    ext2mt.append_child(pugi::node_comment).set_value(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 ");
    ext2mt.append_child(pugi::node_comment).set_value(R"( <map from="avi" to="video/avi" /> )");

    auto mtupnp = mappings.append_child("mimetype-upnpclass");
    map_from_to("audio/*", UPNP_CLASS_MUSIC_TRACK, &mtupnp);
    map_from_to("video/*", UPNP_CLASS_VIDEO_ITEM, &mtupnp);
    map_from_to("image/*", UPNP_CLASS_IMAGE_ITEM, &mtupnp);
    map_from_to("application/ogg", UPNP_CLASS_MUSIC_TRACK, &mtupnp);

    auto mtcontent = mappings.append_child("mimetype-contenttype");
    treat_as("audio/mpeg", CONTENT_TYPE_MP3, &mtcontent);
    treat_as("application/ogg", CONTENT_TYPE_OGG, &mtcontent);
    treat_as("audio/ogg", CONTENT_TYPE_OGG, &mtcontent);
    treat_as("audio/x-flac", CONTENT_TYPE_FLAC, &mtcontent);
    treat_as("audio/flac", CONTENT_TYPE_FLAC, &mtcontent);
    treat_as("audio/x-ms-wma", CONTENT_TYPE_WMA, &mtcontent);
    treat_as("audio/x-wavpack", CONTENT_TYPE_WAVPACK, &mtcontent);
    treat_as("image/jpeg", CONTENT_TYPE_JPG, &mtcontent);
    treat_as("audio/x-mpegurl", CONTENT_TYPE_PLAYLIST, &mtcontent);
    treat_as("audio/x-scpls", CONTENT_TYPE_PLAYLIST, &mtcontent);
    treat_as("audio/x-wav", CONTENT_TYPE_PCM, &mtcontent);
    treat_as("audio/L16", CONTENT_TYPE_PCM, &mtcontent);
    treat_as("video/x-msvideo", CONTENT_TYPE_AVI, &mtcontent);
    treat_as("video/mp4", CONTENT_TYPE_MP4, &mtcontent);
    treat_as("audio/mp4", CONTENT_TYPE_MP4, &mtcontent);
    treat_as("video/x-matroska", CONTENT_TYPE_MKV, &mtcontent);
    treat_as("audio/x-matroska", CONTENT_TYPE_MKA, &mtcontent);
    treat_as("audio/x-dsd", CONTENT_TYPE_DSD, &mtcontent);
}

void ConfigGenerator::generateOnlineContent(pugi::xml_node* import)
{
#ifdef ATRAILERS
    auto onlinecontent = import->append_child("online-content");
    auto at = onlinecontent.append_child("AppleTrailers");
    at.append_attribute("enabled") = DEFAULT_ATRAILERS_ENABLED;
    at.append_attribute("refresh") = DEFAULT_ATRAILERS_REFRESH;
    at.append_attribute("update-at-start") = DEFAULT_ATRAILERS_UPDATE_AT_START;
    at.append_attribute("resolution") = DEFAULT_ATRAILERS_RESOLUTION;
#else
    import->append_child("online-content");
#endif
}

void ConfigGenerator::generateTranscoding(pugi::xml_node* config)
{
    auto transcoding = config->append_child("transcoding");
    transcoding.append_attribute("enabled") = DEFAULT_TRANSCODING_ENABLED;

    auto mt_prof_map = transcoding.append_child("mimetype-profile-mappings");

    auto prof_flv = mt_prof_map.append_child("transcode");
    prof_flv.append_attribute("mimetype") = "video/x-flv";
    prof_flv.append_attribute("using") = "vlcmpeg";

    auto prof_theora = mt_prof_map.append_child("transcode");
    prof_theora.append_attribute("mimetype") = "application/ogg";
    prof_theora.append_attribute("using") = "vlcmpeg";

    auto prof_ogg = mt_prof_map.append_child("transcode");
    prof_ogg.append_attribute("mimetype") = "audio/ogg";
    prof_ogg.append_attribute("using") = "ogg2mp3";

    auto profiles = transcoding.append_child("profiles");

    auto oggmp3 = profiles.append_child("profile");
    oggmp3.append_attribute("name") = "ogg2mp3";
    oggmp3.append_attribute("enabled") = NO;
    oggmp3.append_attribute("type") = "external";

    oggmp3.append_child("mimetype").append_child(pugi::node_pcdata).set_value("audio/mpeg");
    oggmp3.append_child("accept-url").append_child(pugi::node_pcdata).set_value(NO);
    oggmp3.append_child("first-resource").append_child(pugi::node_pcdata).set_value(YES);
    oggmp3.append_child("accept-ogg-theora").append_child(pugi::node_pcdata).set_value(NO);

    auto oggmp3_agent = oggmp3.append_child("agent");
    oggmp3_agent.append_attribute("command") = "ffmpeg";
    oggmp3_agent.append_attribute("arguments") = "-y -i %in -f mp3 %out";

    auto oggmp3_buffer = oggmp3.append_child("buffer");
    oggmp3_buffer.append_attribute("size") = DEFAULT_AUDIO_BUFFER_SIZE;
    oggmp3_buffer.append_attribute("chunk-size") = DEFAULT_AUDIO_CHUNK_SIZE;
    oggmp3_buffer.append_attribute("fill-size") = DEFAULT_AUDIO_FILL_SIZE;

    auto vlcmpeg = profiles.append_child("profile");
    vlcmpeg.append_attribute("name") = "vlcmpeg";
    vlcmpeg.append_attribute("enabled") = NO;
    vlcmpeg.append_attribute("type") = "external";

    vlcmpeg.append_child("mimetype").append_child(pugi::node_pcdata).set_value("video/mpeg");
    vlcmpeg.append_child("accept-url").append_child(pugi::node_pcdata).set_value(YES);
    vlcmpeg.append_child("first-resource").append_child(pugi::node_pcdata).set_value(YES);
    vlcmpeg.append_child("accept-ogg-theora").append_child(pugi::node_pcdata).set_value(YES);

    auto vlcmpeg_agent = vlcmpeg.append_child("agent");
    vlcmpeg_agent.append_attribute("command") = "vlc";
    vlcmpeg_agent.append_attribute("arguments") = "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit";

    auto vlcmpeg_buffer = vlcmpeg.append_child("buffer");
    vlcmpeg_buffer.append_attribute("size") = DEFAULT_VIDEO_BUFFER_SIZE;
    vlcmpeg_buffer.append_attribute("chunk-size") = DEFAULT_VIDEO_CHUNK_SIZE;
    vlcmpeg_buffer.append_attribute("fill-size") = DEFAULT_VIDEO_FILL_SIZE;
}

void ConfigGenerator::generateUdn(pugi::xml_node* server)
{
    auto uuid_str = generateRandomId();
    log_debug("UUID generated: {}", uuid_str);
    server->append_child("udn").append_child(pugi::node_pcdata).set_value(fmt::format("uuid:{}", uuid_str).c_str());
}

void ConfigGenerator::map_from_to(const std::string& from, const std::string& to, pugi::xml_node* parent)
{
    auto map = parent->append_child("map");
    map.append_attribute("from") = from.c_str();
    map.append_attribute("to") = to.c_str();
}

void ConfigGenerator::treat_as(const std::string& mimetype, const std::string& as, pugi::xml_node* parent)
{
    auto treat = parent->append_child("treat");
    treat.append_attribute("mimetype") = mimetype.c_str();
    treat.append_attribute("as") = as.c_str();
}

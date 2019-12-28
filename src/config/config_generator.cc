/*GRB*

Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2019 Gerbera Contributors

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


#include <string>
#include "mxml/mxml.h"
#include "common.h"
#include "util/tools.h"
#include "metadata_handler.h"
#ifdef BSD_NATIVE_UUID
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif
#include "config_generator.h"

using namespace zmm;
using namespace mxml;

ConfigGenerator::ConfigGenerator() {}

ConfigGenerator::~ConfigGenerator() {}

std::string ConfigGenerator::generate(std::string userHome, std::string configDir, std::string prefixDir, std::string magicFile) {
  Ref<Element> config(new Element("config"));
  config->setAttribute("version", std::to_string(CONFIG_XML_VERSION));
  config->setAttribute("xmlns", XML_XMLNS + std::to_string(CONFIG_XML_VERSION));
  config->setAttribute("xmlns:xsi", XML_XMLNS_XSI);
  config->setAttribute("xsi:schemaLocation",
                       std::string(XML_XMLNS) + std::to_string(CONFIG_XML_VERSION) + " " + XML_XMLNS + std::to_string(CONFIG_XML_VERSION) + ".xsd");

  Ref<Comment> docinfo(new Comment("\n\
     See http://gerbera.io or read the docs for more\n\
     information on creating and using config.xml configration files.\n\
    ",true));

  config->appendChild(RefCast(docinfo, Node));
  config->appendElementChild(generateServer(userHome, configDir, prefixDir));
  config->appendElementChild(generateImport(prefixDir, magicFile));
  config->appendElementChild(generateTranscoding());
  config->indent();

  return std::string((XML_HEADER) + config->print() + "\n");
}

Ref<Element> ConfigGenerator::generateServer(std::string userHome, std::string configDir, std::string prefixDir) {
  Ref<Element> server(new Element("server"));

  server->appendElementChild(generateUi());
  server->appendTextChild("name", PACKAGE_NAME);

  server->appendElementChild(generateUdn());

  std::string homepath = userHome + DIR_SEPARATOR + configDir;
  server->appendTextChild("home", homepath);

  std::string webRoot = prefixDir + DIR_SEPARATOR + DEFAULT_WEB_DIR;
  server->appendTextChild("webroot", webRoot);

  Ref<Comment> aliveinfo(new Comment(
      "\n\
        How frequently (in seconds) to send ssdp:alive advertisements.\n\
        Minimum alive value accepted is: "
          + std::to_string(ALIVE_INTERVAL_MIN) + "\n\n\
        The advertisement will be sent every (A/2)-30 seconds,\n\
        and will have a cache-control max-age of A where A is\n\
        the value configured here. Ex: A value of 62 will result\n\
        in an SSDP advertisement being sent every second.\n\
    ", true));

  server->appendChild(RefCast(aliveinfo, Node));
  server->appendTextChild("alive", std::to_string(DEFAULT_ALIVE_INTERVAL));

  server->appendElementChild(generateStorage());

  Ref<Element> protocolinfo(new Element("protocolInfo"));
  protocolinfo->setAttribute("extend", DEFAULT_EXTEND_PROTOCOLINFO);
  protocolinfo->setAttribute("dlna-seek", DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK);
  server->appendElementChild(protocolinfo);

  Ref<Comment> ps3protinfo(new Comment(" For PS3 support change to \"yes\" "));
  server->appendChild(RefCast(ps3protinfo, Node));

  Ref<Comment> redinfo(new Comment("\n\
       Uncomment the lines below to get rid of jerky avi playback on the\n\
       DSM320 or to enable subtitles support on the DSM units\n\
    ", true));

  Ref<Comment> redsonic(new Comment("\n\
    <custom-http-headers>\n\
      <add header=\"X-User-Agent: redsonic\"/>\n\
    </custom-http-headers>\n\
\n\
    <manufacturerURL>redsonic.com</manufacturerURL>\n\
    <modelNumber>105</modelNumber>\n\
    ", true));

  Ref<Comment> tg100info(new Comment(" Uncomment the line below if you have a Telegent TG100 ", true));
  Ref<Comment> tg100(new Comment("\n\
       <upnp-string-limit>101</upnp-string-limit>\n\
    ", true));

  server->appendChild(RefCast(redinfo, Node));
  server->appendChild(RefCast(redsonic, Node));
  server->appendChild(RefCast(tg100info, Node));
  server->appendChild(RefCast(tg100, Node));

  server->appendElementChild(generateExtendedRuntime());

  return server;
}

Ref<Element> ConfigGenerator::generateUi() {
  Ref<Element> ui(new Element("ui"));
  ui->setAttribute("enabled", DEFAULT_UI_EN_VALUE);
  ui->setAttribute("show-tooltips", DEFAULT_UI_SHOW_TOOLTIPS_VALUE);
  Ref<Element> accounts(new Element("accounts"));
  accounts->setAttribute("enabled", DEFAULT_ACCOUNTS_EN_VALUE);
  accounts->setAttribute("session-timeout", std::to_string(DEFAULT_SESSION_TIMEOUT));
  Ref<Element> account(new Element("account"));
  account->setAttribute("user", DEFAULT_ACCOUNT_USER);
  account->setAttribute("password", DEFAULT_ACCOUNT_PASSWORD);
  accounts->appendElementChild(account);
  ui->appendElementChild(accounts);
  return ui;
}

Ref<Element> ConfigGenerator::generateStorage() {
  Ref<Element> storage(new Element("storage"));

#ifdef HAVE_SQLITE3
  Ref<Element> sqlite3(new Element("sqlite3"));
  sqlite3->setAttribute("enabled", DEFAULT_SQLITE_ENABLED);
  sqlite3->appendTextChild("database-file", DEFAULT_SQLITE3_DB_FILENAME);
#ifdef SQLITE_BACKUP_ENABLED
  //    <backup enabled="no" interval="6000"/>
  Ref<Element> backup(new Element("backup"));
  backup->setAttribute("enabled", YES);
  backup->setAttribute("interval", std::to_string(DEFAULT_SQLITE_BACKUP_INTERVAL));
  sqlite3->appendElementChild(backup);
#endif
  storage->appendElementChild(sqlite3);
#endif

#ifdef HAVE_MYSQL
  Ref<Element> mysql(new Element("mysql"));
#ifndef HAVE_SQLITE3
  mysql->setAttribute("enabled", DEFAULT_MYSQL_ENABLED);
  mysql_flag = true;
#else
  mysql->setAttribute("enabled", "no");
#endif
  mysql->appendTextChild("host", DEFAULT_MYSQL_HOST);
  mysql->appendTextChild("username", DEFAULT_MYSQL_USER);
  mysql->appendTextChild("database", DEFAULT_MYSQL_DB);

  storage->appendElementChild(mysql);
#endif
  return storage;
}

Ref<Element> ConfigGenerator::generateExtendedRuntime() {
  Ref<Element> extended(new Element("extended-runtime-options"));

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
  Ref<Element> ffth(new Element("ffmpegthumbnailer"));
  ffth->setAttribute("enabled", DEFAULT_FFMPEGTHUMBNAILER_ENABLED);
  ffth->appendTextChild("thumbnail-size", std::to_string(DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE));
  ffth->appendTextChild("seek-percentage", std::to_string(DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE));
  ffth->appendTextChild("filmstrip-overlay", DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
  ffth->appendTextChild("workaround-bugs", DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);
  ffth->appendTextChild("image-quality", std::to_string(DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY));
  extended->appendElementChild(ffth);
#endif

  Ref<Element> mark(new Element("mark-played-items"));
  mark->setAttribute("enabled", DEFAULT_MARK_PLAYED_ITEMS_ENABLED);
  mark->setAttribute("suppress-cds-updates", DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
  Ref<Element> mark_string(new Element("string"));
  mark_string->setAttribute("mode", DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE);
  mark_string->setText(DEFAULT_MARK_PLAYED_ITEMS_STRING);
  mark->appendElementChild(mark_string);

  Ref<Element> mark_content_section(new Element("mark"));
  Ref<Element> content_video(new Element("content"));
  content_video->setText(DEFAULT_MARK_PLAYED_CONTENT_VIDEO);
  mark_content_section->appendElementChild(content_video);
  mark->appendElementChild(mark_content_section);
  extended->appendElementChild(mark);

  return extended;
}

Ref<Element> ConfigGenerator::generateImport(std::string prefixDir, std::string magicFile) {
  Ref<Element> import(new Element("import"));
  import->setAttribute("hidden-files", DEFAULT_HIDDEN_FILES_VALUE);

#ifdef HAVE_MAGIC
  if (string_ok(magicFile.c_str())) {
    Ref<Element> magicfile(new Element("magic-file"));
    magicfile->setText(magicFile.c_str());
    import->appendElementChild(magicfile);
  }
#endif

  Ref<Element> scripting(new Element("scripting"));
  scripting->setAttribute("script-charset", DEFAULT_JS_CHARSET);

  Ref<Element> layout(new Element("virtual-layout"));
  layout->setAttribute("type", DEFAULT_LAYOUT_TYPE);

#ifdef HAVE_JS
  std::string script;
  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_IMPORT_SCRIPT;
  layout->appendTextChild("import-script", script);

  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_COMMON_SCRIPT;
  scripting->appendTextChild("common-script", script);

  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_PLAYLISTS_SCRIPT;
  scripting->appendTextChild("playlist-script", script);
#endif

  scripting->appendElementChild(layout);
  import->appendElementChild(scripting);
  import->appendElementChild(generateMappings());
#ifdef ONLINE_SERVICES
  import->appendElementChild(generateOnlineContent());
#endif

  return import;
}

Ref<Element> ConfigGenerator::generateMappings() {
  Ref<Element> mappings(new Element("mappings"));
  Ref<Element> ext2mt(new Element("extension-mimetype"));
  ext2mt->setAttribute("ignore-unknown", DEFAULT_IGNORE_UNKNOWN_EXTENSIONS);
  ext2mt->appendElementChild(map_from_to("mp3", "audio/mpeg"));
  ext2mt->appendElementChild(map_from_to("ogx", "application/ogg"));
  ext2mt->appendElementChild(map_from_to("ogv", "video/ogg"));
  ext2mt->appendElementChild(map_from_to("oga", "audio/ogg"));
  ext2mt->appendElementChild(map_from_to("ogg", "audio/ogg"));
  ext2mt->appendElementChild(map_from_to("ogm", "video/ogg"));
  ext2mt->appendElementChild(map_from_to("asf", "video/x-ms-asf"));
  ext2mt->appendElementChild(map_from_to("asx", "video/x-ms-asf"));
  ext2mt->appendElementChild(map_from_to("wma", "audio/x-ms-wma"));
  ext2mt->appendElementChild(map_from_to("wax", "audio/x-ms-wax"));
  ext2mt->appendElementChild(map_from_to("wmv", "video/x-ms-wmv"));
  ext2mt->appendElementChild(map_from_to("wvx", "video/x-ms-wvx"));
  ext2mt->appendElementChild(map_from_to("wm", "video/x-ms-wm"));
  ext2mt->appendElementChild(map_from_to("wmx", "video/x-ms-wmx"));
  ext2mt->appendElementChild(map_from_to("m3u", "audio/x-mpegurl"));
  ext2mt->appendElementChild(map_from_to("pls", "audio/x-scpls"));
  ext2mt->appendElementChild(map_from_to("flv", "video/x-flv"));
  ext2mt->appendElementChild(map_from_to("mkv", "video/x-matroska"));
  ext2mt->appendElementChild(map_from_to("mka", "audio/x-matroska"));
  ext2mt->appendElementChild(map_from_to("dsf", "audio/x-dsd"));
  ext2mt->appendElementChild(map_from_to("dff", "audio/x-dsd"));
  ext2mt->appendElementChild(map_from_to("wv", "audio/x-wavpack"));

  Ref<Comment> ps3info(new Comment(" Uncomment the line below for PS3 divx support ", true));
  Ref<Comment> ps3avi(new Comment(" <map from=\"avi\" to=\"video/divx\"/> ", true));
  ext2mt->appendChild(RefCast(ps3info, Node));
  ext2mt->appendChild(RefCast(ps3avi, Node));

  Ref<Comment> dsmzinfo(new Comment(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 ", true));
  Ref<Comment> dsmzavi(new Comment(" <map from=\"avi\" to=\"video/avi\"/> ", true));
  ext2mt->appendChild(RefCast(dsmzinfo, Node));
  ext2mt->appendChild(RefCast(dsmzavi, Node));
  mappings->appendElementChild(ext2mt);

  Ref<Element> mtupnp(new Element("mimetype-upnpclass"));
  mtupnp->appendElementChild(map_from_to("audio/*", UPNP_DEFAULT_CLASS_MUSIC_TRACK));
  mtupnp->appendElementChild(map_from_to("video/*", UPNP_DEFAULT_CLASS_VIDEO_ITEM));
  mtupnp->appendElementChild(map_from_to("image/*", "object.item.imageItem"));
  mtupnp->appendElementChild(map_from_to("application/ogg", UPNP_DEFAULT_CLASS_MUSIC_TRACK));
  mappings->appendElementChild(mtupnp);

  Ref<Element> mtcontent(new Element("mimetype-contenttype"));
  mtcontent->appendElementChild(treat_as("audio/mpeg", CONTENT_TYPE_MP3));
  mtcontent->appendElementChild(treat_as("application/ogg", CONTENT_TYPE_OGG));
  mtcontent->appendElementChild(treat_as("audio/ogg", CONTENT_TYPE_OGG));
  mtcontent->appendElementChild(treat_as("audio/x-flac", CONTENT_TYPE_FLAC));
  mtcontent->appendElementChild(treat_as("audio/flac", CONTENT_TYPE_FLAC));
  mtcontent->appendElementChild(treat_as("audio/x-ms-wma", CONTENT_TYPE_WMA));
  mtcontent->appendElementChild(treat_as("audio/x-wavpack", CONTENT_TYPE_WAVPACK));
  mtcontent->appendElementChild(treat_as("image/jpeg", CONTENT_TYPE_JPG));
  mtcontent->appendElementChild(treat_as("audio/x-mpegurl", CONTENT_TYPE_PLAYLIST));
  mtcontent->appendElementChild(treat_as("audio/x-scpls", CONTENT_TYPE_PLAYLIST));
  mtcontent->appendElementChild(treat_as("audio/x-wav", CONTENT_TYPE_PCM));
  mtcontent->appendElementChild(treat_as("audio/L16", CONTENT_TYPE_PCM));
  mtcontent->appendElementChild(treat_as("video/x-msvideo", CONTENT_TYPE_AVI));
  mtcontent->appendElementChild(treat_as("video/mp4", CONTENT_TYPE_MP4));
  mtcontent->appendElementChild(treat_as("audio/mp4", CONTENT_TYPE_MP4));
  mtcontent->appendElementChild(treat_as("video/x-matroska", CONTENT_TYPE_MKV));
  mtcontent->appendElementChild(treat_as("audio/x-matroska", CONTENT_TYPE_MKA));
  mtcontent->appendElementChild(treat_as("audio/x-dsd", CONTENT_TYPE_DSD));
  mappings->appendElementChild(mtcontent);

  return mappings;
}

Ref<Element> ConfigGenerator::generateOnlineContent() {
  Ref<Element> onlinecontent(new Element("online-content"));
#ifdef ATRAILERS
  Ref<Element> at(new Element("AppleTrailers"));
  at->setAttribute("enabled", DEFAULT_ATRAILERS_ENABLED);
  at->setAttribute("refresh", std::to_string(DEFAULT_ATRAILERS_REFRESH));
  at->setAttribute("update-at-start", DEFAULT_ATRAILERS_UPDATE_AT_START);
  at->setAttribute("resolution", std::to_string(DEFAULT_ATRAILERS_RESOLUTION));
  onlinecontent->appendElementChild(at);
#endif
  return onlinecontent;
}

Ref<Element> ConfigGenerator::generateTranscoding() {
  Ref<Element> transcoding(new Element("transcoding"));
  transcoding->setAttribute("enabled", DEFAULT_TRANSCODING_ENABLED);

  Ref<Element> mt_prof_map(new Element("mimetype-profile-mappings"));

  Ref<Element> prof_flv(new Element("transcode"));
  prof_flv->setAttribute("mimetype", "video/x-flv");
  prof_flv->setAttribute("using", "vlcmpeg");

  mt_prof_map->appendElementChild(prof_flv);

  Ref<Element> prof_theora(new Element("transcode"));
  prof_theora->setAttribute("mimetype", "application/ogg");
  prof_theora->setAttribute("using", "vlcmpeg");
  mt_prof_map->appendElementChild(prof_theora);

  Ref<Element> prof_ogg(new Element("transcode"));
  prof_ogg->setAttribute("mimetype", "audio/ogg");
  prof_ogg->setAttribute("using", "ogg2mp3");
  mt_prof_map->appendElementChild(prof_ogg);

  transcoding->appendElementChild(mt_prof_map);

  Ref<Element> profiles(new Element("profiles"));

  Ref<Element> oggmp3(new Element("profile"));
  oggmp3->setAttribute("name", "ogg2mp3");
  oggmp3->setAttribute("enabled", NO);
  oggmp3->setAttribute("type", "external");

  oggmp3->appendTextChild("mimetype", "audio/mpeg");
  oggmp3->appendTextChild("accept-url", NO);
  oggmp3->appendTextChild("first-resource", YES);
  oggmp3->appendTextChild("accept-ogg-theora", NO);

  Ref<Element> oggmp3_agent(new Element("agent"));
  oggmp3_agent->setAttribute("command", "ffmpeg");
  oggmp3_agent->setAttribute("arguments", "-y -i %in -f mp3 %out");
  oggmp3->appendElementChild(oggmp3_agent);

  Ref<Element> oggmp3_buffer(new Element("buffer"));
  oggmp3_buffer->setAttribute("size", std::to_string(DEFAULT_AUDIO_BUFFER_SIZE));
  oggmp3_buffer->setAttribute("chunk-size", std::to_string(DEFAULT_AUDIO_CHUNK_SIZE));
  oggmp3_buffer->setAttribute("fill-size", std::to_string(DEFAULT_AUDIO_FILL_SIZE));
  oggmp3->appendElementChild(oggmp3_buffer);

  profiles->appendElementChild(oggmp3);

  Ref<Element> vlcmpeg(new Element("profile"));
  vlcmpeg->setAttribute("name", "vlcmpeg");
  vlcmpeg->setAttribute("enabled", NO);
  vlcmpeg->setAttribute("type", "external");

  vlcmpeg->appendTextChild("mimetype", "video/mpeg");
  vlcmpeg->appendTextChild("accept-url", YES);
  vlcmpeg->appendTextChild("first-resource", YES);
  vlcmpeg->appendTextChild("accept-ogg-theora", YES);

  Ref<Element> vlcmpeg_agent(new Element("agent"));
  vlcmpeg_agent->setAttribute("command", "vlc");
  vlcmpeg_agent->setAttribute("arguments", "-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit");
  vlcmpeg->appendElementChild(vlcmpeg_agent);

  Ref<Element> vlcmpeg_buffer(new Element("buffer"));
  vlcmpeg_buffer->setAttribute("size", std::to_string(DEFAULT_VIDEO_BUFFER_SIZE));
  vlcmpeg_buffer->setAttribute("chunk-size", std::to_string(DEFAULT_VIDEO_CHUNK_SIZE));
  vlcmpeg_buffer->setAttribute("fill-size", std::to_string(DEFAULT_VIDEO_FILL_SIZE));
  vlcmpeg->appendElementChild(vlcmpeg_buffer);

  profiles->appendElementChild(vlcmpeg);

  transcoding->appendElementChild(profiles);

  return transcoding;
}

Ref<Element> ConfigGenerator::generateUdn() {
  uuid_t uuid;
#ifdef BSD_NATIVE_UUID
  char *uuid_str;
  uint32_t status;
  uuid_create(&uuid, &status);
  uuid_to_string(&uuid, &uuid_str, &status);
#else
  char uuid_str[37];
  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_str);
#endif

  log_debug("UUID generated: %s\n", uuid_str);
  Ref<Element> udn(new Element("udn"));
  udn->setText("uuid:" + std::string(uuid_str), mxml_string_type);
#ifdef BSD_NATIVE_UUID
  free(uuid_str);
#endif
  return udn;
}

Ref<Element> ConfigGenerator::map_from_to(std::string from, std::string to) {
  Ref<Element> map(new Element("map"));
  map->setAttribute("from", from);
  map->setAttribute("to", to);
  return map;
}

Ref<Element> ConfigGenerator::treat_as(std::string mimetype, std::string as) {
  Ref<Element> treat(new Element("treat"));
  treat->setAttribute("mimetype", mimetype);
  treat->setAttribute("as", as);
  return treat;
}
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
#include <mxml/mxml.h>
#include <common.h>
#include <tools.h>
#include <metadata_handler.h>
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
  Ref<Element> config(new Element(_("config")));
  config->setAttribute(_("version"), String::from(CONFIG_XML_VERSION));
  config->setAttribute(_("xmlns"), _(XML_XMLNS) + CONFIG_XML_VERSION);
  config->setAttribute(_("xmlns:xsi"), _(XML_XMLNS_XSI));
  config->setAttribute(_("xsi:schemaLocation"),
                       _(XML_XMLNS) + CONFIG_XML_VERSION + " " + XML_XMLNS + CONFIG_XML_VERSION + ".xsd");

  Ref<Comment> docinfo(new Comment(_("\n\
     See http://gerbera.io or read the docs for more\n\
     information on creating and using config.xml configration files.\n\
    "),true));

  config->appendChild(RefCast(docinfo, Node));
  config->appendElementChild(generateServer(userHome, configDir, prefixDir));
  config->appendElementChild(generateImport(prefixDir, magicFile));
  config->appendElementChild(generateTranscoding());
  config->indent();

  return std::string((_(XML_HEADER) + config->print() + "\n").c_str());
}

Ref<Element> ConfigGenerator::generateServer(std::string userHome, std::string configDir, std::string prefixDir) {
  Ref<Element> server(new Element(_("server")));

  server->appendElementChild(generateUi());
  server->appendTextChild(_("name"), _(PACKAGE_NAME));

  server->appendElementChild(generateUdn());

  std::string homepath = userHome + DIR_SEPARATOR + configDir;
  server->appendTextChild(_("home"), String::copy(homepath.c_str()));

  std::string webRoot = prefixDir + DIR_SEPARATOR + DEFAULT_WEB_DIR;
  server->appendTextChild(_("webroot"), String::copy(webRoot.c_str()));

  Ref<Comment> aliveinfo(new Comment(
      _("\n\
        How frequently (in seconds) to send ssdp:alive advertisements.\n\
        Minimum alive value accepted is: ")
          + String::from(ALIVE_INTERVAL_MIN) + _("\n\n\
        The advertisement will be sent every (A/2)-30 seconds,\n\
        and will have a cache-control max-age of A where A is\n\
        the value configured here. Ex: A value of 62 will result\n\
        in an SSDP advertisement being sent every second.\n\
    "),
      true));
  server->appendChild(RefCast(aliveinfo, Node));
  server->appendTextChild(_("alive"), String::from(DEFAULT_ALIVE_INTERVAL));

  server->appendElementChild(generateStorage());

  Ref<Element> protocolinfo(new Element(_("protocolInfo")));
  protocolinfo->setAttribute(_("extend"), _(DEFAULT_EXTEND_PROTOCOLINFO));
  protocolinfo->setAttribute(_("dlna-seek"), _(DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK));
  server->appendElementChild(protocolinfo);

  Ref<Comment> ps3protinfo(new Comment(_(" For PS3 support change to \"yes\" ")));
  server->appendChild(RefCast(ps3protinfo, Node));

  Ref<Comment> redinfo(new Comment(_("\n\
       Uncomment the lines below to get rid of jerky avi playback on the\n\
       DSM320 or to enable subtitles support on the DSM units\n\
    "), true));

  Ref<Comment> redsonic(new Comment(_("\n\
    <custom-http-headers>\n\
      <add header=\"X-User-Agent: redsonic\"/>\n\
    </custom-http-headers>\n\
\n\
    <manufacturerURL>redsonic.com</manufacturerURL>\n\
    <modelNumber>105</modelNumber>\n\
    "), true));

  Ref<Comment> tg100info(new Comment(_(" Uncomment the line below if you have a Telegent TG100 "), true));
  Ref<Comment> tg100(new Comment(_("\n\
       <upnp-string-limit>101</upnp-string-limit>\n\
    "), true));

  server->appendChild(RefCast(redinfo, Node));
  server->appendChild(RefCast(redsonic, Node));
  server->appendChild(RefCast(tg100info, Node));
  server->appendChild(RefCast(tg100, Node));

  server->appendElementChild(generateExtendedRuntime());

  return server;
}

Ref<Element> ConfigGenerator::generateUi() {
  Ref<Element> ui(new Element(_("ui")));
  ui->setAttribute(_("enabled"), _(DEFAULT_UI_EN_VALUE));
  ui->setAttribute(_("show-tooltips"), _(DEFAULT_UI_SHOW_TOOLTIPS_VALUE));
  Ref<Element> accounts(new Element(_("accounts")));
  accounts->setAttribute(_("enabled"), _(DEFAULT_ACCOUNTS_EN_VALUE));
  accounts->setAttribute(_("session-timeout"), String::from(DEFAULT_SESSION_TIMEOUT));
  Ref<Element> account(new Element(_("account")));
  account->setAttribute(_("user"), _(DEFAULT_ACCOUNT_USER));
  account->setAttribute(_("password"), _(DEFAULT_ACCOUNT_PASSWORD));
  accounts->appendElementChild(account);
  ui->appendElementChild(accounts);
  return ui;
}

Ref<Element> ConfigGenerator::generateStorage() {
  Ref<Element> storage(new Element(_("storage")));

#ifdef HAVE_SQLITE3
  Ref<Element> sqlite3(new Element(_("sqlite3")));
  sqlite3->setAttribute(_("enabled"), _(DEFAULT_SQLITE_ENABLED));
  sqlite3->appendTextChild(_("database-file"), _(DEFAULT_SQLITE3_DB_FILENAME));
#ifdef SQLITE_BACKUP_ENABLED
  //    <backup enabled="no" interval="6000"/>
  Ref<Element> backup(new Element(_("backup")));
  backup->setAttribute(_("enabled"), _(YES));
  backup->setAttribute(_("interval"), String::from(DEFAULT_SQLITE_BACKUP_INTERVAL));
  sqlite3->appendElementChild(backup);
#endif
  storage->appendElementChild(sqlite3);
#endif

#ifdef HAVE_MYSQL
  Ref<Element> mysql(new Element(_("mysql")));
#ifndef HAVE_SQLITE3
  mysql->setAttribute(_("enabled"), _(DEFAULT_MYSQL_ENABLED));
  mysql_flag = true;
#else
  mysql->setAttribute(_("enabled"), _("no"));
#endif
  mysql->appendTextChild(_("host"), _(DEFAULT_MYSQL_HOST));
  mysql->appendTextChild(_("username"), _(DEFAULT_MYSQL_USER));
  mysql->appendTextChild(_("database"), _(DEFAULT_MYSQL_DB));

  storage->appendElementChild(mysql);
#endif
  return storage;
}

Ref<Element> ConfigGenerator::generateExtendedRuntime() {
  Ref<Element> extended(new Element(_("extended-runtime-options")));

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
  Ref<Element> ffth(new Element(_("ffmpegthumbnailer")));
  ffth->setAttribute(_("enabled"), _(DEFAULT_FFMPEGTHUMBNAILER_ENABLED));
  ffth->appendTextChild(_("thumbnail-size"), String::from(DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE));
  ffth->appendTextChild(_("seek-percentage"), String::from(DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE));
  ffth->appendTextChild(_("filmstrip-overlay"), _(DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY));
  ffth->appendTextChild(_("workaround-bugs"), _(DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS));
  ffth->appendTextChild(_("image-quality"), String::from(DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY));
  extended->appendElementChild(ffth);
#endif

  Ref<Element> mark(new Element(_("mark-played-items")));
  mark->setAttribute(_("enabled"), _(DEFAULT_MARK_PLAYED_ITEMS_ENABLED));
  mark->setAttribute(_("suppress-cds-updates"),
                     _(DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES));
  Ref<Element> mark_string(new Element(_("string")));
  mark_string->setAttribute(_("mode"),
                            _(DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE));
  mark_string->setText(_(DEFAULT_MARK_PLAYED_ITEMS_STRING));
  mark->appendElementChild(mark_string);

  Ref<Element> mark_content_section(new Element(_("mark")));
  Ref<Element> content_video(new Element(_("content")));
  content_video->setText(_(DEFAULT_MARK_PLAYED_CONTENT_VIDEO));
  mark_content_section->appendElementChild(content_video);
  mark->appendElementChild(mark_content_section);
  extended->appendElementChild(mark);

  return extended;
}

Ref<Element> ConfigGenerator::generateImport(std::string prefixDir, std::string magicFile) {
  Ref<Element> import(new Element(_("import")));
  import->setAttribute(_("hidden-files"), _(DEFAULT_HIDDEN_FILES_VALUE));

#ifdef HAVE_MAGIC
  if (string_ok(magicFile.c_str())) {
    Ref<Element> magicfile(new Element(_("magic-file")));
    magicfile->setText(magicFile.c_str());
    import->appendElementChild(magicfile);
  }
#endif

  Ref<Element> scripting(new Element(_("scripting")));
  scripting->setAttribute(_("script-charset"), _(DEFAULT_JS_CHARSET));

  Ref<Element> layout(new Element(_("virtual-layout")));
  layout->setAttribute(_("type"), _(DEFAULT_LAYOUT_TYPE));

#ifdef HAVE_JS
  std::string script;
  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_IMPORT_SCRIPT;
  layout->appendTextChild(_("import-script"), String::copy(script.c_str()));

  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_COMMON_SCRIPT;
  scripting->appendTextChild(_("common-script"), String::copy(script.c_str()));

  script = prefixDir + DIR_SEPARATOR + DEFAULT_JS_DIR + DIR_SEPARATOR + DEFAULT_PLAYLISTS_SCRIPT;
  scripting->appendTextChild(_("playlist-script"), String::copy(script.c_str()));
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
  Ref<Element> mappings(new Element(_("mappings")));
  Ref<Element> ext2mt(new Element(_("extension-mimetype")));
  ext2mt->setAttribute(_("ignore-unknown"), _(DEFAULT_IGNORE_UNKNOWN_EXTENSIONS));
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

  Ref<Comment> ps3info(new Comment(_(" Uncomment the line below for PS3 divx support "), true));
  Ref<Comment> ps3avi(new Comment(_(" <map from=\"avi\" to=\"video/divx\"/> "), true));
  ext2mt->appendChild(RefCast(ps3info, Node));
  ext2mt->appendChild(RefCast(ps3avi, Node));

  Ref<Comment> dsmzinfo(new Comment(_(" Uncomment the line below for D-Link DSM / ZyXEL DMA-1000 "), true));
  Ref<Comment> dsmzavi(new Comment(_(" <map from=\"avi\" to=\"video/avi\"/> "), true));
  ext2mt->appendChild(RefCast(dsmzinfo, Node));
  ext2mt->appendChild(RefCast(dsmzavi, Node));
  mappings->appendElementChild(ext2mt);

  Ref<Element> mtupnp(new Element(_("mimetype-upnpclass")));
  mtupnp->appendElementChild(map_from_to("audio/*", UPNP_DEFAULT_CLASS_MUSIC_TRACK));
  mtupnp->appendElementChild(map_from_to("video/*", UPNP_DEFAULT_CLASS_VIDEO_ITEM));
  mtupnp->appendElementChild(map_from_to("image/*", "object.item.imageItem"));
  mtupnp->appendElementChild(map_from_to("application/ogg", UPNP_DEFAULT_CLASS_MUSIC_TRACK));
  mappings->appendElementChild(mtupnp);

  Ref<Element> mtcontent(new Element(_("mimetype-contenttype")));
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
  Ref<Element> onlinecontent(new Element(_("online-content")));
#ifdef ATRAILERS
  Ref<Element> at(new Element(_("AppleTrailers")));
  at->setAttribute(_("enabled"), _(DEFAULT_ATRAILERS_ENABLED));
  at->setAttribute(_("refresh"), String::from(DEFAULT_ATRAILERS_REFRESH));
  at->setAttribute(_("update-at-start"), _(DEFAULT_ATRAILERS_UPDATE_AT_START));
  at->setAttribute(_("resolution"), String::from(DEFAULT_ATRAILERS_RESOLUTION));
  onlinecontent->appendElementChild(at);
#endif
  return onlinecontent;
}

Ref<Element> ConfigGenerator::generateTranscoding() {
  Ref<Element> transcoding(new Element(_("transcoding")));
  transcoding->setAttribute(_("enabled"), _(DEFAULT_TRANSCODING_ENABLED));

  Ref<Element> mt_prof_map(new Element(_("mimetype-profile-mappings")));

  Ref<Element> prof_flv(new Element(_("transcode")));
  prof_flv->setAttribute(_("mimetype"), _("video/x-flv"));
  prof_flv->setAttribute(_("using"), _("vlcmpeg"));

  mt_prof_map->appendElementChild(prof_flv);

  Ref<Element> prof_theora(new Element(_("transcode")));
  prof_theora->setAttribute(_("mimetype"), _("application/ogg"));
  prof_theora->setAttribute(_("using"), _("vlcmpeg"));
  mt_prof_map->appendElementChild(prof_theora);

  Ref<Element> prof_ogg(new Element(_("transcode")));
  prof_ogg->setAttribute(_("mimetype"), _("audio/ogg"));
  prof_ogg->setAttribute(_("using"), _("ogg2mp3"));
  mt_prof_map->appendElementChild(prof_ogg);

  transcoding->appendElementChild(mt_prof_map);

  Ref<Element> profiles(new Element(_("profiles")));

  Ref<Element> oggmp3(new Element(_("profile")));
  oggmp3->setAttribute(_("name"), _("ogg2mp3"));
  oggmp3->setAttribute(_("enabled"), _(NO));
  oggmp3->setAttribute(_("type"), _("external"));

  oggmp3->appendTextChild(_("mimetype"), _("audio/mpeg"));
  oggmp3->appendTextChild(_("accept-url"), _(NO));
  oggmp3->appendTextChild(_("first-resource"), _(YES));
  oggmp3->appendTextChild(_("accept-ogg-theora"), _(NO));

  Ref<Element> oggmp3_agent(new Element(_("agent")));
  oggmp3_agent->setAttribute(_("command"), _("ffmpeg"));
  oggmp3_agent->setAttribute(_("arguments"), _("-y -i %in -f mp3 %out"));
  oggmp3->appendElementChild(oggmp3_agent);

  Ref<Element> oggmp3_buffer(new Element(_("buffer")));
  oggmp3_buffer->setAttribute(_("size"), String::from(DEFAULT_AUDIO_BUFFER_SIZE));
  oggmp3_buffer->setAttribute(_("chunk-size"), String::from(DEFAULT_AUDIO_CHUNK_SIZE));
  oggmp3_buffer->setAttribute(_("fill-size"), String::from(DEFAULT_AUDIO_FILL_SIZE));
  oggmp3->appendElementChild(oggmp3_buffer);

  profiles->appendElementChild(oggmp3);

  Ref<Element> vlcmpeg(new Element(_("profile")));
  vlcmpeg->setAttribute(_("name"), _("vlcmpeg"));
  vlcmpeg->setAttribute(_("enabled"), _(NO));
  vlcmpeg->setAttribute(_("type"), _("external"));

  vlcmpeg->appendTextChild(_("mimetype"), _("video/mpeg"));
  vlcmpeg->appendTextChild(_("accept-url"), _(YES));
  vlcmpeg->appendTextChild(_("first-resource"), _(YES));
  vlcmpeg->appendTextChild(_("accept-ogg-theora"), _(YES));

  Ref<Element> vlcmpeg_agent(new Element(_("agent")));
  vlcmpeg_agent->setAttribute(_("command"), _("vlc"));
  vlcmpeg_agent->setAttribute(_("arguments"), _("-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit"));
  vlcmpeg->appendElementChild(vlcmpeg_agent);

  Ref<Element> vlcmpeg_buffer(new Element(_("buffer")));
  vlcmpeg_buffer->setAttribute(_("size"), String::from(DEFAULT_VIDEO_BUFFER_SIZE));
  vlcmpeg_buffer->setAttribute(_("chunk-size"), String::from(DEFAULT_VIDEO_CHUNK_SIZE));
  vlcmpeg_buffer->setAttribute(_("fill-size"), String::from(DEFAULT_VIDEO_FILL_SIZE));
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
  Ref<Element> udn(new Element(_("udn")));
  udn->setText(_("uuid:") + uuid_str, mxml_string_type);
#ifdef BSD_NATIVE_UUID
  free(uuid_str);
#endif
  return udn;
}

Ref<Element> ConfigGenerator::map_from_to(std::string from, std::string to) {
  Ref<Element> map(new Element(_("map")));
  map->setAttribute(_("from"), String::copy(from.c_str()));
  map->setAttribute(_("to"), String::copy(to.c_str()));
  return map;
}

Ref<Element> ConfigGenerator::treat_as(std::string mimetype, std::string as) {
  Ref<Element> treat(new Element(_("treat")));
  treat->setAttribute(_("mimetype"), String::copy(mimetype.c_str()));
  treat->setAttribute(_("as"), String::copy(as.c_str()));
  return treat;
}
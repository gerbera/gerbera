/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    common.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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

/// \file common.h
#ifndef __COMMON_H__
#define __COMMON_H__

#include <cerrno>

#include "exceptions.h"
#include "upnp_common.h"
#include "util/logger.h"

#define CONFIG_XML_VERSION 2

// fixed CdsObjectIDs
#define CDS_ID_ROOT 0
#define CDS_ID_FS_ROOT 1
static constexpr bool IS_FORBIDDEN_CDS_ID(int id) { return id <= CDS_ID_FS_ROOT; }

#define YES "yes"
#define NO "no"

// URL FORMATTING CONSTANTS
#define URL_OBJECT_ID "object_id"
#define URL_REQUEST_TYPE "req_type"
#define URL_RESOURCE_ID "res_id"
#define URL_FILE_EXTENSION "ext"
#define URL_PARAM_SEPARATOR '/'
#define _URL_PARAM_SEPARATOR "/"
#define URL_UI_PARAM_SEPARATOR '?'
#define SERVER_VIRTUAL_DIR "content"
#define CONTENT_MEDIA_HANDLER "media"
#define CONTENT_SERVE_HANDLER "serve"
#define CONTENT_ONLINE_HANDLER "online"
#define CONTENT_UI_HANDLER "interface"
#define DEVICE_DESCRIPTION_PATH "description.xml"

// SEPARATOR
#define FS_ROOT_DIRECTORY "/"
#define DIR_SEPARATOR '/'
#define VIRTUAL_CONTAINER_SEPARATOR '/'
#define VIRTUAL_CONTAINER_ESCAPE '\\'

// MIME TYPES FOR WEB UI
#define MIMETYPE_XML "text/xml"
#define MIMETYPE_TEXT "text/plain"
#define MIMETYPE_JSON "application/json" // RFC 4627
// default mime types for items in the cds
#define MIMETYPE_DEFAULT "application/octet-stream"

// default protocol
#define PROTOCOL "http-get"

// device description defaults
#define DESC_FRIENDLY_NAME "Gerbera"
#define DESC_MANUFACTURER "Gerbera Contributors"
#define DESC_MANUFACTURER_URL "http://gerbera.io/"
#define DESC_MODEL_DESCRIPTION "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME "Gerbera"
#define DESC_MODEL_NUMBER GERBERA_VERSION
#define DESC_SERIAL_NUMBER "1"

// default values
#define DEFAULT_INTERNAL_CHARSET "UTF-8"
#define DEFAULT_FILESYSTEM_CHARSET "UTF-8"
#define DEFAULT_FALLBACK_CHARSET "US-ASCII"
#define DEFAULT_JS_CHARSET "UTF-8"

#define DEFAULT_CONFIG_HOME ".config/gerbera"
#define DEFAULT_TMPDIR "/tmp/"
#define DEFAULT_CONFIG_NAME "config.xml"
#define DEFAULT_UI_EN_VALUE YES
#define DEFAULT_UI_SHOW_TOOLTIPS_VALUE YES
#define DEFAULT_POLL_WHEN_IDLE_VALUE NO
#define DEFAULT_POLL_INTERVAL 2
#define DEFAULT_ACCOUNTS_EN_VALUE NO
#define DEFAULT_ACCOUNT_USER "gerbera"
#define DEFAULT_ACCOUNT_PASSWORD "gerbera"
#define DEFAULT_ALIVE_INTERVAL 1800 // seconds
#define ALIVE_INTERVAL_MIN 62 // seconds
#define DEFAULT_BOOKMARK_FILE "gerbera.html"
#define DEFAULT_IGNORE_UNKNOWN_EXTENSIONS NO
#define DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS NO
#define DEFAULT_IMPORT_LAYOUT_PARENT_PATH NO
#define DEFAULT_IMPORT_SCRIPT "import.js"
#define DEFAULT_PLAYLISTS_SCRIPT "playlists.js"
#define DEFAULT_PLAYLIST_CREATE_LINK YES
#define DEFAULT_COMMON_SCRIPT "common.js"
#define DEFAULT_WEB_DIR "web"
#define DEFAULT_JS_DIR "js"
#define DEFAULT_HIDDEN_FILES_VALUE NO
#define DEFAULT_FOLLOW_SYMLINKS_VALUE YES
#define DEFAULT_RESOURCES_CASE_SENSITIVE YES
#define DEFAULT_UPNP_STRING_LIMIT (-1)
#define DEFAULT_SESSION_TIMEOUT 30
#define SESSION_TIMEOUT_CHECK_INTERVAL (5 * 60)
#define DEFAULT_PRES_URL_APPENDTO_ATTR "none"
#define DEFAULT_ITEMS_PER_PAGE_1 10
#define DEFAULT_ITEMS_PER_PAGE_2 25
#define DEFAULT_ITEMS_PER_PAGE_3 50
#define DEFAULT_ITEMS_PER_PAGE_4 100
#define DEFAULT_LAYOUT_TYPE "builtin"
#define DEFAULT_HIDE_PC_DIRECTORY NO
#define DEFAULT_CLIENTS_EN_VALUE NO

#ifdef SOPCAST
#define DEFAULT_SOPCAST_ENABLED NO
#define DEFAULT_SOPCAST_UPDATE_AT_START NO
#endif

#ifdef ATRAILERS
#define DEFAULT_ATRAILERS_ENABLED NO
#define DEFAULT_ATRAILERS_UPDATE_AT_START NO
#define DEFAULT_ATRAILERS_REFRESH 43200
#define DEFAULT_ATRAILERS_RESOLUTION 640
#endif

#ifdef ONLINE_SERVICES
#define CFG_DEFAULT_UPDATE_AT_START 10 // seconds
#endif

#define DEFAULT_TRANSCODING_ENABLED NO
#define DEFAULT_AUDIO_BUFFER_SIZE 1048576
#define DEFAULT_AUDIO_CHUNK_SIZE 131072
#define DEFAULT_AUDIO_FILL_SIZE 262144

#define DEFAULT_VIDEO_BUFFER_SIZE 14400000
#define DEFAULT_VIDEO_CHUNK_SIZE 512000
#define DEFAULT_VIDEO_FILL_SIZE 120000

#define URL_PARAM_TRANSCODE_PROFILE_NAME "pr_name"
#define URL_PARAM_TRANSCODE "tr"
#define URL_VALUE_TRANSCODE_NO_RES_ID "none"

#define URL_VALUE_TRANSCODE "1"
#define MT_SQLITE_SYNC_FULL 2
#define MT_SQLITE_SYNC_NORMAL 1
#define MT_SQLITE_SYNC_OFF 0
#define DEFAULT_SQLITE_SYNC "off"
#define DEFAULT_SQLITE_RESTORE "restore"
#define DEFAULT_SQLITE_BACKUP_ENABLED NO
#define DEFAULT_SQLITE_BACKUP_INTERVAL 600
#define DEFAULT_SQLITE_ENABLED YES

#ifdef HAVE_MYSQL
#define DEFAULT_MYSQL_HOST "localhost"
#define DEFAULT_MYSQL_DB "gerbera"
#define DEFAULT_MYSQL_USER "gerbera"
#define DEFAULT_MYSQL_ENABLED NO

#else //HAVE_MYSQL
#define DEFAULT_MYSQL_ENABLED NO
#endif

#define DEFAULT_SQLITE3_DB_FILENAME "gerbera.db"

// misc
#define INVALID_OBJECT_ID (-333)
#define INVALID_OBJECT_ID_2 (-666)
#define CHECK_SOCKET (-666)

// database
#define LOC_DIR_PREFIX 'D'
#define LOC_FILE_PREFIX 'F'
#define LOC_VIRT_PREFIX 'V'
#define LOC_ILLEGAL_PREFIX 'X'

#ifndef DEFAULT_JS_RUNTIME_MEM
#define DEFAULT_JS_RUNTIME_MEM (1L * 1024L * 1024L)
#endif

#define XML_XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XMLNS "http://mediatomb.cc/config/"

#ifdef HAVE_CURL
#define DEFAULT_CURL_BUFFER_SIZE 262144
#define DEFAULT_CURL_INITIAL_FILL_SIZE 0
#endif

#define DEFAULT_LIBOPTS_ENTRY_SEPARATOR "; "

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
#define DEFAULT_FFMPEGTHUMBNAILER_ENABLED NO
#define DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE 128
#define DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE 5
#define DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY YES
#define DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS NO
#define DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY 8
#define DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED YES
#define DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR ""
#endif

#if defined(HAVE_LASTFMLIB)
#define DEFAULT_LASTFM_ENABLED NO
#define DEFAULT_LASTFM_USERNAME "lastfmuser"
#define DEFAULT_LASTFM_PASSWORD "lastfmpass"
#endif

#define DEFAULT_MARK_PLAYED_ITEMS_ENABLED NO
#define DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES YES
#define DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE "prepend"
#define DEFAULT_MARK_PLAYED_ITEMS_STRING "*"
#define DEFAULT_MARK_PLAYED_CONTENT_AUDIO "audio"
#define DEFAULT_MARK_PLAYED_CONTENT_VIDEO "video"
#define DEFAULT_MARK_PLAYED_CONTENT_IMAGE "image"

#define LINK_FILE_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_MEDIA_HANDLER "/"
#define LINK_URL_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_ONLINE_HANDLER "/"

#endif // __COMMON_H__

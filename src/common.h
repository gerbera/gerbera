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
#define DESC_MANUFACTURER_URL "http://gerbera.io/"

// default values
#define DEFAULT_INTERNAL_CHARSET "UTF-8"
#define DEFAULT_FILESYSTEM_CHARSET "UTF-8"
#define DEFAULT_FALLBACK_CHARSET "US-ASCII"

#define DEFAULT_CONFIG_HOME ".config/gerbera"
#define DEFAULT_CONFIG_NAME "config.xml"
#define DEFAULT_ACCOUNT_USER "gerbera"
#define DEFAULT_ACCOUNT_PASSWORD "gerbera"
#define ALIVE_INTERVAL_MIN 62 // seconds
#define DEFAULT_IMPORT_SCRIPT "import.js"
#define DEFAULT_PLAYLISTS_SCRIPT "playlists.js"
#define DEFAULT_COMMON_SCRIPT "common.js"
#define DEFAULT_WEB_DIR "web"
#define DEFAULT_JS_DIR "js"
static constexpr auto SESSION_TIMEOUT_CHECK_INTERVAL = std::chrono::minutes(5);

#ifdef ONLINE_SERVICES
static constexpr auto CFG_DEFAULT_UPDATE_AT_START = std::chrono::seconds(10);
#endif

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

// misc
#define INVALID_OBJECT_ID (-333)
#define INVALID_OBJECT_ID_2 (-666)
#define CHECK_SOCKET (-666)

// database
#define LOC_DIR_PREFIX 'D'
#define LOC_FILE_PREFIX 'F'
#define LOC_VIRT_PREFIX 'V'
#define LOC_ILLEGAL_PREFIX 'X'

#define XML_XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XMLNS "http://mediatomb.cc/config/"

#define DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE "prepend"
#define DEFAULT_MARK_PLAYED_CONTENT_AUDIO "audio"
#define DEFAULT_MARK_PLAYED_CONTENT_VIDEO "video"
#define DEFAULT_MARK_PLAYED_CONTENT_IMAGE "image"

#define LINK_FILE_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_MEDIA_HANDLER "/"
#define LINK_URL_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_ONLINE_HANDLER "/"

#endif // __COMMON_H__

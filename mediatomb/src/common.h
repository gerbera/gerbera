/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    common.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
#if defined(HAVE_CONFIG_H)
    #include "autoconfig.h"
#endif

#ifndef __COMMON_H__
#define __COMMON_H__

#include <errno.h>

#include "memory.h"
#include "logger.h"
#include "zmmf/zmmf.h"
#include "exceptions.h"

// ERROR CODES                                                                                 
/// \brief UPnP specific error code.
#define UPNP_E_ACTION_FAILED            501
#define UPNP_E_SUBSCRIPTION_FAILED      503
/// \brief UPnP specific error code.
#define UPNP_E_NO_SUCH_ID               701
#define UPNP_E_NOT_EXIST                706

// UPnP default classes
#define UPNP_DEFAULT_CLASS_CONTAINER    "object.container"
#define UPNP_DEFAULT_CLASS_ITEM         "object.item"
#define UPNP_DEFAULT_CLASS_ACTIVE_ITEM  "object.item.activeItem"
#define UPNP_DEFAULT_CLASS_MUSIC_ALBUM  "object.container.album.musicAlbum"
#define UPNP_DEFAULT_CLASS_MUSIC_TRACK  "object.item.audioItem.musicTrack"
#define UPNP_DEFAULT_CLASS_MUSIC_CONT   "object.container.musicContainer"
#define UPNP_DEFAULT_CLASS_MUSIC_GENRE  "object.container.genre.musicGenre"
#define UPNP_DEFAULT_CLASS_MUSIC_ARTIST "object.container.person.musicArtist"
#define UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER "object.container.playlistContainer"

#define D_PROFILE                       "DLNA.ORG_PN"
#define D_CONVERSION_INDICATOR          "DLNA.ORG_CI"
#define D_DEFAULT_OPS                   "DLNA.ORG_OP=01"
#define D_NO_CONVERSION                 "0"
#define D_CONVERSION                    "1"
#define D_MP3                           "MP3"

// fixed CdsObjectIDs
#define CDS_ID_BLACKHOLE                -1
#define CDS_ID_ROOT                     0
#define CDS_ID_FS_ROOT                  1
#define IS_FORBIDDEN_CDS_ID(id)         (id <= CDS_ID_FS_ROOT)

// internal setting keys
#define SET_LAST_MODIFIED               "last_modified"

#define YES                             "yes"
#define NO                              "no"

// URL FORMATTING CONSTANTS
#define URL_OBJECT_ID                   "object_id"
#define URL_REQUEST_TYPE                "req_type"
#define URL_RESOURCE_ID                 "res_id"
#define URL_FILE_EXTENSION              "ext"
#define URL_PARAM_SEPARATOR             '/' 
#define _URL_PARAM_SEPARATOR            "/"
#define URL_UI_PARAM_SEPARATOR          '?'
#define URL_ARG_SEPARATOR               '&'
#define _URL_ARG_SEPARATOR              "&"
#define SERVER_VIRTUAL_DIR              "content"
#define CONTENT_MEDIA_HANDLER           "media"
#define CONTENT_SERVE_HANDLER           "serve"
#define CONTENT_UI_HANDLER              "interface"
#define URL_PARAM_TRANSCODE_PROFILE_NAME "pr_name"
#define URL_PARAM_TRANSCODE             "tr"
#define URL_PARAM_TRANSCODE_TARGET_MIMETYPE "tmt"
// REQUEST TYPES
#define REQ_TYPE_BROWSE                 "browse"
#define REQ_TYPE_LOGIN                  "login"

// SEPARATOR
#define DIR_SEPARATOR                   '/'
#define _DIR_SEPARATOR                  "/"
#define VIRTUAL_CONTAINER_SEPARATOR     '/'
#define VIRTUAL_CONTAINER_ESCAPE        '\\'
// MIME TYPES FOR WEB UI
#define MIMETYPE_XML                   "text/xml"
#define MIMETYPE_HTML                  "text/html"
#define MIMETYPE_TEXT                  "text/plain"
// default mime types for items in the cds
#define MIMETYPE_DEFAULT               "application/octet-stream"

// regexp for mimetype matching
#define MIMETYPE_REGEXP                 "^([a-z0-9_-]+/[a-z0-9_-]+)"

// default protocol 
#define PROTOCOL                        "http-get"

// device description defaults
#define DESC_DEVICE_NAMESPACE           "urn:schemas-upnp-org:device-1-0"
#define DESC_DEVICE_TYPE                "urn:schemas-upnp-org:device:MediaServer:1"
#define DESC_SPEC_VERSION_MAJOR         "1"
#define DESC_SPEC_VERSION_MINOR         "0"
#define DESC_FRIENDLY_NAME              PACKAGE_NAME 
#define DESC_MANUFACTURER               "(c) 2004-2007 Gena Batyan <bgeradz@mediatomb.cc>, Sergey Bostandzhyan <jin@mediatomb.cc>, Leonhard Wimmer <leo@mediatomb.cc>"
#define DESC_MANUFACTURER_URL           "http://mediatomb.cc/"
#define DESC_MODEL_DESCRIPTION          "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME                 PACKAGE_NAME
#define DESC_MODEL_NUMBER               VERSION 
#define DESC_SERIAL_NUMBER              "1"

//services
// connection manager
#define DESC_CM_SERVICE_TYPE            "urn:schemas-upnp-org:service:ConnectionManager:1"
#define DESC_CM_SERVICE_ID              "urn:schemas-upnp-org:serviceId:ConnectionManager"
#define DESC_CM_SCPD_URL                "cm.xml"
#define DESC_CM_CONTROL_URL             "/upnp/control/cm"
#define DESC_CM_EVENT_URL               "/upnp/event/cm"

// content directory
#define DESC_CDS_SERVICE_TYPE           "urn:schemas-upnp-org:service:ContentDirectory:1"
#define DESC_CDS_SERVICE_ID             "urn:schemas-upnp-org:serviceId:ContentDirectory"
#define DESC_CDS_SCPD_URL               "cds.xml"
#define DESC_CDS_CONTROL_URL            "/upnp/control/cds"
#define DESC_CDS_EVENT_URL              "/upnp/event/cds"

#define DESC_ICON_PNG_MIMETYPE          "image/png"
#define DESC_ICON_BMP_MIMETYPE          "image/x-ms-bmp"
#define DESC_ICON_JPG_MIMETYPE          "image/jpeg"
#define DESC_ICON32_PNG                 "/icons/mt-icon32.png"
#define DESC_ICON32_BMP                 "/icons/mt-icon32.bmp"
#define DESC_ICON32_JPG                 "/icons/mt-icon32.jpg"
#define DESC_ICON48_PNG                 "/icons/mt-icon48.png"
#define DESC_ICON48_BMP                 "/icons/mt-icon48.bmp"
#define DESC_ICON48_JPG                 "/icons/mt-icon48.jpg"
#define DESC_ICON120_JPG                "/icons/mt-icon120.jpg"
#define DESC_ICON120_PNG                "/icons/mt-icon120.png"
#define DESC_ICON120_BMP                "/icons/mt-icon120.bmp"


#if defined(ENABLE_MRREG)
    // media receiver registrar (xbox 360)
    #define DESC_MRREG_SERVICE_TYPE     "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"
    #define DESC_MRREG_SERVICE_ID       "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
    #define DESC_MRREG_SCPD_URL         "mr_reg.xml"
    #define DESC_MRREG_CONTROL_URL      "/upnp/control/mr_reg"
    #define DESC_MRREG_EVENT_URL        "/upnp/event/mr_reg"
#endif

// xml namespaces
#define XML_NAMESPACE_ATTR              "xmlns"
#define XML_DIDL_LITE_NAMESPACE         "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define XML_DC_NAMESPACE_ATTR           "xmlns:dc"
#define XML_DC_NAMESPACE                "http://purl.org/dc/elements/1.1/"
#define XML_UPNP_NAMESPACE_ATTR         "xmlns:upnp"
#define XML_UPNP_NAMESPACE              "urn:schemas-upnp-org:metadata-1-0/upnp/"

// default values
#define DEFAULT_INTERNAL_CHARSET        "UTF-8"
#define DEFAULT_FILESYSTEM_CHARSET      "ISO-8859-1"
#define DEFAULT_FALLBACK_CHARSET        "US-ASCII"
#define DEFAULT_JS_CHARSET              "UTF-8"

#if defined(__CYGWIN__)
    #define DEFAULT_CONFIG_HOME         "MediaTomb"
#else
    #define DEFAULT_CONFIG_HOME         ".mediatomb"
#endif
#define DEFAULT_SYSTEM_HOME             "mediatomb"
#define DEFAULT_ETC                     "etc"
#define DEFAULT_CONFIG_NAME             "config.xml"
#define DEFAULT_UI_EN_VALUE             YES
#define DEFAULT_POLL_WHEN_IDLE_VALUE    NO
#define DEFAULT_POLL_INTERVAL           2 
#define DEFAULT_ACCOUNTS_EN_VALUE       NO
#define DEFAULT_ALIVE_INTERVAL          180 // seconds
#define DEFAULT_BOOKMARK_FILE           "mediatomb.html"
#define DEFAULT_IGNORE_UNKNOWN_EXTENSIONS NO
#define DEFAULT_IMPORT_SCRIPT           "import.js"
#define DEFAULT_PLAYLISTS_SCRIPT        "playlists.js"
#define DEFAULT_PLAYLIST_CREATE_LINK    YES
#define DEFAULT_COMMON_SCRIPT           "common.js"
#define DEFAULT_MYSQL_HOST              "localhost"
#define DEFAULT_MYSQL_DB                "mediatomb"
#define DEFAULT_MYSQL_USER              "mediatomb"
#define DEFAULT_WEB_DIR                 "web"
#define DEFAULT_JS_DIR                  "js"
#define DEFAULT_HIDDEN_FILES_VALUE      NO
#define DEFAULT_UPNP_STRING_LIMIT       (-1)
#define DEFAULT_SESSION_TIMEOUT         30
#define SESSION_TIMEOUT_CHECK_INTERVAL  (5 * 60)
#define DEFAULT_PRES_URL_APPENDTO_ATTR  "none"
#define DEFAULT_ITEMS_PER_PAGE_1        10
#define DEFAULT_ITEMS_PER_PAGE_2        25
#define DEFAULT_ITEMS_PER_PAGE_3        50
#define DEFAULT_ITEMS_PER_PAGE_4        100
#define DEFAULT_LAYOUT_TYPE             "builtin" 
#define DEFAULT_EXTEND_PROTOCOLINFO     NO
#define DEFAULT_HIDE_PC_DIRECTORY       NO
#define DEFAULT_TIMEOUT_RETRIES         0
#ifdef HAVE_SQLITE3
    #define DEFAULT_STORAGE_DRIVER      "sqlite3"
#else
    #define DEFAULT_STORAGE_DRIVER      "mysql"
#endif

#define DEFAULT_SQLITE3_DB_FILENAME     "mediatomb.db"

#define CONFIG_MAPPINGS_TEMPLATE        "mappings.xml"
// misc
#define INVALID_OBJECT_ID               (-333)
#define INVALID_OBJECT_ID_2             (-666)

#ifndef DEFAULT_JS_RUNTIME_MEM
    #define DEFAULT_JS_RUNTIME_MEM      (1L * 1024L * 1024L)
#endif

#define XML_HEADER                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
#define XML_XMLNS                         "http://mediatomb.cc/" VERSION "/config"
#define XML_XMLNS_XSI                     "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XSI_SCHEMA_LOCATION           "http://mediatomb.cc/" VERSION "/config http://mediatomb.cc/" VERSION "/config.xsd"

#endif // __COMMON_H__

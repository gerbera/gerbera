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

#include <errno.h>

#include "exceptions.h"
#include "logger.h"
#include "memory.h"
#include "zmm/zmmf.h"

#define CONFIG_XML_VERSION_0_11_0 1
#define CONFIG_XML_VERSION 2

// ERROR CODES
/// \brief UPnP specific error code.
#define UPNP_E_ACTION_FAILED 501
#define UPNP_E_SUBSCRIPTION_FAILED 503
/// \brief UPnP specific error code.
#define UPNP_E_NO_SUCH_ID 701
#define UPNP_E_NOT_EXIST 706

// UPnP default classes
#define UPNP_DEFAULT_CLASS_CONTAINER "object.container"
#define UPNP_DEFAULT_CLASS_ITEM "object.item"
#define UPNP_DEFAULT_CLASS_VIDEO_ITEM "object.item.videoItem"
#define UPNP_DEFAULT_CLASS_IMAGE_ITEM "object.item.imageItem"
#define UPNP_DEFAULT_CLASS_ACTIVE_ITEM "object.item.activeItem"
#define UPNP_DEFAULT_CLASS_MUSIC_ALBUM "object.container.album.musicAlbum"
#define UPNP_DEFAULT_CLASS_MUSIC_TRACK "object.item.audioItem.musicTrack"
#define UPNP_DEFAULT_CLASS_MUSIC_GENRE "object.container.genre.musicGenre"
#define UPNP_DEFAULT_CLASS_MUSIC_ARTIST "object.container.person.musicArtist"
#define UPNP_DEFAULT_CLASS_MUSIC_COMPOSER "object.container.person.musicComposer"
#define UPNP_DEFAULT_CLASS_MUSIC_CONDUCTOR "object.container.person.musicConductor"
#define UPNP_DEFAULT_CLASS_MUSIC_ORCHESTRA "object.container.person.musicOrchestra"
#define UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER "object.container.playlistContainer"
#define UPNP_DEFAULT_CLASS_VIDEO_BROADCAST "object.item.videoItem.videoBroadcast"

#define D_HTTP_TRANSFER_MODE_HEADER "transferMode.dlna.org: "
#define D_HTTP_TRANSFER_MODE_STREAMING "Streaming"
#define D_HTTP_TRANSFER_MODE_INTERACTIVE "Interactive"
#define D_HTTP_CONTENT_FEATURES_HEADER "contentFeatures.dlna.org: "

#define D_PROFILE "DLNA.ORG_PN"
#define D_CONVERSION_INDICATOR "DLNA.ORG_CI"
#define D_NO_CONVERSION "0"
#define D_CONVERSION "1"
#define D_OP "DLNA.ORG_OP"
// all seek modes
#define D_OP_SEEK_RANGE "01"
#define D_OP_SEEK_TIME "10"
#define D_OP_SEEK_BOTH "11"
#define D_OP_SEEK_DISABLED "00"
// since we are using only range seek
#define D_OP_SEEK_ENABLED D_OP_SEEK_RANGE
#define D_FLAGS "DLNA.ORG_FLAGS"
#define D_TR_FLAGS_AV "01200000000000000000000000000000"
#define D_TR_FLAGS_IMAGE "00800000000000000000000000000000"
#define D_MP3 "MP3"
#define D_LPCM "LPCM"
#define D_JPEG_SM "JPEG_SM"
#define D_JPEG_MED "JPEG_MED"
#define D_JPEG_LRG "JPEG_LRG"
#define D_JPEG_TN "JPEG_TN"
#define D_JPEG_SM_ICO "JPEG_SM_ICO"
#define D_JPEG_LRG_ICO "JPEG_LRG_ICO"
#define D_PN_MPEG_PS_PAL "MPEG_PS_PAL"
#define D_PN_MKV "MKV"
#define D_PN_AVC_MP4_EU "AVC_MP4_EU"
#define D_PN_AVI "AVI"
// fixed CdsObjectIDs
#define CDS_ID_BLACKHOLE -1
#define CDS_ID_ROOT 0
#define CDS_ID_FS_ROOT 1
#define IS_FORBIDDEN_CDS_ID(id) (id <= CDS_ID_FS_ROOT)

// internal setting keys
#define SET_LAST_MODIFIED "last_modified"

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
#define URL_ARG_SEPARATOR '&'
#define _URL_ARG_SEPARATOR "&"
#define SERVER_VIRTUAL_DIR "content"
#define CONTENT_MEDIA_HANDLER "media"
#define CONTENT_SERVE_HANDLER "serve"
#define CONTENT_ONLINE_HANDLER "online"
#define CONTENT_UI_HANDLER "interface"
// REQUEST TYPES
#define REQ_TYPE_BROWSE "browse"
#define REQ_TYPE_LOGIN "login"

// SEPARATOR
#define DIR_SEPARATOR '/'
#define _DIR_SEPARATOR "/"
#define VIRTUAL_CONTAINER_SEPARATOR '/'
#define VIRTUAL_CONTAINER_ESCAPE '\\'
// MIME TYPES FOR WEB UI
#define MIMETYPE_XML "text/xml"
#define MIMETYPE_HTML "text/html"
#define MIMETYPE_TEXT "text/plain"
#define MIMETYPE_JSON "application/json" // RFC 4627
// default mime types for items in the cds
#define MIMETYPE_DEFAULT "application/octet-stream"

// regexp for mimetype matching
#define MIMETYPE_REGEXP "^([a-z0-9_-]+/[a-z0-9_-]+)"

// default protocol
#define PROTOCOL "http-get"

// device description defaults
#define DESC_DEVICE_NAMESPACE "urn:schemas-upnp-org:device-1-0"
#define DESC_DEVICE_TYPE "urn:schemas-upnp-org:device:MediaServer:1"
#define DESC_SPEC_VERSION_MAJOR "1"
#define DESC_SPEC_VERSION_MINOR "0"
#define DESC_FRIENDLY_NAME PACKAGE_NAME
#define DESC_MANUFACTURER "Gerbera Contributors"
#define DESC_MANUFACTURER_URL "http://gerbera.io/"
#define DESC_MODEL_DESCRIPTION "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME PACKAGE_NAME
#define DESC_MODEL_NUMBER VERSION
#define DESC_SERIAL_NUMBER "1"

//services
// connection manager
#define DESC_CM_SERVICE_TYPE "urn:schemas-upnp-org:service:ConnectionManager:1"
#define DESC_CM_SERVICE_ID "urn:upnp-org:serviceId:ConnectionManager"
#define DESC_CM_SCPD_URL "cm.xml"
#define DESC_CM_CONTROL_URL "/upnp/control/cm"
#define DESC_CM_EVENT_URL "/upnp/event/cm"

// content directory
#define DESC_CDS_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"
#define DESC_CDS_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"
#define DESC_CDS_SCPD_URL "cds.xml"
#define DESC_CDS_CONTROL_URL "/upnp/control/cds"
#define DESC_CDS_EVENT_URL "/upnp/event/cds"

#define DESC_ICON_PNG_MIMETYPE "image/png"
#define DESC_ICON_BMP_MIMETYPE "image/x-ms-bmp"
#define DESC_ICON_JPG_MIMETYPE "image/jpeg"
#define DESC_ICON32_PNG "/icons/mt-icon32.png"
#define DESC_ICON32_BMP "/icons/mt-icon32.bmp"
#define DESC_ICON32_JPG "/icons/mt-icon32.jpg"
#define DESC_ICON48_PNG "/icons/mt-icon48.png"
#define DESC_ICON48_BMP "/icons/mt-icon48.bmp"
#define DESC_ICON48_JPG "/icons/mt-icon48.jpg"
#define DESC_ICON120_JPG "/icons/mt-icon120.jpg"
#define DESC_ICON120_PNG "/icons/mt-icon120.png"
#define DESC_ICON120_BMP "/icons/mt-icon120.bmp"

// media receiver registrar (xbox 360)
#define DESC_MRREG_SERVICE_TYPE "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"
#define DESC_MRREG_SERVICE_ID "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
#define DESC_MRREG_SCPD_URL "mr_reg.xml"
#define DESC_MRREG_CONTROL_URL "/upnp/control/mr_reg"
#define DESC_MRREG_EVENT_URL "/upnp/event/mr_reg"

// xml namespaces
#define XML_NAMESPACE_ATTR "xmlns"
#define XML_DIDL_LITE_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define XML_DC_NAMESPACE_ATTR "xmlns:dc"
#define XML_DC_NAMESPACE "http://purl.org/dc/elements/1.1/"
#define XML_UPNP_NAMESPACE_ATTR "xmlns:upnp"
#define XML_UPNP_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/upnp/"
#define XML_SEC_NAMESPACE_ATTR "xmlns:sec"
#define XML_SEC_NAMESPACE "http://www.sec.co.kr/"

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
#define DEFAULT_IMPORT_SCRIPT "import.js"
#define DEFAULT_PLAYLISTS_SCRIPT "playlists.js"
#define DEFAULT_PLAYLIST_CREATE_LINK YES
#define DEFAULT_COMMON_SCRIPT "common.js"
#define DEFAULT_WEB_DIR "web"
#define DEFAULT_JS_DIR "js"
#define DEFAULT_HIDDEN_FILES_VALUE NO
#define DEFAULT_UPNP_STRING_LIMIT (-1)
#define DEFAULT_SESSION_TIMEOUT 30
#define SESSION_TIMEOUT_CHECK_INTERVAL (5 * 60)
#define DEFAULT_PRES_URL_APPENDTO_ATTR "none"
#define DEFAULT_ITEMS_PER_PAGE_1 10
#define DEFAULT_ITEMS_PER_PAGE_2 25
#define DEFAULT_ITEMS_PER_PAGE_3 50
#define DEFAULT_ITEMS_PER_PAGE_4 100
#define DEFAULT_LAYOUT_TYPE "builtin"
#define DEFAULT_EXTEND_PROTOCOLINFO NO
#define DEFAULT_EXTEND_PROTOCOLINFO_SM_HACK NO
#define DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK YES
#define DEFAULT_HIDE_PC_DIRECTORY NO
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
#define URL_PARAM_TRANSCODE_TARGET_MIMETYPE "tmt"
#define URL_VALUE_TRANSCODE_NO_RES_ID "none"

#define URL_VALUE_TRANSCODE "1"
#define DEFAULT_STORAGE_CACHING_ENABLED YES
#ifdef HAVE_SQLITE3
#define MT_SQLITE_SYNC_FULL 2
#define MT_SQLITE_SYNC_NORMAL 1
#define MT_SQLITE_SYNC_OFF 0
#define DEFAULT_SQLITE_SYNC "off"
#define DEFAULT_SQLITE_RESTORE "restore"
#define DEFAULT_SQLITE_BACKUP_ENABLED NO
#define DEFAULT_SQLITE_BACKUP_INTERVAL 600
#define DEFAULT_SQLITE_ENABLED YES
#define DEFAULT_STORAGE_DRIVER "sqlite3"
#else
#define DEFAULT_STORAGE_DRIVER "mysql"
#define DEFAULT_SQLITE_ENABLED NO
#endif

#ifdef HAVE_MYSQL
#define DEFAULT_MYSQL_HOST "localhost"
#define DEFAULT_MYSQL_DB "gerbera"
#define DEFAULT_MYSQL_USER "gerbera"
#ifdef HAVE_SQLITE3
#define DEFAULT_MYSQL_ENABLED NO
#else
#define DEFAULT_MYSQL_ENABLED YES
#endif //HAVE_SQLITE3

#else //HAVE_MYSQL
#define DEFAULT_MYSQL_ENABLED NO
#endif

#define DEFAULT_SQLITE3_DB_FILENAME "gerbera.db"

#define CONFIG_MAPPINGS_TEMPLATE "mappings.xml"
// misc
#define INVALID_OBJECT_ID (-333)
#define INVALID_OBJECT_ID_2 (-666)
#define CHECK_SOCKET (-666)

// storage
#define LOC_DIR_PREFIX 'D'
#define LOC_FILE_PREFIX 'F'
#define LOC_VIRT_PREFIX 'V'
#define LOC_ILLEGAL_PREFIX 'X'

#ifndef DEFAULT_JS_RUNTIME_MEM
#define DEFAULT_JS_RUNTIME_MEM (1L * 1024L * 1024L)
#endif

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
#define XML_XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"
#define XML_XMLNS "http://mediatomb.cc/config/"

#ifdef HAVE_CURL
#define DEFAULT_CURL_BUFFER_SIZE 262144
#define DEFAULT_CURL_INITIAL_FILL_SIZE 0
#endif

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
#define DEFAULT_MARK_PLAYED_ITEMS_STRING "* "
#define DEFAULT_MARK_PLAYED_CONTENT_AUDIO "audio"
#define DEFAULT_MARK_PLAYED_CONTENT_VIDEO "video"
#define DEFAULT_MARK_PLAYED_CONTENT_IMAGE "image"

#define LINK_FILE_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_MEDIA_HANDLER "/"
#define LINK_WEB_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_UI_HANDLER "/"
#define LINK_SERVE_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_SERVE_HANDLER "/"
#define LINK_URL_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_ONLINE_HANDLER "/"
#endif // __COMMON_H__

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
#include "util/logger.h"

constexpr auto CONFIG_XML_VERSION_0_11_0 = 1;
constexpr auto CONFIG_XML_VERSION = 2;

// ERROR CODES
/// \brief UPnP specific error code.
constexpr auto UPNP_E_ACTION_FAILED = 501;
constexpr auto UPNP_E_SUBSCRIPTION_FAILED = 503;
/// \brief UPnP specific error code.
constexpr auto UPNP_E_NO_SUCH_ID = 701;
constexpr auto UPNP_E_NOT_EXIST = 706;

// UPnP default classes
constexpr auto UPNP_DEFAULT_CLASS_CONTAINER = "object.container";
constexpr auto UPNP_DEFAULT_CLASS_ITEM = "object.item";
constexpr auto UPNP_DEFAULT_CLASS_VIDEO_ITEM = "object.item.videoItem";
constexpr auto UPNP_DEFAULT_CLASS_IMAGE_ITEM = "object.item.imageItem";
constexpr auto UPNP_DEFAULT_CLASS_ACTIVE_ITEM = "object.item.activeItem";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_ALBUM = "object.container.album.musicAlbum";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_TRACK = "object.item.audioItem.musicTrack";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_GENRE = "object.container.genre.musicGenre";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_ARTIST = "object.container.person.musicArtist";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_COMPOSER = "object.container.person.musicComposer";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_CONDUCTOR = "object.container.person.musicConductor";
constexpr auto UPNP_DEFAULT_CLASS_MUSIC_ORCHESTRA = "object.container.person.musicOrchestra";
constexpr auto UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER = "object.container.playlistContainer";
constexpr auto UPNP_DEFAULT_CLASS_VIDEO_BROADCAST = "object.item.videoItem.videoBroadcast";

constexpr auto D_HTTP_TRANSFER_MODE_HEADER = "transferMode.dlna.org";
constexpr auto D_HTTP_TRANSFER_MODE_STREAMING = "Streaming";
constexpr auto D_HTTP_TRANSFER_MODE_INTERACTIVE = "Interactive";
constexpr auto D_HTTP_CONTENT_FEATURES_HEADER = "contentFeatures.dlna.org";

constexpr auto D_PROFILE = "DLNA.ORG_PN";
constexpr auto D_CONVERSION_INDICATOR = "DLNA.ORG_CI";
constexpr auto D_NO_CONVERSION = "0";
constexpr auto D_CONVERSION = "1";
constexpr auto D_OP = "DLNA.ORG_OP";
// all seek modes
constexpr auto D_OP_SEEK_RANGE = "01";
constexpr auto D_OP_SEEK_TIME = "10";
constexpr auto D_OP_SEEK_BOTH = "11";
constexpr auto D_OP_SEEK_DISABLED = "00";
// since we are using only range seek
constexpr auto D_OP_SEEK_ENABLED = D_OP_SEEK_RANGE;
constexpr auto D_FLAGS = "DLNA.ORG_FLAGS";
constexpr auto D_TR_FLAGS_AV = "01200000000000000000000000000000";
constexpr auto D_TR_FLAGS_IMAGE = "00800000000000000000000000000000";
constexpr auto D_MP3 = "MP3";
constexpr auto D_LPCM = "LPCM";
constexpr auto D_JPEG_SM = "JPEG_SM";
constexpr auto D_JPEG_MED = "JPEG_MED";
constexpr auto D_JPEG_LRG = "JPEG_LRG";
constexpr auto D_JPEG_TN = "JPEG_TN";
constexpr auto D_JPEG_SM_ICO = "JPEG_SM_ICO";
constexpr auto D_JPEG_LRG_ICO = "JPEG_LRG_ICO";
constexpr auto D_PN_MPEG_PS_PAL = "MPEG_PS_PAL";
constexpr auto D_PN_MKV = "MKV";
constexpr auto D_PN_AVC_MP4_EU = "AVC_MP4_EU";
constexpr auto D_PN_AVI = "AVI";
// fixed CdsObjectIDs
constexpr auto CDS_ID_BLACKHOLE = -1;
constexpr auto CDS_ID_ROOT = 0;
constexpr auto CDS_ID_FS_ROOT = 1;
constexpr auto IS_FORBIDDEN_CDS_ID(int id) { return id <= CDS_ID_FS_ROOT; };

// internal setting keys
constexpr auto SET_LAST_MODIFIED = "last_modified";

constexpr auto YES = "yes";
constexpr auto NO = "no";

// URL FORMATTING CONSTANTS
constexpr auto URL_OBJECT_ID = "object_id";
constexpr auto URL_REQUEST_TYPE = "req_type";
constexpr auto URL_RESOURCE_ID = "res_id";
constexpr auto URL_FILE_EXTENSION = "ext";
constexpr auto URL_PARAM_SEPARATOR = '/';
constexpr auto _URL_PARAM_SEPARATOR = "/";
constexpr auto URL_UI_PARAM_SEPARATOR = '?';
constexpr auto URL_ARG_SEPARATOR = '&';
constexpr auto _URL_ARG_SEPARATOR = "&";
#define SERVER_VIRTUAL_DIR "content"
#define CONTENT_MEDIA_HANDLER "media"
#define CONTENT_SERVE_HANDLER "serve"
#define CONTENT_ONLINE_HANDLER "online"
#define CONTENT_UI_HANDLER "interface"
constexpr auto DEVICE_DESCRIPTION_PATH = "description.xml";

// REQUEST TYPES
constexpr auto REQ_TYPE_BROWSE = "browse";
constexpr auto REQ_TYPE_LOGIN = "login";

// SEPARATOR
constexpr auto FS_ROOT_DIRECTORY = "/";
constexpr auto DIR_SEPARATOR = '/';
constexpr auto VIRTUAL_CONTAINER_SEPARATOR = '/';
constexpr auto VIRTUAL_CONTAINER_ESCAPE = '\\';

// MIME TYPES FOR WEB UI
constexpr auto MIMETYPE_XML = "text/xml";
constexpr auto MIMETYPE_HTML = "text/html";
constexpr auto MIMETYPE_TEXT = "text/plain";
constexpr auto MIMETYPE_JSON = "application/json"; // RFC 4627
// default mime types for items in the cds
constexpr auto MIMETYPE_DEFAULT = "application/octet-stream";

// regexp for mimetype matching
constexpr auto MIMETYPE_REGEXP = "^([a-z0-9_-]+/[a-z0-9_-]+)";

// default protocol
constexpr auto PROTOCOL = "http-get";

// device description defaults
constexpr auto DESC_DEVICE_NAMESPACE = "urn:schemas-upnp-org:device-1-0";
constexpr auto DESC_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
constexpr auto DESC_SPEC_VERSION_MAJOR = "1";
constexpr auto DESC_SPEC_VERSION_MINOR = "0";
constexpr auto DESC_FRIENDLY_NAME = PACKAGE_NAME;
constexpr auto DESC_MANUFACTURER = "Gerbera Contributors";
constexpr auto DESC_MANUFACTURER_URL = "http://gerbera.io/";
constexpr auto DESC_MODEL_DESCRIPTION = "Free UPnP AV MediaServer, GNU GPL";
constexpr auto DESC_MODEL_NAME = PACKAGE_NAME;
constexpr auto DESC_MODEL_NUMBER = VERSION;
constexpr auto DESC_SERIAL_NUMBER = "1";

//services
// connection manager
constexpr auto DESC_CM_SERVICE_TYPE = "urn:schemas-upnp-org:service:ConnectionManager:1";
constexpr auto DESC_CM_SERVICE_ID = "urn:upnp-org:serviceId:ConnectionManager";
constexpr auto DESC_CM_SCPD_URL = "cm.xml";
constexpr auto DESC_CM_CONTROL_URL = "/upnp/control/cm";
constexpr auto DESC_CM_EVENT_URL = "/upnp/event/cm";

// content directory
constexpr auto DESC_CDS_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";
constexpr auto DESC_CDS_SERVICE_ID = "urn:upnp-org:serviceId:ContentDirectory";
constexpr auto DESC_CDS_SCPD_URL = "cds.xml";
constexpr auto DESC_CDS_CONTROL_URL = "/upnp/control/cds";
constexpr auto DESC_CDS_EVENT_URL = "/upnp/event/cds";

constexpr auto DESC_ICON_PNG_MIMETYPE = "image/png";
constexpr auto DESC_ICON_BMP_MIMETYPE = "image/x-ms-bmp";
constexpr auto DESC_ICON_JPG_MIMETYPE = "image/jpeg";
constexpr auto DESC_ICON32_PNG = "/icons/mt-icon32.png";
constexpr auto DESC_ICON32_BMP = "/icons/mt-icon32.bmp";
constexpr auto DESC_ICON32_JPG = "/icons/mt-icon32.jpg";
constexpr auto DESC_ICON48_PNG = "/icons/mt-icon48.png";
constexpr auto DESC_ICON48_BMP = "/icons/mt-icon48.bmp";
constexpr auto DESC_ICON48_JPG = "/icons/mt-icon48.jpg";
constexpr auto DESC_ICON120_JPG = "/icons/mt-icon120.jpg";
constexpr auto DESC_ICON120_PNG = "/icons/mt-icon120.png";
constexpr auto DESC_ICON120_BMP = "/icons/mt-icon120.bmp";

// media receiver registrar (xbox 360)
constexpr auto DESC_MRREG_SERVICE_TYPE = "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1";
constexpr auto DESC_MRREG_SERVICE_ID = "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar";
constexpr auto DESC_MRREG_SCPD_URL = "mr_reg.xml";
constexpr auto DESC_MRREG_CONTROL_URL = "/upnp/control/mr_reg";
constexpr auto DESC_MRREG_EVENT_URL = "/upnp/event/mr_reg";

// xml namespaces
constexpr auto XML_NAMESPACE_ATTR = "xmlns";
constexpr auto XML_DIDL_LITE_NAMESPACE = "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/";
constexpr auto XML_DC_NAMESPACE_ATTR = "xmlns:dc";
constexpr auto XML_DC_NAMESPACE = "http://purl.org/dc/elements/1.1/";
constexpr auto XML_UPNP_NAMESPACE_ATTR = "xmlns:upnp";
constexpr auto XML_UPNP_NAMESPACE = "urn:schemas-upnp-org:metadata-1-0/upnp/";
constexpr auto XML_SEC_NAMESPACE_ATTR = "xmlns:sec";
constexpr auto XML_SEC_NAMESPACE = "http://www.sec.co.kr/";

// default values
constexpr auto DEFAULT_INTERNAL_CHARSET = "UTF-8";
constexpr auto DEFAULT_FILESYSTEM_CHARSET = "UTF-8";
constexpr auto DEFAULT_FALLBACK_CHARSET = "US-ASCII";
constexpr auto DEFAULT_JS_CHARSET = "UTF-8";

constexpr auto DEFAULT_CONFIG_HOME = ".config/gerbera";
constexpr auto DEFAULT_TMPDIR = "/tmp/";
constexpr auto DEFAULT_CONFIG_NAME = "config.xml";
constexpr auto DEFAULT_UI_EN_VALUE = YES;
constexpr auto DEFAULT_UI_SHOW_TOOLTIPS_VALUE = YES;
constexpr auto DEFAULT_POLL_WHEN_IDLE_VALUE = NO;
constexpr auto DEFAULT_POLL_INTERVAL = 2;
constexpr auto DEFAULT_ACCOUNTS_EN_VALUE = NO;
constexpr auto DEFAULT_ACCOUNT_USER = "gerbera";
constexpr auto DEFAULT_ACCOUNT_PASSWORD = "gerbera";
constexpr auto DEFAULT_ALIVE_INTERVAL = 1800; // seconds
constexpr auto ALIVE_INTERVAL_MIN = 62; // seconds
constexpr auto DEFAULT_BOOKMARK_FILE = "gerbera.html";
constexpr auto DEFAULT_IGNORE_UNKNOWN_EXTENSIONS = NO;
constexpr auto DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS = NO;
constexpr auto DEFAULT_IMPORT_SCRIPT = "import.js";
constexpr auto DEFAULT_PLAYLISTS_SCRIPT = "playlists.js";
constexpr auto DEFAULT_PLAYLIST_CREATE_LINK = YES;
constexpr auto DEFAULT_COMMON_SCRIPT = "common.js";
constexpr auto DEFAULT_WEB_DIR = "web";
constexpr auto DEFAULT_JS_DIR = "js";
constexpr auto DEFAULT_HIDDEN_FILES_VALUE = NO;
constexpr auto DEFAULT_UPNP_STRING_LIMIT = -1;
constexpr auto DEFAULT_SESSION_TIMEOUT = 30;
constexpr auto SESSION_TIMEOUT_CHECK_INTERVAL = (5 * 60);
constexpr auto DEFAULT_PRES_URL_APPENDTO_ATTR = "none";
constexpr auto DEFAULT_ITEMS_PER_PAGE_1 = 10;
constexpr auto DEFAULT_ITEMS_PER_PAGE_2 = 25;
constexpr auto DEFAULT_ITEMS_PER_PAGE_3 = 50;
constexpr auto DEFAULT_ITEMS_PER_PAGE_4 = 100;
constexpr auto DEFAULT_LAYOUT_TYPE = "builtin";
constexpr auto DEFAULT_EXTEND_PROTOCOLINFO = NO;
constexpr auto DEFAULT_EXTEND_PROTOCOLINFO_SM_HACK = NO;
constexpr auto DEFAULT_EXTEND_PROTOCOLINFO_DLNA_SEEK = YES;
constexpr auto DEFAULT_HIDE_PC_DIRECTORY = NO;
#ifdef SOPCAST
constexpr auto DEFAULT_SOPCAST_ENABLED = NO;
constexpr auto DEFAULT_SOPCAST_UPDATE_AT_START = NO;
#endif
#ifdef ATRAILERS
constexpr auto DEFAULT_ATRAILERS_ENABLED = NO;
constexpr auto DEFAULT_ATRAILERS_UPDATE_AT_START = NO;
constexpr auto DEFAULT_ATRAILERS_REFRESH = 43200;
constexpr auto DEFAULT_ATRAILERS_RESOLUTION = 640;
#endif
#ifdef ONLINE_SERVICES
constexpr auto CFG_DEFAULT_UPDATE_AT_START = 10; // seconds
#endif
constexpr auto DEFAULT_TRANSCODING_ENABLED = NO;
constexpr auto DEFAULT_AUDIO_BUFFER_SIZE = 1048576;
constexpr auto DEFAULT_AUDIO_CHUNK_SIZE = 131072;
constexpr auto DEFAULT_AUDIO_FILL_SIZE = 262144;

constexpr auto DEFAULT_VIDEO_BUFFER_SIZE = 14400000;
constexpr auto DEFAULT_VIDEO_CHUNK_SIZE = 512000;
constexpr auto DEFAULT_VIDEO_FILL_SIZE = 120000;

constexpr auto URL_PARAM_TRANSCODE_PROFILE_NAME = "pr_name";
constexpr auto URL_PARAM_TRANSCODE = "tr";
constexpr auto URL_PARAM_TRANSCODE_TARGET_MIMETYPE = "tmt";
constexpr auto URL_VALUE_TRANSCODE_NO_RES_ID = "none";

constexpr auto URL_VALUE_TRANSCODE = "1";
constexpr auto DEFAULT_STORAGE_CACHING_ENABLED = YES;
#ifdef HAVE_SQLITE3
constexpr auto MT_SQLITE_SYNC_FULL = 2;
constexpr auto MT_SQLITE_SYNC_NORMAL = 1;
constexpr auto MT_SQLITE_SYNC_OFF = 0;
constexpr auto DEFAULT_SQLITE_SYNC = "off";
constexpr auto DEFAULT_SQLITE_RESTORE = "restore";
constexpr auto DEFAULT_SQLITE_BACKUP_ENABLED = NO;
constexpr auto DEFAULT_SQLITE_BACKUP_INTERVAL = 600;
constexpr auto DEFAULT_SQLITE_ENABLED = YES;
constexpr auto DEFAULT_STORAGE_DRIVER = "sqlite3";
#else
constexpr auto DEFAULT_STORAGE_DRIVER = "mysql";
constexpr auto DEFAULT_SQLITE_ENABLED = NO;
#endif

#ifdef HAVE_MYSQL
constexpr auto DEFAULT_MYSQL_HOST = "localhost";
constexpr auto DEFAULT_MYSQL_DB = "gerbera";
constexpr auto DEFAULT_MYSQL_USER = "gerbera";
#ifdef HAVE_SQLITE3
constexpr auto DEFAULT_MYSQL_ENABLED = NO;
#else
constexpr auto DEFAULT_MYSQL_ENABLED = YES;
#endif //HAVE_SQLITE3

#else //HAVE_MYSQL
constexpr auto DEFAULT_MYSQL_ENABLED = NO;
#endif

constexpr auto DEFAULT_SQLITE3_DB_FILENAME = "gerbera.db";

constexpr auto CONFIG_MAPPINGS_TEMPLATE = "mappings.xml";
// misc
constexpr auto INVALID_OBJECT_ID = -333;
constexpr auto INVALID_OBJECT_ID_2 = -666;
constexpr auto CHECK_SOCKET = -666;

// storage
constexpr auto LOC_DIR_PREFIX = 'D';
constexpr auto LOC_FILE_PREFIX = 'F';
constexpr auto LOC_VIRT_PREFIX = 'V';
constexpr auto LOC_ILLEGAL_PREFIX = 'X';

#ifndef DEFAULT_JS_RUNTIME_MEM
constexpr auto DEFAULT_JS_RUNTIME_MEM = 1L * 1024L * 1024L;
#endif

constexpr auto XML_XMLNS_XSI = "http://www.w3.org/2001/XMLSchema-instance";
constexpr auto XML_XMLNS = "http://mediatomb.cc/config/";

#ifdef HAVE_CURL
constexpr auto DEFAULT_CURL_BUFFER_SIZE = 262144;
constexpr auto DEFAULT_CURL_INITIAL_FILL_SIZE = 0;
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_ENABLED = NO;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE = 128;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE = 5;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY = YES;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS = NO;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY = 8;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED = YES;
constexpr auto DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR = "";
#endif

#if defined(HAVE_LASTFMLIB)
constexpr auto DEFAULT_LASTFM_ENABLED = NO;
constexpr auto DEFAULT_LASTFM_USERNAME = "lastfmuser";
constexpr auto DEFAULT_LASTFM_PASSWORD = "lastfmpass";
#endif

constexpr auto DEFAULT_MARK_PLAYED_ITEMS_ENABLED = NO;
constexpr auto DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES = YES;
constexpr auto DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE = "prepend";
constexpr auto DEFAULT_MARK_PLAYED_ITEMS_STRING = "*";
constexpr auto DEFAULT_MARK_PLAYED_CONTENT_AUDIO = "audio";
constexpr auto DEFAULT_MARK_PLAYED_CONTENT_VIDEO = "video";
constexpr auto DEFAULT_MARK_PLAYED_CONTENT_IMAGE = "image";

constexpr auto LINK_FILE_REQUEST_HANDLER = "/" SERVER_VIRTUAL_DIR "/" CONTENT_MEDIA_HANDLER "/";
constexpr auto LINK_WEB_REQUEST_HANDLER = "/" SERVER_VIRTUAL_DIR "/" CONTENT_UI_HANDLER "/";
constexpr auto LINK_SERVE_REQUEST_HANDLER = "/" SERVER_VIRTUAL_DIR "/" CONTENT_SERVE_HANDLER "/";
constexpr auto LINK_URL_REQUEST_HANDLER = "/" SERVER_VIRTUAL_DIR "/" CONTENT_ONLINE_HANDLER "/";
#endif // __COMMON_H__

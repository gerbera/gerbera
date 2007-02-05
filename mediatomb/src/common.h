/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    common.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
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

// fixed CdsObjectIDs
#define CDS_ID_BLACKHOLE                -1
#define CDS_ID_ROOT                     0
#define CDS_ID_FS_ROOT                  1
#define IS_FORBIDDEN_CDS_ID(id)         (id <= CDS_ID_FS_ROOT)

// internal setting keys
#define SET_LAST_MODIFIED               "last_modified"

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
#define DESC_MANUFACTURER               "(c) 2004-2006 Gena Batyan <bgeradz@mediatomb.org>, Sergey Bostandzhyan <jin@mediatomb.org>, Leonhard Wimmer <leo@mediatomb.org>"
#define DESC_MANUFACTURER_URL           "http://mediatomb.org/"
#define DESC_MODEL_DESCRIPTION          "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME                 PACKAGE_NAME
#define DESC_MODEL_NUMBER               VERSION 
#define DESC_SERIAL_NUMBER              "1"

//services
// connection manager
#define DESC_CM_SERVICE_TYPE            "urn:schemas-upnp-org:service:ConnectionManager:1"
#define DESC_CM_SERVICE_ID              "urn:schemas-upnp-org:service:ConnectionManager"
#define DESC_CM_SCPD_URL                "cm.xml"
#define DESC_CM_CONTROL_URL             "/upnp/control/cm"
#define DESC_CM_EVENT_URL               "/upnp/event/cm"

// content directory
#define DESC_CDS_SERVICE_TYPE           "urn:schemas-upnp-org:service:ContentDirectory:1"
#define DESC_CDS_SERVICE_ID             "urn:schemas-upnp-org:service:ContentDirectory"
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
#if defined(__CYGWIN__)
    #define DEFAULT_CONFIG_HOME         "MediaTomb"
#else
    #define DEFAULT_CONFIG_HOME         ".mediatomb"
#endif
#define DEFAULT_SYSTEM_HOME             "mediatomb"
#define DEFAULT_ETC                     "etc"
#define DEFAULT_CONFIG_NAME             "config.xml"
#define DEFAULT_UI_EN_VALUE             "yes"
#define DEFAULT_POLL_WHEN_IDLE_VALUE    "no"
#define DEFAULT_POLL_INTERVAL           2 
#define DEFAULT_ACCOUNTS_EN_VALUE       "no"
#define DEFAULT_ALIVE_INTERVAL          180 // seconds
#define DEFAULT_BOOKMARK_FILE           "mediatomb.html"
#define DEFAULT_IGNORE_UNKNOWN_EXTENSIONS "no"
#define DEFAULT_IMPORT_SCRIPT           "import.js"
#define DEFAULT_MYSQL_HOST              "localhost"
#define DEFAULT_MYSQL_DB                "mediatomb"
#define DEFAULT_MYSQL_USER              "mediatomb"
#define DEFAULT_WEB_DIR                 "web"
#define DEFAULT_JS_DIR                  "js"
#define DEFAULT_HIDDEN_FILES_VALUE      "no"
#define DEFAULT_UPNP_STRING_LIMIT       (-1)
#define DEFAULT_SESSION_TIMEOUT         30
#define SESSION_TIMEOUT_CHECK_INTERVAL  (5 * 60)
#define DEFAULT_PRES_URL_APPENDTO_ATTR  "none"
#define DEFAULT_ITEMS_PER_PAGE_1         10
#define DEFAULT_ITEMS_PER_PAGE_2         25
#define DEFAULT_ITEMS_PER_PAGE_3         50
#define DEFAULT_ITEMS_PER_PAGE_4         100

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


#endif // __COMMON_H__


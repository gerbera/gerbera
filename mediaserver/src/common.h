/*  common.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <errno.h>
#include "upnp/upnp.h"
#include "zmmf/zmmf.h"
#include "mxml/mxml.h"
#include "exceptions.h"
#include "tools.h"

#define SERVER_VERSION                  "0.8.0"
/// \TODO implement session timeouts!!!
#define DEFAULT_SESSION_TIMEOUT         123456

// ERROR CODES                                                                                 
/// \brief UPnP specific error code.
#define UPNP_E_ACTION_FAILED            501
#define UPNP_E_SUBSCRIPTION_FAILED      503
/// \brief UPnP specific error code.
#define UPNP_E_NO_SUCH_ID               701
#define UPNP_E_NOT_EXIST                706


// URL FORMATTING CONSTANTS
#define URL_OBJECT_ID                   "object_id"
#define URL_REQUEST_TYPE                "req_type"
#define URL_REQUEST_SEPARATOR           '?'

#define CONTENT_MEDIA_HANDLER           "media"
#define CONTENT_UI_HANDLER              "interface"
// REQUEST TYPES
#define REQ_TYPE_BROWSE                 "browse"
#define REQ_TYPE_LOGIN                  "login"

// SEPARATOR
#define DIR_SEPARATOR                   '/'

// MIME TYPES FOR WEB UI
#define MIME_TYPE_XML                   "text/xml"
#define MIME_TYPE_HTML                  "text/html"

// device description defaults
#define DESC_DEVICE_NAMESPACE           "urn:schemas-upnp-org:device-1-0"
#define DESC_DEVICE_TYPE                "urn:schemas-upnp-org:device:MediaServer:1"
#define DESC_SPEC_VERSION_MAJOR         "1"
#define DESC_SPEC_VERSION_MINOR         "0"
#define DESC_FRIENDLY_NAME              "MediaTomb"
#define DESC_MANUFACTURER               "(c) 2004, 2005 Gena Batyan <bgeradz@deadlock.dhs.org>, Sergey Bostandzhyan <jin@deadlock.dhs.org>"
#define DESC_MANUFACTURER_URL           "http://www.deadlock.dhs.org/upnp"
#define DESC_MODEL_DESCRIPTION          "Free UPnP AV MediaServer, GNU GPL"
#define DESC_MODEL_NAME                 "MediaTomb"
#define DESC_MODEL_NUMBER               SERVER_VERSION 
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

#define DEFAULT_INTERNAL_CHARSET        "UTF-8"
#define DEFAULT_FILESYSTEM_CHARSET      "ISO-8859-1"
#define DEFAULT_CONFIG_HOME             ".mediatomb"
#define DEFAULT_SYSTEM_HOME             "mediatomb"
#define DEFAULT_ETC                     "etc"
#define DEFAULT_CONFIG_NAME             "config.xml"
#define DEFAULT_UI_VALUE                "no"
#define DEFAULT_ALIVE_INTERVAL          180
#define DEFAULT_BOOKMARK_FILE           "mediatomb.html"
#define DEFAULT_IGNORE_UNKNOWN_EXTENSIONS "no"
#define DEFAULT_IMPORT_SCRIPT           "import.js"
#endif // __COMMON_H__


/*MT*

    MediaTomb - http://www.mediatomb.cc/

    common.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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

#include "exceptions.h"
#include "util/logger.h"

// fixed CdsObjectIDs
#define CDS_ID_ROOT (0)
#define CDS_ID_FS_ROOT (1)
#define INVALID_OBJECT_ID (-333)
#define INVALID_OBJECT_ID_2 (-666)
static constexpr bool IS_FORBIDDEN_CDS_ID(int id) { return id <= CDS_ID_FS_ROOT; }

// URL FORMATTING CONSTANTS
#define URL_OBJECT_ID "object_id"
#define URL_REQUEST_TYPE "req_type"
#define URL_RESOURCE_ID "res_id"
constexpr auto URL_PARAM_SEPARATOR = std::string_view("/");
#define URL_UI_PARAM_SEPARATOR '?'
#define SERVER_VIRTUAL_DIR "content"

// SEPARATOR
#define FS_ROOT_DIRECTORY "/"
constexpr auto DIR_SEPARATOR = char('/');
#define VIRTUAL_CONTAINER_SEPARATOR '/'
#define VIRTUAL_CONTAINER_ESCAPE '\\'

// default mime types for items in the cds
#define MIMETYPE_DEFAULT "application/octet-stream"

// default protocol
constexpr auto PROTOCOL = std::string_view("http-get");

// device description defaults
#define DESC_MANUFACTURER_URL "http://gerbera.io/"

#define DEFAULT_INTERNAL_CHARSET "UTF-8"
#define DEFAULT_CONFIG_HOME ".config/gerbera"

static constexpr auto CLIENT_GROUP_TAG = "group";
#define DEFAULT_CLIENT_GROUP "default"

#define URL_PARAM_TRANSCODE_PROFILE_NAME "pr_name"
#define URL_PARAM_TRANSCODE "tr"
#define URL_VALUE_TRANSCODE_NO_RES_ID "none"
#define URL_VALUE_TRANSCODE "1"

// misc
#define LAYOUT_TYPE_JS "js"
#define LAYOUT_TYPE_BUILTIN "builtin"
#define LAYOUT_TYPE_DISABLED "disabled"

#define DEFAULT_MARK_PLAYED_CONTENT_AUDIO "audio"
#define DEFAULT_MARK_PLAYED_CONTENT_VIDEO "video"
#define DEFAULT_MARK_PLAYED_CONTENT_IMAGE "image"

#define LINK_FILE_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_MEDIA_HANDLER "/"
#define LINK_URL_REQUEST_HANDLER "/" SERVER_VIRTUAL_DIR "/" CONTENT_ONLINE_HANDLER "/"

#endif // __COMMON_H__

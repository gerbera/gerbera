/*MT*

    MediaTomb - http://www.mediatomb.cc/

    common.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file common.h
#ifndef __COMMON_H__
#define __COMMON_H__

#define SOURCE -1
#define OFF 0

#define GRB_STRINGIZE_IMPL(x) #x
#define GRB_STRINGIZE(x) GRB_STRINGIZE_IMPL(x)

// fixed CdsObjectIDs
#define CDS_ID_ROOT (0)
#define CDS_ID_FS_ROOT (1)
#define INVALID_OBJECT_ID (-333)
static constexpr bool IS_FORBIDDEN_CDS_ID(int id) { return id <= CDS_ID_FS_ROOT; }

// SEPARATOR
#define FS_ROOT_DIRECTORY "/"
constexpr auto DIR_SEPARATOR = char('/');
#define VIRTUAL_CONTAINER_SEPARATOR '/'
#define VIRTUAL_CONTAINER_ESCAPE '\\'

// default mime types for items in the cds
#define MIMETYPE_DEFAULT "application/octet-stream"

#define DEFAULT_INTERNAL_CHARSET "UTF-8"

static constexpr auto CLIENT_GROUP_TAG = "group";
#define DEFAULT_CLIENT_GROUP "default"
#define UNUSED_CLIENT_GROUP ""

#endif // __COMMON_H__

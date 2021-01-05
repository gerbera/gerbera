/*GRB*

    Gerbera - https://gerbera.io/

    script_names.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file script_names.h

#ifndef __SCRIPTING_SCRIPT_NAMES_H__
#define __SCRIPTING_SCRIPT_NAMES_H__

#include <array>

#include "metadata/metadata_handler.h"

constexpr std::array<std::pair<resource_attributes_t, const char*>, 10> res_names = { {
    { R_SIZE, "R_SIZE" },
    { R_DURATION, "R_DURATION" },
    { R_BITRATE, "R_BITRATE" },
    { R_SAMPLEFREQUENCY, "R_SAMPLEFREQUENCY" },
    { R_NRAUDIOCHANNELS, "R_NRAUDIOCHANNELS" },
    { R_RESOLUTION, "R_RESOLUTION" },
    { R_COLORDEPTH, "R_COLORDEPTH" },
    { R_PROTOCOLINFO, "R_PROTOCOLINFO" },
    { R_RESOURCE_FILE, "R_RESOURCE_FILE" },
    { R_TYPE, "R_TYPE" },
} };

constexpr std::array<std::pair<metadata_fields_t, const char*>, 21> mt_names = { {
    { M_TITLE, "M_TITLE" },
    { M_ARTIST, "M_ARTIST" },
    { M_ALBUM, "M_ALBUM" },
    { M_DATE, "M_DATE" },
    { M_UPNP_DATE, "M_UPNP_DATE" },
    { M_GENRE, "M_GENRE" },
    { M_DESCRIPTION, "M_DESCRIPTION" },
    { M_LONGDESCRIPTION, "M_LONGDESCRIPTION" },
    { M_TRACKNUMBER, "M_TRACKNUMBER" },
    { M_ALBUMARTURI, "M_ALBUMARTURI" },
    { M_REGION, "M_REGION" },
    { M_AUTHOR, "M_AUTHOR" },
    { M_DIRECTOR, "M_DIRECTOR" },
    { M_PUBLISHER, "M_PUBLISHER" },
    { M_RATING, "M_RATING" },
    { M_ACTOR, "M_ACTOR" },
    { M_PRODUCER, "M_PRODUCER" },
    { M_ALBUMARTIST, "M_ALBUMARTIST" },
    { M_COMPOSER, "M_COMPOSER" },
    { M_CONDUCTOR, "M_CONDUCTOR" },
    { M_ORCHESTRA, "M_ORCHESTRA" },
} };

constexpr std::array<std::pair<int, const char*>, 5> ot_names = { {
    { OBJECT_TYPE_CONTAINER, "OBJECT_TYPE_CONTAINER" },
    { OBJECT_TYPE_ITEM, "OBJECT_TYPE_ITEM" },
    { OBJECT_TYPE_ITEM_EXTERNAL_URL, "OBJECT_TYPE_ITEM_EXTERNAL_URL" },
} };

constexpr std::array<std::pair<const char*, const char*>, 12> upnp_classes = { {
    { UPNP_CLASS_MUSIC_ALBUM, "UPNP_CLASS_CONTAINER_MUSIC_ALBUM" },
    { UPNP_CLASS_MUSIC_ARTIST, "UPNP_CLASS_CONTAINER_MUSIC_ARTIST" },
    { UPNP_CLASS_MUSIC_GENRE, "UPNP_CLASS_CONTAINER_MUSIC_GENRE" },
    { UPNP_CLASS_MUSIC_COMPOSER, "UPNP_CLASS_CONTAINER_MUSIC_COMPOSER" },
    { UPNP_CLASS_MUSIC_CONDUCTOR, "UPNP_CLASS_CONTAINER_MUSIC_CONDUCTOR" },
    { UPNP_CLASS_MUSIC_ORCHESTRA, "UPNP_CLASS_CONTAINER_MUSIC_ORCHESTRA" },
    { UPNP_CLASS_CONTAINER, "UPNP_CLASS_CONTAINER" },
    { UPNP_CLASS_ITEM, "UPNP_CLASS_ITEM" },
    { UPNP_CLASS_MUSIC_TRACK, "UPNP_CLASS_ITEM_MUSIC_TRACK" },
    { UPNP_CLASS_VIDEO_ITEM, "UPNP_CLASS_CONTAINER_ITEM_VIDEO" },
    { UPNP_CLASS_IMAGE_ITEM, "UPNP_CLASS_CONTAINER_ITEM_IMAGE" },
    { UPNP_CLASS_PLAYLIST_CONTAINER, "UPNP_CLASS_PLAYLIST_CONTAINER" },
} };

#endif

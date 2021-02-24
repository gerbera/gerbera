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

using res = std::pair<resource_attributes_t, std::string_view>;
constexpr auto res_names = std::array {
    res { R_SIZE, "R_SIZE" },
    res { R_DURATION, "R_DURATION" },
    res { R_BITRATE, "R_BITRATE" },
    res { R_SAMPLEFREQUENCY, "R_SAMPLEFREQUENCY" },
    res { R_NRAUDIOCHANNELS, "R_NRAUDIOCHANNELS" },
    res { R_RESOLUTION, "R_RESOLUTION" },
    res { R_COLORDEPTH, "R_COLORDEPTH" },
    res { R_PROTOCOLINFO, "R_PROTOCOLINFO" },
    res { R_RESOURCE_FILE, "R_RESOURCE_FILE" },
    res { R_BITS_PER_SAMPLE, "R_BITS_PER_SAMPLE" },
    res { R_TYPE, "R_TYPE" },
};

using mt = std::pair<metadata_fields_t, std::string_view>;
constexpr auto mt_names = std::array {
    mt { M_TITLE, "M_TITLE" },
    mt { M_ARTIST, "M_ARTIST" },
    mt { M_ALBUM, "M_ALBUM" },
    mt { M_DATE, "M_DATE" },
    mt { M_UPNP_DATE, "M_UPNP_DATE" },
    mt { M_GENRE, "M_GENRE" },
    mt { M_DESCRIPTION, "M_DESCRIPTION" },
    mt { M_LONGDESCRIPTION, "M_LONGDESCRIPTION" },
    mt { M_TRACKNUMBER, "M_TRACKNUMBER" },
    mt { M_PARTNUMBER, "M_PARTNUMBER" },
    mt { M_ALBUMARTURI, "M_ALBUMARTURI" },
    mt { M_REGION, "M_REGION" },
    mt { M_AUTHOR, "M_AUTHOR" },
    mt { M_DIRECTOR, "M_DIRECTOR" },
    mt { M_PUBLISHER, "M_PUBLISHER" },
    mt { M_RATING, "M_RATING" },
    mt { M_ACTOR, "M_ACTOR" },
    mt { M_PRODUCER, "M_PRODUCER" },
    mt { M_ALBUMARTIST, "M_ALBUMARTIST" },
    mt { M_COMPOSER, "M_COMPOSER" },
    mt { M_CONDUCTOR, "M_CONDUCTOR" },
    mt { M_ORCHESTRA, "M_ORCHESTRA" },
};

using ot = std::pair<int, std::string_view>;
constexpr auto ot_names = std::array {
    ot { OBJECT_TYPE_CONTAINER, "OBJECT_TYPE_CONTAINER" },
    ot { OBJECT_TYPE_ITEM, "OBJECT_TYPE_ITEM" },
    ot { OBJECT_TYPE_ITEM_EXTERNAL_URL, "OBJECT_TYPE_ITEM_EXTERNAL_URL" },
};

using upnp_class = std::pair<std::string_view, std::string_view>;
constexpr auto upnp_classes = std::array {
    upnp_class { UPNP_CLASS_MUSIC_ALBUM, "UPNP_CLASS_CONTAINER_MUSIC_ALBUM" },
    upnp_class { UPNP_CLASS_MUSIC_ARTIST, "UPNP_CLASS_CONTAINER_MUSIC_ARTIST" },
    upnp_class { UPNP_CLASS_MUSIC_GENRE, "UPNP_CLASS_CONTAINER_MUSIC_GENRE" },
    upnp_class { UPNP_CLASS_MUSIC_COMPOSER, "UPNP_CLASS_CONTAINER_MUSIC_COMPOSER" },
    upnp_class { UPNP_CLASS_MUSIC_CONDUCTOR, "UPNP_CLASS_CONTAINER_MUSIC_CONDUCTOR" },
    upnp_class { UPNP_CLASS_MUSIC_ORCHESTRA, "UPNP_CLASS_CONTAINER_MUSIC_ORCHESTRA" },
    upnp_class { UPNP_CLASS_CONTAINER, "UPNP_CLASS_CONTAINER" },
    upnp_class { UPNP_CLASS_ITEM, "UPNP_CLASS_ITEM" },
    upnp_class { UPNP_CLASS_MUSIC_TRACK, "UPNP_CLASS_ITEM_MUSIC_TRACK" },
    upnp_class { UPNP_CLASS_VIDEO_ITEM, "UPNP_CLASS_CONTAINER_ITEM_VIDEO" },
    upnp_class { UPNP_CLASS_IMAGE_ITEM, "UPNP_CLASS_CONTAINER_ITEM_IMAGE" },
    upnp_class { UPNP_CLASS_PLAYLIST_CONTAINER, "UPNP_CLASS_PLAYLIST_CONTAINER" },
};

#endif

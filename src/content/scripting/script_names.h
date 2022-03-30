/*GRB*

    Gerbera - https://gerbera.io/

    script_names.h - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

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

#include "cds_objects.h"
#include "metadata/metadata_handler.h"

const static auto res_names = std::map<CdsResource::Attribute, std::string_view> {
    std::pair(CdsResource::Attribute::SIZE, "R_SIZE"),
    std::pair(CdsResource::Attribute::DURATION, "R_DURATION"),
    std::pair(CdsResource::Attribute::BITRATE, "R_BITRATE"),
    std::pair(CdsResource::Attribute::SAMPLEFREQUENCY, "R_SAMPLEFREQUENCY"),
    std::pair(CdsResource::Attribute::NRAUDIOCHANNELS, "R_NRAUDIOCHANNELS"),
    std::pair(CdsResource::Attribute::RESOLUTION, "R_RESOLUTION"),
    std::pair(CdsResource::Attribute::COLORDEPTH, "R_COLORDEPTH"),
    std::pair(CdsResource::Attribute::PROTOCOLINFO, "R_PROTOCOLINFO"),
    std::pair(CdsResource::Attribute::RESOURCE_FILE, "R_RESOURCE_FILE"),
    std::pair(CdsResource::Attribute::BITS_PER_SAMPLE, "R_BITS_PER_SAMPLE"),
    std::pair(CdsResource::Attribute::TYPE, "R_TYPE"),
    std::pair(CdsResource::Attribute::FANART_OBJ_ID, "R_FANART_OBJ_ID"),
    std::pair(CdsResource::Attribute::FANART_RES_ID, "R_FANART_RES_ID"),
    std::pair(CdsResource::Attribute::LANGUAGE, "R_LANGUAGE"),
    std::pair(CdsResource::Attribute::AUDIOCODEC, "R_AUDIOCODEC"),
    std::pair(CdsResource::Attribute::VIDEOCODEC, "R_VIDEOCODEC"),
};

const static auto mt_names = std::map<metadata_fields_t, std::string_view> {
    std::pair(M_TITLE, "M_TITLE"),
    std::pair(M_ARTIST, "M_ARTIST"),
    std::pair(M_ALBUM, "M_ALBUM"),
    std::pair(M_DATE, "M_DATE"),
    std::pair(M_CREATION_DATE, "M_CREATION_DATE"),
    std::pair(M_UPNP_DATE, "M_UPNP_DATE"),
    std::pair(M_GENRE, "M_GENRE"),
    std::pair(M_DESCRIPTION, "M_DESCRIPTION"),
    std::pair(M_LONGDESCRIPTION, "M_LONGDESCRIPTION"),
    std::pair(M_TRACKNUMBER, "M_TRACKNUMBER"),
    std::pair(M_PARTNUMBER, "M_PARTNUMBER"),
    std::pair(M_ALBUMARTURI, "M_ALBUMARTURI"),
    std::pair(M_REGION, "M_REGION"),
    std::pair(M_CREATOR, "M_CREATOR"),
    std::pair(M_AUTHOR, "M_AUTHOR"),
    std::pair(M_DIRECTOR, "M_DIRECTOR"),
    std::pair(M_PUBLISHER, "M_PUBLISHER"),
    std::pair(M_RATING, "M_RATING"),
    std::pair(M_ACTOR, "M_ACTOR"),
    std::pair(M_PRODUCER, "M_PRODUCER"),
    std::pair(M_ALBUMARTIST, "M_ALBUMARTIST"),
    std::pair(M_COMPOSER, "M_COMPOSER"),
    std::pair(M_CONDUCTOR, "M_CONDUCTOR"),
    std::pair(M_ORCHESTRA, "M_ORCHESTRA"),
    std::pair(M_CONTENT_CLASS, "M_CONTENT_CLASS"),
};

const static auto ot_names = std::map<CdsObject::Type, std::string_view> {
    { CdsObject::Type::CONTAINER, "OBJECT_TYPE_CONTAINER" },
    { CdsObject::Type::ITEM, "OBJECT_TYPE_ITEM" },
    { CdsObject::Type::EXTERNAL_URL, "OBJECT_TYPE_ITEM_EXTERNAL_URL" },
};

const static auto upnp_classes = std::map<const std::string_view, const std::string_view> {
    { UPNP_CLASS_MUSIC_ALBUM, "UPNP_CLASS_CONTAINER_MUSIC_ALBUM" },
    { UPNP_CLASS_MUSIC_ARTIST, "UPNP_CLASS_CONTAINER_MUSIC_ARTIST" },
    { UPNP_CLASS_MUSIC_GENRE, "UPNP_CLASS_CONTAINER_MUSIC_GENRE" },
    { UPNP_CLASS_MUSIC_COMPOSER, "UPNP_CLASS_CONTAINER_MUSIC_COMPOSER" },
    { UPNP_CLASS_MUSIC_CONDUCTOR, "UPNP_CLASS_CONTAINER_MUSIC_CONDUCTOR" },
    { UPNP_CLASS_MUSIC_ORCHESTRA, "UPNP_CLASS_CONTAINER_MUSIC_ORCHESTRA" },
    { UPNP_CLASS_CONTAINER, "UPNP_CLASS_CONTAINER" },
    { UPNP_CLASS_ITEM, "UPNP_CLASS_ITEM" },
    { UPNP_CLASS_AUDIO_ITEM, "UPNP_CLASS_AUDIO_ITEM" },
    { UPNP_CLASS_VIDEO_ITEM, "UPNP_CLASS_VIDEO_ITEM" },
    { UPNP_CLASS_IMAGE_ITEM, "UPNP_CLASS_IMAGE_ITEM" },
    { UPNP_CLASS_PLAYLIST_ITEM, "UPNP_CLASS_PLAYLIST_ITEM" },
    { UPNP_CLASS_MUSIC_TRACK, "UPNP_CLASS_ITEM_MUSIC_TRACK" },
    { UPNP_CLASS_VIDEO_ITEM, "UPNP_CLASS_CONTAINER_ITEM_VIDEO" },
    { UPNP_CLASS_IMAGE_ITEM, "UPNP_CLASS_CONTAINER_ITEM_IMAGE" },
    { UPNP_CLASS_PLAYLIST_CONTAINER, "UPNP_CLASS_PLAYLIST_CONTAINER" },
};

#endif

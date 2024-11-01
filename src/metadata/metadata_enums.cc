/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_types.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file metadata_enums.cc

#include "metadata_enums.h" // API

#include "content/scripting/script_names.h"
#include "util/tools.h"

std::map<MetadataFields, std::string> MetaEnumMapper::mt_keys = std::map<MetadataFields, std::string> {
    std::pair(MetadataFields::M_TITLE, DC_TITLE),
    std::pair(MetadataFields::M_ARTIST, UPNP_SEARCH_ARTIST),
    std::pair(MetadataFields::M_ALBUM, "upnp:album"),
    std::pair(MetadataFields::M_DATE, DC_DATE),
    std::pair(MetadataFields::M_CREATION_DATE, "dc:created"),
    std::pair(MetadataFields::M_UPNP_DATE, "upnp:date"),
    std::pair(MetadataFields::M_GENRE, UPNP_SEARCH_GENRE),
    std::pair(MetadataFields::M_DESCRIPTION, "dc:description"),
    std::pair(MetadataFields::M_LONGDESCRIPTION, "upnp:longDescription"),
    std::pair(MetadataFields::M_PARTNUMBER, "upnp:episodeSeason"),
    std::pair(MetadataFields::M_TRACKNUMBER, "upnp:originalTrackNumber"),
    std::pair(MetadataFields::M_ALBUMARTURI, "upnp:albumArtURI"),
    std::pair(MetadataFields::M_REGION, "upnp:region"),
    std::pair(MetadataFields::M_CREATOR, "dc:creator"),
    std::pair(MetadataFields::M_AUTHOR, "upnp:author"),
    std::pair(MetadataFields::M_DIRECTOR, "upnp:director"),
    std::pair(MetadataFields::M_PUBLISHER, "dc:publisher"),
    std::pair(MetadataFields::M_RATING, "upnp:rating"),
    std::pair(MetadataFields::M_ACTOR, "upnp:actor"),
    std::pair(MetadataFields::M_PRODUCER, "upnp:producer"),
    std::pair(MetadataFields::M_ALBUMARTIST, UPNP_SEARCH_ARTIST "@role[AlbumArtist]"),
    std::pair(MetadataFields::M_COMPOSER, "upnp:composer"),
    std::pair(MetadataFields::M_CONDUCTOR, "upnp:conductor"),
    std::pair(MetadataFields::M_ORCHESTRA, "upnp:orchestra"),
    std::pair(MetadataFields::M_CONTENT_CLASS, "upnp:contentClass"),
    std::pair(MetadataFields::M_UPNP_SHORTCUT, "upnp:shortcut"),
};

MetadataFields MetaEnumMapper::remapMetaDataField(const std::string& fieldName)
{
    for (auto&& [f, s] : mt_names) {
        if (s == fieldName) {
            return f;
        }
    }
    for (auto&& [f, s] : mt_keys) {
        if (s == fieldName) {
            return f;
        }
    }
    return MetadataFields::M_MAX;
}

std::string MetaEnumMapper::getMetaFieldName(MetadataFields field)
{
    return getValueOrDefault(mt_keys, field, { "unknown" });
}

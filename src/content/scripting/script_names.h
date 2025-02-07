/*GRB*

    Gerbera - https://gerbera.io/

    script_names.h - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

#include "cds/cds_enums.h"
#include "config/result/box_layout.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"

const static auto res_names = std::map<ResourceAttribute, std::string> {
    std::pair(ResourceAttribute::SIZE, "R_SIZE"),
    std::pair(ResourceAttribute::DURATION, "R_DURATION"),
    std::pair(ResourceAttribute::BITRATE, "R_BITRATE"),
    std::pair(ResourceAttribute::SAMPLEFREQUENCY, "R_SAMPLEFREQUENCY"),
    std::pair(ResourceAttribute::NRAUDIOCHANNELS, "R_NRAUDIOCHANNELS"),
    std::pair(ResourceAttribute::RESOLUTION, "R_RESOLUTION"),
    std::pair(ResourceAttribute::COLORDEPTH, "R_COLORDEPTH"),
    std::pair(ResourceAttribute::PROTOCOLINFO, "R_PROTOCOLINFO"),
    std::pair(ResourceAttribute::RESOURCE_FILE, "R_RESOURCE_FILE"),
    std::pair(ResourceAttribute::BITS_PER_SAMPLE, "R_BITS_PER_SAMPLE"),
    std::pair(ResourceAttribute::TYPE, "R_TYPE"),
    std::pair(ResourceAttribute::FORMAT, "R_FORMAT"),
    std::pair(ResourceAttribute::FANART_OBJ_ID, "R_FANART_OBJ_ID"),
    std::pair(ResourceAttribute::FANART_RES_ID, "R_FANART_RES_ID"),
    std::pair(ResourceAttribute::LANGUAGE, "R_LANGUAGE"),
    std::pair(ResourceAttribute::AUDIOCODEC, "R_AUDIOCODEC"),
    std::pair(ResourceAttribute::VIDEOCODEC, "R_VIDEOCODEC"),
    std::pair(ResourceAttribute::ORIENTATION, "R_ORIENTATION"),
    std::pair(ResourceAttribute::PIXELFORMAT, "R_PIXELFORMAT"),
};

const static auto mt_names = std::map<MetadataFields, std::string_view> {
    std::pair(MetadataFields::M_TITLE, "M_TITLE"),
    std::pair(MetadataFields::M_ARTIST, "M_ARTIST"),
    std::pair(MetadataFields::M_ALBUM, "M_ALBUM"),
    std::pair(MetadataFields::M_DATE, "M_DATE"),
    std::pair(MetadataFields::M_CREATION_DATE, "M_CREATION_DATE"),
    std::pair(MetadataFields::M_UPNP_DATE, "M_UPNP_DATE"),
    std::pair(MetadataFields::M_GENRE, "M_GENRE"),
    std::pair(MetadataFields::M_DESCRIPTION, "M_DESCRIPTION"),
    std::pair(MetadataFields::M_LONGDESCRIPTION, "M_LONGDESCRIPTION"),
    std::pair(MetadataFields::M_TRACKNUMBER, "M_TRACKNUMBER"),
    std::pair(MetadataFields::M_PARTNUMBER, "M_PARTNUMBER"),
    std::pair(MetadataFields::M_ALBUMARTURI, "M_ALBUMARTURI"),
    std::pair(MetadataFields::M_REGION, "M_REGION"),
    std::pair(MetadataFields::M_CREATOR, "M_CREATOR"),
    std::pair(MetadataFields::M_AUTHOR, "M_AUTHOR"),
    std::pair(MetadataFields::M_DIRECTOR, "M_DIRECTOR"),
    std::pair(MetadataFields::M_PUBLISHER, "M_PUBLISHER"),
    std::pair(MetadataFields::M_RATING, "M_RATING"),
    std::pair(MetadataFields::M_ACTOR, "M_ACTOR"),
    std::pair(MetadataFields::M_PRODUCER, "M_PRODUCER"),
    std::pair(MetadataFields::M_ALBUMARTIST, "M_ALBUMARTIST"),
    std::pair(MetadataFields::M_COMPOSER, "M_COMPOSER"),
    std::pair(MetadataFields::M_CONDUCTOR, "M_CONDUCTOR"),
    std::pair(MetadataFields::M_ORCHESTRA, "M_ORCHESTRA"),
    std::pair(MetadataFields::M_CONTENT_CLASS, "M_CONTENT_CLASS"),
    std::pair(MetadataFields::M_UPNP_SHORTCUT, "M_UPNP_SHORTCUT"),
};

const static auto ot_names = std::map<int, std::string_view> {
    { OBJECT_TYPE_CONTAINER, "OBJECT_TYPE_CONTAINER" },
    { OBJECT_TYPE_ITEM, "OBJECT_TYPE_ITEM" },
    { OBJECT_TYPE_ITEM_EXTERNAL_URL, "OBJECT_TYPE_ITEM_EXTERNAL_URL" },
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
    { UPNP_CLASS_PLAYLIST_ITEM, "UPNP_CLASS_PLAYLIST_ITEM" },

    { UPNP_CLASS_AUDIO_ITEM, "UPNP_CLASS_AUDIO_ITEM" },
    { UPNP_CLASS_MUSIC_TRACK, "UPNP_CLASS_ITEM_MUSIC_TRACK" },
    { UPNP_CLASS_AUDIO_BOOK, "UPNP_CLASS_AUDIO_BOOK" },
    { UPNP_CLASS_AUDIO_BROADCAST, "UPNP_CLASS_AUDIO_BROADCAST" },

    { UPNP_CLASS_IMAGE_ITEM, "UPNP_CLASS_IMAGE_ITEM" },
    { UPNP_CLASS_IMAGE_PHOTO, "UPNP_CLASS_IMAGE_PHOTO" },

    { UPNP_CLASS_VIDEO_ITEM, "UPNP_CLASS_VIDEO_ITEM" },
    { UPNP_CLASS_VIDEO_MOVIE, "UPNP_CLASS_VIDEO_MOVIE" },
    { UPNP_CLASS_VIDEO_MUSICVIDEOCLIP, "UPNP_CLASS_VIDEO_MUSICVIDEOCLIP" },
    { UPNP_CLASS_VIDEO_BROADCAST, "UPNP_CLASS_VIDEO_BROADCAST" },

    { UPNP_CLASS_PHOTO_ALBUM, "UPNP_CLASS_CONTAINER_ITEM_IMAGE" },
    { UPNP_CLASS_PLAYLIST_CONTAINER, "UPNP_CLASS_PLAYLIST_CONTAINER" },

    { UPNP_CLASS_DYNAMIC_CONTAINER, "UPNP_CLASS_DYNAMIC_CONTAINER" },
};

const static auto boxKeyNames = std::map<const std::string_view, const std::string_view> {
    { "BK_audioAllAlbums", BoxKeys::audioAllAlbums },
    { "BK_audioAllArtists", BoxKeys::audioAllArtists },
    { "BK_audioAll", BoxKeys::audioAll },
    { "BK_audioAllComposers", BoxKeys::audioAllComposers },
    { "BK_audioAllDirectories", BoxKeys::audioAllDirectories },
    { "BK_audioAllGenres", BoxKeys::audioAllGenres },
    { "BK_audioAllSongs", BoxKeys::audioAllSongs },
    { "BK_audioAllTracks", BoxKeys::audioAllTracks },
    { "BK_audioAllYears", BoxKeys::audioAllYears },
    { "BK_audioArtistChronology", BoxKeys::audioArtistChronology },
    { "BK_audioRoot", BoxKeys::audioRoot },

    { "BK_audioInitialAbc", BoxKeys::audioInitialAbc },
    { "BK_audioInitialAllArtistTracks", BoxKeys::audioInitialAllArtistTracks },
    { "BK_audioInitialAllBooks", BoxKeys::audioInitialAllBooks },
    { "BK_audioInitialAudioBookRoot", BoxKeys::audioInitialAudioBookRoot },

    { "BK_audioStructuredAllAlbums", BoxKeys::audioStructuredAllAlbums },
    { "BK_audioStructuredAllArtistTracks", BoxKeys::audioStructuredAllArtistTracks },
    { "BK_audioStructuredAllArtists", BoxKeys::audioStructuredAllArtists },
    { "BK_audioStructuredAllGenres", BoxKeys::audioStructuredAllGenres },
    { "BK_audioStructuredAllTracks", BoxKeys::audioStructuredAllTracks },
    { "BK_audioStructuredAllYears", BoxKeys::audioStructuredAllYears },

    { "BK_videoAllDates", BoxKeys::videoAllDates },
    { "BK_videoAllDirectories", BoxKeys::videoAllDirectories },
    { "BK_videoAll", BoxKeys::videoAll },
    { "BK_videoAllYears", BoxKeys::videoAllYears },
    { "BK_videoRoot", BoxKeys::videoRoot },
    { "BK_videoUnknown", BoxKeys::videoUnknown },

    { "BK_imageAllDates", BoxKeys::imageAllDates },
    { "BK_imageAllDirectories", BoxKeys::imageAllDirectories },
    { "BK_imageAll", BoxKeys::imageAll },
    { "BK_imageAllYears", BoxKeys::imageAllYears },
    { "BK_imageRoot", BoxKeys::imageRoot },
    { "BK_imageUnknown", BoxKeys::imageUnknown },

#ifdef ONLINE_SERVICES
    { "BK_trailerAllGenres", BoxKeys::trailerAllGenres },
    { "BK_trailerAll", BoxKeys::trailerAll },
    { "BK_trailerPostDate", BoxKeys::trailerPostDate },
    { "BK_trailerRelDate", BoxKeys::trailerRelDate },
    { "BK_trailerRoot", BoxKeys::trailerRoot },
    { "BK_trailerUnknown", BoxKeys::trailerUnknown },
#endif

    { "BK_playlistAll", BoxKeys::playlistAll },
    { "BK_playlistAllDirectories", BoxKeys::playlistAllDirectories },
    { "BK_playlistRoot", BoxKeys::playlistRoot },
};

#endif

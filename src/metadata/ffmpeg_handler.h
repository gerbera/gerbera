/*MT*

    MediaTomb - http://www.mediatomb.cc/

    ffmpeg_handler.h - this file is part of MediaTomb.

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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// \file ffmpeg_handler.h
/// \brief Definition of the FfmpegHandler class - getting metadata via
/// ffmpeg library calls.

#ifndef __FFMPEG_HANDLER_H__
#define __FFMPEG_HANDLER_H__
#ifdef HAVE_FFMPEG

#include "metadata_enums.h"
#include "metadata_handler.h"

#include <array>

// forward declarations
class CdsItem;
class FfmpegObject;
class IOHandler;
struct AVFormatContext;
class StringConverter;

/// \brief This class is responsible for reading id3 tags metadata
class FfmpegHandler : public MediaMetadataHandler {
public:
    explicit FfmpegHandler(const std::shared_ptr<Context>& context);

    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;
    std::string getMimeType() const override;

private:
    /// \brief get all AUX values as configured
    void addFfmpegAuxdataFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;
    /// \brief get all metadata fields
    void addFfmpegMetadataFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;
    /// \brief get additional resource fields
    static void addFfmpegResourceFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject);
    /// \brief fabricate comment from metadata
    void addFfmpegComment(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;

    static constexpr std::array propertyMap {
        std::pair(MetadataFields::M_TITLE, "title"),
        std::pair(MetadataFields::M_ARTIST, "artist"),
        std::pair(MetadataFields::M_ALBUM, "album"),
        std::pair(MetadataFields::M_GENRE, "genre"),
        std::pair(MetadataFields::M_DESCRIPTION, "description"),
        std::pair(MetadataFields::M_TRACKNUMBER, "track"),
        std::pair(MetadataFields::M_PARTNUMBER, "discnumber"),
        std::pair(MetadataFields::M_ALBUMARTIST, "album_artist"),
        std::pair(MetadataFields::M_COMPOSER, "composer"),
        std::pair(MetadataFields::M_DATE, "date"),
        std::pair(MetadataFields::M_CREATION_DATE, "creation_time"),
    };
};

#endif // HAVE_FFMPEG
#endif //__FFMPEG_HANDLER_H__

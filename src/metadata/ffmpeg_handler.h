/*MT*

    MediaTomb - http://www.mediatomb.cc/

    ffmpeg_handler.h - this file is part of MediaTomb.

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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// \file ffmpeg_handler.h
/// \brief Definition of the FfmpegHandler class - getting metadata via
/// ffmpeg library calls.

#ifdef HAVE_FFMPEG
#ifndef __FFMPEG_HANDLER_H__
#define __FFMPEG_HANDLER_H__

#include <array>
#include <optional>

#include "metadata_handler.h"

// forward declaration
class IOHandler;
struct AVFormatContext;

/// \brief This class is responsible for reading id3 tags metadata
class FfmpegHandler : public MetadataHandler {
public:
    explicit FfmpegHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) override;
    std::string getMimeType() const override;

private:
    std::map<std::string, std::string> specialPropertyMap;

    void addFfmpegAuxdataFields(const std::shared_ptr<CdsItem>& item, const AVFormatContext* pFormatCtx) const;
    void addFfmpegMetadataFields(const std::shared_ptr<CdsItem>& item, const AVFormatContext* pFormatCtx) const;
    static void addFfmpegResourceFields(const std::shared_ptr<CdsItem>& item, const AVFormatContext* pFormatCtx);

    static constexpr auto propertyMap = std::array {
        std::pair(M_TITLE, "title"),
        std::pair(M_ARTIST, "artist"),
        std::pair(M_ALBUM, "album"),
        std::pair(M_GENRE, "genre"),
        std::pair(M_DESCRIPTION, "description"),
        std::pair(M_TRACKNUMBER, "track"),
        std::pair(M_PARTNUMBER, "discnumber"),
        std::pair(M_ALBUMARTIST, "album_artist"),
        std::pair(M_COMPOSER, "composer"),
        std::pair(M_DATE, "date"),
        std::pair(M_CREATION_DATE, "creation_time"),
    };
};

#endif //__FFMPEG_HANDLER_H__
#endif // HAVE_FFMPEG

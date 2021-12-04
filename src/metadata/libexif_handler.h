/*MT*

    MediaTomb - http://www.mediatomb.cc/

    libexif_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

/// \file libexif_handler.h
/// \brief Definition of the LibExifHandler class.

#pragma once

#ifdef HAVE_LIBEXIF

#include <libexif/exif-content.h>
#include <libexif/exif-data.h>
#include <vector>

#include "metadata_handler.h"

// forward declaration
class CdsItem;
class StringConverter;

/// \brief This class is responsible for reading exif header metadata via the
/// libexif library
class LibExifHandler : public MetadataHandler {
    using MetadataHandler::MetadataHandler;

private:
    // image resolution in pixels
    // the problem is that I do not know when I encounter the
    // tags for X and Y, so I have to save the information
    // and in the very end - when I have both values - add the
    // appropriate resource
    std::string imageX;
    std::string imageY;

    void process_ifd(const ExifContent* content, const std::shared_ptr<CdsItem>& item, const std::unique_ptr<StringConverter>& sc,
        const std::vector<std::string>& auxtags, const std::map<std::string, std::string>& metatags);

public:
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, int resNum) override;
};
#endif

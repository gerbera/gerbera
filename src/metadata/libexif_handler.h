/*MT*

    MediaTomb - http://www.mediatomb.cc/

    libexif_handler.h - this file is part of MediaTomb.

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

/// \file libexif_handler.h
/// \brief Definition of the LibExifHandler class.
#ifndef __METADATA_LIBEXIF_H__
#define __METADATA_LIBEXIF_H__

#ifdef HAVE_LIBEXIF
#include "metadata_handler.h"

#include <libexif/exif-content.h>
#include <libexif/exif-data.h>
#include <vector>

// forward declarations
class CdsItem;
class LibExifObject;
class StringConverter;

/// \brief This class is responsible for reading exif header metadata via the
/// libexif library
class LibExifHandler : public MediaMetadataHandler {
private:
    ExifLog* log = nullptr;

    /// \brief read exif content values
    void process_ifd(
        const ExifContent* content,
        const std::shared_ptr<CdsItem>& item,
        LibExifObject& exifObject,
        std::vector<std::string>& snippets,
        std::string& imageX,
        std::string& imageY);

public:
    explicit LibExifHandler(const std::shared_ptr<Context>& context);
    ~LibExifHandler();

    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;
};

#endif
#endif // __METADATA_LIBEXIF_H__

/*GRB*

    Gerbera - https://gerbera.io/
    
    matroska_handler.h - this file is part of Gerbera.
    
    Copyright (C) 2019-2021 Gerbera Contributors
    
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

/// \file matroska_handler.h
/// \brief Definition of the MatroskaHandler class.

#ifndef __METADATA_MATROSKA_H__
#define __METADATA_MATROSKA_H__

#ifdef HAVE_MATROSKA

#include <ebml/EbmlStream.h>
#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>

#include "metadata_handler.h"

// forward declaration
class MemIOHandler;

/// \brief This class is responsible for reading webm or mkv tags metadata
class MatroskaHandler : public MetadataHandler {
public:
    explicit MatroskaHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime);
    void fillMetadata(std::shared_ptr<CdsItem> item) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum) override;

private:
    void parseMKV(const std::shared_ptr<CdsItem>& item, MemIOHandler** p_io_handler) const;
    void parseLevel1Element(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebml_stream, LIBEBML_NAMESPACE::EbmlElement* el_l1, MemIOHandler** p_io_handler) const;
    void parseInfo(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, LIBEBML_NAMESPACE::EbmlMaster* info) const;
    void parseAttachments(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebml_stream, LIBEBML_NAMESPACE::EbmlMaster* attachments, MemIOHandler** io_handler) const;
    std::string getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData* data) const;
    static void addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& art_mimetype);
};

#endif

#endif // __METADATA_MATROSKA_H__

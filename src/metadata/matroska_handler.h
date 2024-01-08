/*GRB*

    Gerbera - https://gerbera.io/

    matroska_handler.h - this file is part of Gerbera.

    Copyright (C) 2019-2024 Gerbera Contributors

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
#include "metadata_handler.h"

#include <ebml/EbmlStream.h>
#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>

// forward declaration
class MemIOHandler;

/// \brief This class is responsible for reading webm or mkv tags metadata
class MatroskaHandler : public MetadataHandler {
    using MetadataHandler::MetadataHandler;

public:
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) override;

private:
    int activeFlag {};

    void parseMKV(const std::shared_ptr<CdsItem>& item, std::unique_ptr<MemIOHandler>* pIoHandler);
    void parseLevel1Element(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlElement* elL1, std::unique_ptr<MemIOHandler>* pIoHandler);
    void parseHead(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* info, std::unique_ptr<MemIOHandler>* pIoHandler);
    void parseInfo(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* info);
    void parseAttachments(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* pIoHandler);
    std::string getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData& data) const;
    static void addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& artMimetype);
};

#endif

#endif // __METADATA_MATROSKA_H__

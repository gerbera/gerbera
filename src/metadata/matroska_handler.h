/*GRB*

    Gerbera - https://gerbera.io/

    matroska_handler.h - this file is part of Gerbera.

    Copyright (C) 2019-2025 Gerbera Contributors

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
#include <matroska/KaxVersion.h>
#if LIBMATROSKA_VERSION >= 0x010200
#include <matroska/KaxSemantic.h>
#else
#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#endif

#include <map>

// forward declarations
class CdsItem;
class MemIOHandler;

/// \brief This class is responsible for reading webm or mkv tags metadata
class MatroskaHandler : public MediaMetadataHandler {
public:
    explicit MatroskaHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    int activeFlag {};
    std::map<std::string, std::string> mkvTags;

    /// \brief Parse Matroska file data
    void parseMKV(
        const std::shared_ptr<CdsItem>& item,
        std::unique_ptr<MemIOHandler>* pIoHandler);
    /// \brief Parse head of Matroska metadata
    void parseHead(
        const std::shared_ptr<CdsItem>& item,
        libebml::IOCallback& ebmlFile,
        libebml::EbmlStream& ebmlStream,
        libebml::EbmlMaster* info,
        std::unique_ptr<MemIOHandler>* pIoHandler);
    /// \brief Parse info of Matroska metadata
    void parseInfo(
        const std::shared_ptr<CdsItem>& item,
        libebml::EbmlStream& ebmlStream,
        libebml::EbmlMaster* info);
    /// \brief Parse tags of Matroska metadata
    void parseTags(
        const std::shared_ptr<CdsItem>& item,
        libebml::EbmlStream& ebmlStream,
        libebml::EbmlMaster* info);
    /// \brief Parse level one of Matroska metadata (segment)
    void parseSegmentContent(
        const std::shared_ptr<CdsItem>& item,
        libebml::IOCallback& ebmlFile,
        libebml::EbmlStream& ebmlStream,
        libebml::EbmlElement* elL1,
        std::unique_ptr<MemIOHandler>* pIoHandler);
    /// \brief Parse attachment of Matroska metadata
    void parseAttachments(
        const std::shared_ptr<CdsItem>& item,
        libebml::EbmlStream& ebmlStream,
        libebml::EbmlMaster* attachments,
        std::unique_ptr<MemIOHandler>* pIoHandler);
    std::string getContentTypeFromByteVector(const libmatroska::KaxFileData& data) const;

    static void addArtworkResource(
        const std::shared_ptr<CdsItem>& item,
        const std::string& artMimetype);
};

#endif

#endif // __METADATA_MATROSKA_H__

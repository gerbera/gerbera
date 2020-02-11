/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    matroska_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2019 Patrick Ammann <pammann@gmx.net>
    
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

/// \file matroska_handler.h
/// \brief Definition of the MatroskaHandler class.

#ifndef __METADATA_MATROSKA_H__
#define __METADATA_MATROSKA_H__

#ifdef HAVE_MATROSKA

#include <ebml/EbmlStream.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxAttached.h>

#include "metadata_handler.h"
#include "iohandler/mem_io_handler.h"

/// \brief This class is responsible for reading webm or mkv tags metadata
class MatroskaHandler : public MetadataHandler {
public:
    MatroskaHandler(std::shared_ptr<ConfigManager> config);
    virtual void fillMetadata(std::shared_ptr<CdsItem> item);
    virtual std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum);

private:
    void parseMKV(const std::shared_ptr<CdsItem>& item, MemIOHandler** p_io_handler);
    void parseLevel1Element(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebml_stream, LIBEBML_NAMESPACE::EbmlElement* el_l1, MemIOHandler** p_io_handler);
    void parseInfo(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, LIBMATROSKA_NAMESPACE::KaxInfo* info);
    void parseAttachments(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebml_stream, LIBMATROSKA_NAMESPACE::KaxAttachments* attachments, MemIOHandler** io_handler);
    std::string getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData* data) const;
    void addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& content_type);
};

#endif

#endif // __METADATA_MATROSKA_H__

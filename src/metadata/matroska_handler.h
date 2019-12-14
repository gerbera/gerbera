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
#include "mem_io_handler.h"

/// \brief This class is responsible for reading webm or mkv tags metadata
class MatroskaHandler : public MetadataHandler {
public:
    MatroskaHandler();
    virtual void fillMetadata(zmm::Ref<CdsItem> item);
    virtual zmm::Ref<IOHandler> serveContent(zmm::Ref<CdsItem> item, int resNum, off_t* data_size);

private:
    void parseMKV(zmm::Ref<CdsItem> item, MemIOHandler** p_io_handler, off_t** p_data_size);
    void parseLevel1Element(zmm::Ref<CdsItem> item, LIBEBML_NAMESPACE::EbmlStream & ebml_stream, LIBEBML_NAMESPACE::EbmlElement* el_l1, MemIOHandler** p_io_handler, off_t** p_data_size);
    void parseInfo(zmm::Ref<CdsItem> item, EbmlStream & ebml_stream, LIBMATROSKA_NAMESPACE::KaxInfo *info);
    void parseAttachments(zmm::Ref<CdsItem> item, LIBEBML_NAMESPACE::EbmlStream & ebml_stream, LIBMATROSKA_NAMESPACE::KaxAttachments *attachments, MemIOHandler** io_handler, off_t** p_data_size);
    std::string getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData* data) const;
    void addArtworkResource(zmm::Ref<CdsItem> item, std::string content_type);
};

#endif

#endif // __METADATA_MATROSKA_H__

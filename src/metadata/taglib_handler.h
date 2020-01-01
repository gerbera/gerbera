/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    taglib_handler.h - this file is part of MediaTomb.
    
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

/// \file taglib_handler.h
/// \brief Definition of the TagHandler class.

#ifndef __METADATA_TAGLIB_H__
#define __METADATA_TAGLIB_H__

#ifdef HAVE_TAGLIB

#include <string>

#include <taglib/tbytevector.h>
#include <taglib/tfile.h>
#include <taglib/tiostream.h>

#include "metadata_handler.h"

/// \brief This class is responsible for reading id3 or ogg tags metadata
class TagLibHandler : public MetadataHandler {
public:
    TagLibHandler(std::shared_ptr<ConfigManager> config);
    virtual void fillMetadata(zmm::Ref<CdsItem> item);
    virtual std::unique_ptr<IOHandler> serveContent(zmm::Ref<CdsItem> item, int resNum);

private:
    void addField(metadata_fields_t field, const TagLib::File& file, const TagLib::Tag* tag, zmm::Ref<CdsItem> item) const;

    void populateGenericTags(zmm::Ref<CdsItem> item, const TagLib::File& file) const;
    bool isValidArtworkContentType(std::string content_type);
    std::string getContentTypeFromByteVector(const TagLib::ByteVector& data) const;
    void addArtworkResource(zmm::Ref<CdsItem> item, std::string content_type);
    void extractMP3(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractOgg(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractASF(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractFLAC(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractAPE(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractWavPack(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractMP4(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
    void extractAiff(TagLib::IOStream* roStream, zmm::Ref<CdsItem> item);
};

#endif

#endif // __METADATA_TAGLIB_H__

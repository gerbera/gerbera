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
    explicit TagLibHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime);
    void fillMetadata(std::shared_ptr<CdsItem> item) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum) override;

private:
    std::string entrySeparator;
    std::string legacyEntrySeparator;

    void addField(metadata_fields_t field, const TagLib::File& file, const TagLib::Tag* tag, const std::shared_ptr<CdsItem>& item) const;

    void populateGenericTags(const std::shared_ptr<CdsItem>& item, const TagLib::File& file) const;
    static bool isValidArtworkContentType(const std::string& art_mimetype);
    std::string getContentTypeFromByteVector(const TagLib::ByteVector& data) const;
    static void addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& art_mimetype);
    void extractMP3(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractOgg(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractASF(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractFLAC(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractAPE(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractWavPack(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractMP4(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractAiff(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const;
};

#endif

#endif // __METADATA_TAGLIB_H__

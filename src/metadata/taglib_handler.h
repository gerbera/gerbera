/*MT*

    MediaTomb - http://www.mediatomb.cc/

    taglib_handler.h - this file is part of MediaTomb.

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

/// \file taglib_handler.h
/// \brief Definition of the TagHandler class.

#ifndef __METADATA_TAGLIB_H__
#define __METADATA_TAGLIB_H__

#ifdef HAVE_TAGLIB

#include <tbytevector.h>
#include <tfile.h>
#include <tiostream.h>

#include "metadata_handler.h"

class StringConverter;

/// \brief This class is responsible for reading id3 or ogg tags metadata
class TagLibHandler : public MetadataHandler {
public:
    explicit TagLibHandler(const std::shared_ptr<Context>& context);

    /// \brief read metadata from file and add to object
    /// \param obj Object to handle
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief stream content of object or resource to client
    /// \param obj Object to stream
    /// \param resource the resource
    /// \return iohandler to stream to client
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) override;

private:
    std::string entrySeparator;
    std::string legacyEntrySeparator;
    std::map<std::string, std::string> specialPropertyMap;

    void addField(metadata_fields_t field, const TagLib::File& file, const TagLib::Tag* tag, const std::shared_ptr<CdsItem>& item) const;
    void addSpecialFields(const TagLib::File& file, const TagLib::Tag* tag, const std::shared_ptr<CdsItem>& item) const;

    static std::unique_ptr<TagLib::File> getOggFile(TagLib::IOStream& ioStream);

    void populateGenericTags(const std::shared_ptr<CdsItem>& item, const TagLib::File& file) const;
    void populateAuxTags(const std::shared_ptr<CdsItem>& item, const TagLib::PropertyMap& propertyMap, const std::unique_ptr<StringConverter>& sc) const;
    static bool isValidArtworkContentType(std::string_view artMimetype);
    std::string getContentTypeFromByteVector(const TagLib::ByteVector& data) const;
    static void addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& artMimetype);
    void extractMP3(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractOgg(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractASF(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractFLAC(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractAPE(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractWavPack(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractMP4(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
    void extractAiff(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const;
};

#endif

#endif // __METADATA_TAGLIB_H__

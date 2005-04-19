/*  metadata_reader.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
    Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \file metadata_reader.h 
/// \brief Definition of the MetadataReader class.
#ifndef __METADATA_READER_H__
#define __METADATA_READER_H__

/// \brief This class is responsible for providing access to metadata information
/// of various media. Currently only id3 is supported.
class MetadataReader : public zmm::Object
{
    
public:
    MetadataReader();
       
    zmm::Ref<Dictionary> getMetadata(zmm::String filename);

protected:
    void addField(zmm::String filename, zmm::String name, zmm::String value);
    zmm::Ref<Dictionary> data;
}

#endif


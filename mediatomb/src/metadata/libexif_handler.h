/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    libexif_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file libexif_handler.h
/// \brief Definition of the LibExifHandler class.
#ifndef __METADATA_LIBEXIF_H__
#define __METADATA_LIBEXIF_H__

#include "metadata_handler.h"
#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include "string_converter.h"


/// \brief This class is responsible for reading exif header metadata via the
/// libefix library
class LibExifHandler : public MetadataHandler
{
protected:
    // image resolution in pixels
    // the problem is that I do not know when I encounter the
    // tags for X and Y, so I have to save the information 
    // and in the very end - when I have both values - add the
    // appropriate resource
    zmm::String imageX;
    zmm::String imageY;

    void process_ifd (ExifContent *content, zmm::Ref<CdsItem> item, zmm::Ref<StringConverter> sc, zmm::Ref<zmm::Array<zmm::StringBase> > auxtags);
    
public:
    LibExifHandler();
    virtual void fillMetadata(zmm::Ref<CdsItem> item);
    virtual zmm::Ref<IOHandler> serveContent(zmm::Ref<CdsItem> item, int resNum, off_t *data_size);
};

#endif // __METADATA_LIBEXIF_H__


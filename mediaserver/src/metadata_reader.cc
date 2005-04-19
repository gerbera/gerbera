/*  metadata_reader.cc - this file is part of MediaTomb.

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

/// \file metadata_reader.cc
/// \brief Implementeation of the MetadataReader class.

#include "common.h"
#include <id3/tag.h>

using namespace zmm;
using namespace mxml;

    
MetadataReader()
{
}
       
Ref<Dictionary> getMetadata(String filename)
{
}

void addField(String filename, String name, String value)
{
    data(new Dictionary());

    ID3_Tag myTag(filename.c_str());
    ID3_Tag::Iterator* iter = myTag.CreateIterator();
    ID3_Frame* myFrame = NULL;
    while (NULL != (myFrame = iter->GetNext()))
    {
        ID3_Frame::Iterator* iter = myFrame->CreateIterator();
        ID3_Field* myField = NULL;
        while (NULL != (myField = iter->GetNext()))
        {
            uchar   data[1024];
            myField->Get(data, 1024);
        }
        delete iter;

    }
    delete iter;

}



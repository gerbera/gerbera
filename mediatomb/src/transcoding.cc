/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: transcoding.h $
*/

/// \file transcoding.cc
/// \brief Definitions of the Transcoding classes. 

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef TRANSCODING

#include "transcoding.h"

using namespace zmm;

TranscodingProfileList::TranscodingProfileList()
{
    list = Ref<Array<TranscodingProfile> > (new Array<TranscodingProfile>());
}

void TranscodingProfileList::add(zmm::Ref<TranscodingProfile> prof)
{
    list->append(prof);
}

Ref<TranscodingProfile> TranscodingProfileList::get(zmm::String acceptedMimeType)
{
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) != nil && acceptedMimeType == list->get(i)->getAcceptedMimeType())
                return list->get(i);
    }
    return nil;
}

Ref<TranscodingProfile> TranscodingProfileList::getByName(zmm::String name)
{
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) != nil && name == list->get(i)->getName())
                return list->get(i);
    }
    return nil;
}

Ref<TranscodingProfile> TranscodingProfileList::get(int index)
{
    if ((index < 0) || (index >= list->size()))
        return nil;

    return list->get(index);
}

#endif//TRANSCODING

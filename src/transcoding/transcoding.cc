/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcoding.cc - this file is part of MediaTomb.
    
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

/// \file transcoding.cc
/// \brief Definitions of the Transcoding classes. 

#ifdef EXTERNAL_TRANSCODING

#include "transcoding.h"
#include "tools.h"

using namespace zmm;

TranscodingProfile::TranscodingProfile()
{
    first_resource = false;
    buffer_size = 0;
    chunk_size = 0;
    initial_fill_size = 0;
    tr_type = TR_None;
    theora = false;
    accept_url = true;
    dvd_only = false;
    force_chunked = true;
    hide_orig_res = false;
    thumbnail = false;
    sample_frequency = SOURCE; // keep original
    number_of_channels = SOURCE;
    attributes = Ref<Dictionary>(new Dictionary());
    fourcc_list = Ref<Array<StringBase> >(new Array<StringBase>());
    fourcc_mode = FCC_None;
}

TranscodingProfile::TranscodingProfile(transcoding_type_t tr_type, String name) 
{
    this->name = name;
    this->tr_type = tr_type;
    theora = false;
    first_resource = false;
    accept_url = true;
    dvd_only = false;
    force_chunked = true;
    hide_orig_res = false;
    thumbnail = false;
    sample_frequency = SOURCE; // keep original
    number_of_channels = SOURCE;
    buffer_size = 0;
    chunk_size = 0;
    initial_fill_size = 0;
    tr_type = TR_None;
    attributes = Ref<Dictionary>(new Dictionary());
    fourcc_list = Ref<Array<StringBase> >(new Array<StringBase>());
    fourcc_mode = FCC_None;
}

void TranscodingProfile::setBufferOptions(size_t bs, size_t cs, size_t ifs)
{
    buffer_size = bs;
    chunk_size = cs;
    initial_fill_size = ifs;
}

void TranscodingProfile::addAttribute(zmm::String name, zmm::String value)
{
    attributes->put(name, value);
}

Ref<Dictionary> TranscodingProfile::getAttributes()
{   
    return attributes;
}   


void TranscodingProfile::setAVIFourCCList(Ref<Array<StringBase> > list,
                                          avi_fourcc_listmode_t mode)
{
    fourcc_list = list;
    fourcc_mode = mode;
}

Ref<Array<StringBase> > TranscodingProfile::getAVIFourCCList()
{
    return fourcc_list;
}


TranscodingProfileList::TranscodingProfileList()
{
    list = Ref<ObjectDictionary<ObjectDictionary<TranscodingProfile> > >(new ObjectDictionary<ObjectDictionary<TranscodingProfile> >());
}

void TranscodingProfileList::add(zmm::String sourceMimeType, zmm::Ref<TranscodingProfile> prof)
{
    Ref<ObjectDictionary<TranscodingProfile> > inner = list->get(sourceMimeType);

    if (inner == nil)
        inner = Ref<ObjectDictionary<TranscodingProfile> >(new ObjectDictionary<TranscodingProfile>());

    inner->put(prof->getName(), prof);
    list->put(sourceMimeType, inner);
}

Ref<ObjectDictionary<TranscodingProfile> > TranscodingProfileList::get(zmm::String sourceMimeType)
{
    return list->get(sourceMimeType);
}

Ref<TranscodingProfile> TranscodingProfileList::getByName(zmm::String name)
{
    Ref<Array<ObjectDictionaryElement<ObjectDictionary<TranscodingProfile> > > > mt_list = list->getElements();

    for (int i = 0; i < mt_list->size(); i++)
    {
        Ref<ObjectDictionary<TranscodingProfile> > names = mt_list->get(i)->getValue();
        if (names != nil)
        {
            Ref<TranscodingProfile> tp = names->get(name);
            if (tp != nil)
                return tp;
        }
    }
    return nil;
}

#endif//EXTERNAL_TRANSCODING

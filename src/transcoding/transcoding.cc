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

#include "transcoding.h" // API

#include "util/tools.h"

TranscodingProfile::TranscodingProfile(bool enabled, transcoding_type_t trType, std::string name)
    : enabled(enabled)
    , name(std::move(name))
    , tr_type(trType)
{
}

TranscodingFilter::TranscodingFilter(std::string mimeType, std::string transcoder)
    : mimeType(std::move(mimeType))
    , transcoder(std::move(transcoder))
{
}

void TranscodingProfile::setBufferOptions(std::size_t bs, std::size_t cs, std::size_t ifs)
{
    buffer_size = bs;
    chunk_size = cs;
    initial_fill_size = ifs;
}

void TranscodingProfile::setAttributeOverride(CdsResource::Attribute attribute, const std::string& value)
{
    attributeOverrides[attribute] = value;
}

std::string TranscodingProfile::getAttributeOverride(CdsResource::Attribute attribute) const
{
    auto it = attributeOverrides.find(attribute);
    if (it != attributeOverrides.end()) {
        return it->second;
    }
    return {};
}

std::map<CdsResource::Attribute, std::string> TranscodingProfile::getAttributeOverrides() const
{
    return attributeOverrides;
}

void TranscodingProfile::setAVIFourCCList(const std::vector<std::string>& list, avi_fourcc_listmode_t mode)
{
    fourcc_list = list;
    fourcc_mode = mode;
}

const std::vector<std::string>& TranscodingProfile::getAVIFourCCList() const
{
    return fourcc_list;
}

std::shared_ptr<TranscodingProfile> TranscodingProfileList::getByName(const std::string& name, bool getAll) const
{
    for (auto&& filter : filterList) {
        if (filter->getTranscodingProfile() && filter->getTranscodingProfile()->getName() == name && (getAll || filter->getTranscodingProfile()->isEnabled()))
            return filter->getTranscodingProfile();
    }
    return {};
}

std::string TranscodingProfile::mapFourCcMode(avi_fourcc_listmode_t mode)
{
    switch (mode) {
    case FCC_None:
        return "disabled";
    case FCC_Ignore:
        return "ignore";
    case FCC_Process:
        return "process";
    }
    return "disabled";
}

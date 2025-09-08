/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcoding.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file config/result/transcoding.cc
#define GRB_LOG_FAC GrbLogFacility::transcoding

#include "transcoding.h" // API

TranscodingProfile::TranscodingProfile(bool enabled, TranscodingType trType, std::string name)
    : enabled(enabled)
    , name(std::move(name))
    , trType(trType)
{
}

TranscodingFilter::TranscodingFilter(std::string mimeType, std::string transcoder)
    : mimeType(std::move(mimeType))
    , transcoder(std::move(transcoder))
{
}

void TranscodingProfile::setBufferOptions(std::size_t bs, std::size_t cs, std::size_t ifs)
{
    bufferSize = bs;
    chunkSize = cs;
    initialFillSize = ifs;
}

void TranscodingProfile::setAttributeOverride(ResourceAttribute attribute, const std::string& value)
{
    attributeOverrides[attribute] = value;
}

std::string TranscodingProfile::getAttributeOverride(ResourceAttribute attribute) const
{
    auto it = attributeOverrides.find(attribute);
    if (it != attributeOverrides.end()) {
        return it->second;
    }
    return {};
}

std::map<ResourceAttribute, std::string> TranscodingProfile::getAttributeOverrides() const
{
    return attributeOverrides;
}

void TranscodingProfile::setAVIFourCCList(const std::vector<std::string>& list, AviFourccListmode mode)
{
    fourccList = list;
    fourccMode = mode;
}

const std::vector<std::string>& TranscodingProfile::getAVIFourCCList() const
{
    return fourccList;
}

std::shared_ptr<TranscodingProfile> TranscodingProfileList::getByName(const std::string& name, bool getAll) const
{
    for (auto&& filter : filterList) {
        if (filter->getTranscodingProfile() && filter->getTranscodingProfile()->getName() == name && (getAll || filter->getTranscodingProfile()->isEnabled()))
            return filter->getTranscodingProfile();
    }
    return {};
}

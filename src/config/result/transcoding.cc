/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcoding.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

#include "util/grb_time.h"

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

long long TranscodingProfile::getSeekBitrate() const
{
    auto entry = environment.find("SEEK_BITRATE");
    if (entry == environment.end())
        return 0;
    try {
        auto rate = std::stoll(entry->second);
        return rate > 0 ? rate : 0;
    } catch (const std::exception&) {
        return 0;
    }
}

off_t TranscodingProfile::getSeekSize(const std::string& duration) const
{
    auto rate = getSeekBitrate();
    if (rate <= 0 || duration.empty())
        return -1;
    // One second short of the duration: a stream at constant bit rate ends a
    // little before duration * rate, and a client that was promised more bytes
    // than arrive sees the connection close before the end. One second short,
    // the stream always fills the length and ends cleanly on it.
    auto milliseconds = HMSFToMilliseconds(duration) - 1000;
    if (milliseconds <= 0)
        return -1;
    return static_cast<off_t>(milliseconds * rate / 8000);
}

void TranscodingBuffer::setOptions(std::size_t bs, std::size_t cs, std::size_t ifs)
{
    size = bs;
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

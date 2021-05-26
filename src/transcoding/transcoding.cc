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

TranscodingProfile::TranscodingProfile(transcoding_type_t tr_type, std::string name)
    : name(std::move(name))
    , tr_type(tr_type)
{
}

void TranscodingProfile::setBufferOptions(size_t bs, size_t cs, size_t ifs)
{
    buffer_size = bs;
    chunk_size = cs;
    initial_fill_size = ifs;
}

void TranscodingProfile::addAttribute(const std::string& name, std::string value)
{
    attributes[name] = std::move(value);
}

std::map<std::string, std::string> TranscodingProfile::getAttributes() const
{
    return attributes;
}

void TranscodingProfile::setAVIFourCCList(std::vector<std::string> list, avi_fourcc_listmode_t mode)
{
    fourcc_list = std::move(list);
    fourcc_mode = mode;
}

std::vector<std::string> TranscodingProfile::getAVIFourCCList() const
{
    return fourcc_list;
}

void TranscodingProfileList::add(const std::string& sourceMimeType, const std::shared_ptr<TranscodingProfile>& prof)
{
    auto it = list.find(sourceMimeType);
    auto inner = [this, it] {
        if (it != list.end())
            return it->second;
        return std::make_shared<TranscodingProfileMap>();
    }();

    inner->insert(std::pair<std::string, std::shared_ptr<TranscodingProfile>>(prof->getName(), prof));
    list[sourceMimeType] = inner;
}

std::shared_ptr<TranscodingProfileMap> TranscodingProfileList::get(const std::string& sourceMimeType)
{
    auto it = list.find(sourceMimeType);
    if (it != list.end())
        return it->second;
    return nullptr;
}

std::shared_ptr<TranscodingProfile> TranscodingProfileList::getByName(const std::string& name, bool getAll)
{
    for (auto&& [key, inner] : list) {
        auto tp = inner->find(name);
        if (tp != inner->end() && (getAll || tp->second->getEnabled()))
            return tp->second;
    }
    return nullptr;
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

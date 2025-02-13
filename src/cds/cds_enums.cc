/*GRB*
Gerbera - https://gerbera.io/

    cds_enums.cc - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file cds_enums.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "cds_enums.h" // API

#include "content/scripting/script_names.h"
#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <algorithm>
#include <array>
#include <fmt/format.h>

static constexpr std::array chKeys = {
    std::pair(ContentHandler::DEFAULT, "Default"),
#ifdef HAVE_LIBEXIF
    std::pair(ContentHandler::LIBEXIF, "LibExif"),
#endif
#ifdef HAVE_TAGLIB
    std::pair(ContentHandler::ID3, "TagLib"),
#endif
    std::pair(ContentHandler::TRANSCODE, "Transcode"),
    std::pair(ContentHandler::EXTURL, "Exturl"),
    std::pair(ContentHandler::MP4, "MP4"),
#ifdef HAVE_FFMPEGTHUMBNAILER
    std::pair(ContentHandler::FFTH, "FFmpegThumbnailer"),
#endif
    std::pair(ContentHandler::FLAC, "Flac"),
    std::pair(ContentHandler::FANART, "Fanart"),
    std::pair(ContentHandler::CONTAINERART, "ContainerArt"),
#ifdef HAVE_MATROSKA
    std::pair(ContentHandler::MATROSKA, "Matroska"),
#endif
#ifdef HAVE_WAVPACK
    std::pair(ContentHandler::WAVPACK, "WavPack"),
#endif
    std::pair(ContentHandler::SUBTITLE, "Subtitle"),
    std::pair(ContentHandler::METAFILE, "MetaFile"),
    std::pair(ContentHandler::RESOURCE, "Resource"),
};

bool EnumMapper::checkContentHandler(const std::string& contHandler)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [contHandler](auto&& entry) { return contHandler == entry.second; });
    return chEntry != chKeys.end();
}

ContentHandler EnumMapper::remapContentHandler(const std::string& contHandler)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [contHandler](auto&& entry) { return contHandler == entry.second; });
    if (chEntry != chKeys.end()) {
        return chEntry->first;
    }
    throw_std_runtime_error("Invalid content handler value {}", contHandler);
}

ContentHandler EnumMapper::remapContentHandler(int ch)
{
    return static_cast<ContentHandler>(ch);
}

std::string EnumMapper::mapContentHandler2String(ContentHandler ch)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [ch](auto&& entry) { return ch == entry.first; });
    if (chEntry != chKeys.end()) {
        return chEntry->second;
    }
    throw_std_runtime_error("Invalid content handler value {}", ch);
}

std::string EnumMapper::getPurposeDisplay(ResourcePurpose purpose)
{
    return purposeToDisplay.at(purpose);
}

ResourcePurpose EnumMapper::mapPurpose(const std::string& name)
{
    auto purpose = toLower(name);
    auto purpEntry = std::find_if(purposeToDisplay.begin(), purposeToDisplay.end(), [purpose](auto&& entry) { return purpose == toLower(entry.second); });
    if (purpEntry != purposeToDisplay.end()) {
        return purpEntry->first;
    }
    throw_std_runtime_error("Invalid purpose value {}", name);
}

std::string EnumMapper::getAttributeName(ResourceAttribute attr)
{
    return attrToName.at(attr);
}

std::string EnumMapper::getAttributeDisplay(ResourceAttribute attr)
{
    return attrToDisplay.at(attr);
}

ResourceAttribute EnumMapper::mapAttributeName(const std::string& name)
{
    for (auto&& [attr, n] : attrToName) {
        if (n == name) {
            return attr;
        }
    }
    for (auto&& [attr, n] : res_names) {
        if (n == name) {
            return attr;
        }
    }
    throw std::out_of_range { fmt::format("Could not map {}", name) };
}

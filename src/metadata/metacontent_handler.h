/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metacontent_handler.h - this file is part of MediaTomb.

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

/// \file metacontent_handler.h
/// \brief Definition of the Metacontent, FanArt and Subtitle classes.

#ifndef __METADATA_CONTENT_H__
#define __METADATA_CONTENT_H__

#include <filesystem>
namespace fs = std::filesystem;

#include "metadata_handler.h"

/// \brief This class is responsible for populating filesystem based metadata
class MetacontentHandler : public MetadataHandler {
public:
    explicit MetacontentHandler(std::shared_ptr<Config> config);

protected:
    static fs::path getContentPath(std::vector<std::string>& names, const std::shared_ptr<CdsItem>& item);
    static std::string expandName(const std::string& name, const std::shared_ptr<CdsItem>& item);
};

/// \brief This class is responsible for populating filesystem based album and fan art
class FanArtHandler : public MetacontentHandler {
public:
    explicit FanArtHandler(std::shared_ptr<Config> config);
    void fillMetadata(std::shared_ptr<CdsItem> item) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum) override;

private:
    static std::vector<std::string> names;
};

/// \brief This class is responsible for populating filesystem based subtitles
class SubtitleHandler : public MetacontentHandler {
public:
    explicit SubtitleHandler(std::shared_ptr<Config> config);
    void fillMetadata(std::shared_ptr<CdsItem> item) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum) override;

private:
    static std::vector<std::string> names;
};

#endif // __METADATA_CONTENT_H__

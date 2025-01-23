/*MT*

    MediaTomb - http://www.mediatomb.cc/

    fallback_layout.h - this file is part of MediaTomb.

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

/// \file builtin_layout.h

#ifndef __BUILTIN_LAYOUT_H__
#define __BUILTIN_LAYOUT_H__

#include <map>
#include <memory>

#include "layout.h"

// forward declaration
class CdsContainer;
class Config;
class ConverterManager;

/// \brief layout class implementation for simple virtual layout
class BuiltinLayout : public Layout {
public:
    explicit BuiltinLayout(std::shared_ptr<Content> content);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<ConverterManager> converterManager;
    std::map<std::string, std::string> genreMap;
    std::map<std::string, std::pair<int, bool>> chain;
    std::map<std::string, std::shared_ptr<CdsContainer>> container;

    int add(
        const std::shared_ptr<CdsObject>& obj,
        const std::pair<int, bool>& parentID,
        bool useRef = true);
    int getDir(
        const std::shared_ptr<CdsObject>& obj,
        const fs::path& rootPath,
        const std::string_view& c1,
        const std::string_view& c2,
        const std::string& upnpClass);
    std::vector<int> addVideo(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap) override;
    std::vector<int> addImage(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap) override;
    std::vector<int> addAudio(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap) override;

#ifdef ONLINE_SERVICES
    std::vector<int> addOnlineItem(
        const std::shared_ptr<CdsObject>& obj,
        OnlineServiceType serviceType,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap) override;
#endif

    std::string mapGenre(const std::string& genre);
    std::shared_ptr<CdsContainer> containerAt(const std::string_view& boxKey) { return container.at(boxKey.data()); }
};

#endif // __BUILTIN_LAYOUT_H__

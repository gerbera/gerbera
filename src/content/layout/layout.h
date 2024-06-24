/*MT*

    MediaTomb - http://www.mediatomb.cc/

    layout.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file layout.h

#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include "util/grb_fs.h"

#include <map>
#include <vector>

// forward declaration
enum class AutoscanMediaMode;
class CdsContainer;
class CdsObject;
class Content;
enum class OnlineServiceType;

class Layout {
public:
    explicit Layout(std::shared_ptr<Content> content)
        : content(std::move(content))
    {
    }

    virtual ~Layout() = default;

    virtual void processCdsObject(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::string& contentType,
        const std::map<AutoscanMediaMode, std::string>& containerMap,
        std::vector<int>& refObjects);

protected:
    virtual std::vector<int> addVideo(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap)
        = 0;
    virtual std::vector<int> addImage(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap)
        = 0;
    virtual std::vector<int> addAudio(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap)
        = 0;

#ifdef ONLINE_SERVICES
    virtual std::vector<int> addTrailer(const std::shared_ptr<CdsObject>& obj,
        OnlineServiceType serviceType,
        const fs::path& rootpath,
        const std::map<AutoscanMediaMode, std::string>& containerMap)
        = 0;
#endif

    std::shared_ptr<Content> content;

    void cleanUp(const std::vector<int>& refObjects, const std::vector<int>& resObjects);
};

#endif // __LAYOUT_H__

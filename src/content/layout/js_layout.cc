/*MT*

    MediaTomb - http://www.mediatomb.cc/

    js_layout.cc - this file is part of MediaTomb.

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

/// \file js_layout.cc
#define GRB_LOG_FAC GrbLogFacility::layout

#ifdef HAVE_JS
#include "js_layout.h" // API

#include "content/scripting/import_script.h"

JSLayout::JSLayout(const std::shared_ptr<Content>& content, const std::string& parent)
    : Layout(content)
    , import_script(std::make_unique<ImportScript>(content, parent))
{
}

void JSLayout::processCdsObject(const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsContainer>& parent,
    const fs::path& rootpath,
    const std::string& contentType,
    const std::map<AutoscanMediaMode, std::string>& containerMap,
    std::vector<int>& refObjects)
{
    if (!import_script)
        return;

    if (import_script->hasImportFunctions())
        Layout::processCdsObject(obj, parent, rootpath, contentType, containerMap, refObjects);
    else {
        auto resObjects = import_script->processCdsObject(obj, rootpath, containerMap);
        cleanUp(refObjects, resObjects);
    }
}

std::vector<int> JSLayout::addAudio(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    if (!import_script)
        return {};

    return import_script->addAudio(obj, parent, rootpath, containerMap);
}

std::vector<int> JSLayout::addVideo(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    if (!import_script)
        return {};

    return import_script->addVideo(obj, parent, rootpath, containerMap);
}

std::vector<int> JSLayout::addImage(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    if (!import_script)
        return {};

    return import_script->addImage(obj, parent, rootpath, containerMap);
}

#ifdef ONLINE_SERVICES
std::vector<int> JSLayout::addOnlineItem(const std::shared_ptr<CdsObject>& obj, OnlineServiceType serviceType, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    if (!import_script)
        return {};

    return import_script->addOnlineItem(obj, rootpath, containerMap);
}
#endif
#endif // HAVE_JS

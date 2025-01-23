/*MT*

    MediaTomb - http://www.mediatomb.cc/

    js_layout.h - this file is part of MediaTomb.

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

/// \file js_layout.h

#ifndef __JS_LAYOUT_H__
#define __JS_LAYOUT_H__

#include "layout.h"

#include <memory>

class ImportScript;
class ScriptingRuntime;

/// \brief layout class implementation for flexible java script implemented virtual layout
class JSLayout : public Layout {
protected:
    std::shared_ptr<ScriptingRuntime> runtime;
    std::unique_ptr<ImportScript> import_script;

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

public:
    JSLayout(const std::shared_ptr<Content>& content, const std::string& parent);

    void processCdsObject(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& parent,
        const fs::path& rootpath,
        const std::string& contentType,
        const std::map<AutoscanMediaMode, std::string>& containerMap,
        std::vector<int>& refObjects) override;
};

#endif // __JS_LAYOUT_H__

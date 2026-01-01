/*MT*

    MediaTomb - http://www.mediatomb.cc/

    import_script.h - this file is part of MediaTomb.

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

/// @file content/scripting/import_script.h

#ifndef __SCRIPTING_IMPORT_SCRIPT_H__
#define __SCRIPTING_IMPORT_SCRIPT_H__

#include "common.h"
#include "script.h"

#include <map>
#include <memory>

// forward declaration
enum class AutoscanMediaMode;
class Config;
class Content;
class ScriptingRuntime;

class ImportScript : public Script {
public:
    ImportScript(const std::shared_ptr<Content>& content, const std::string& parent);

    std::vector<int> addVideo(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& cont,
        const fs::path& scriptPath,
        const std::map<AutoscanMediaMode, std::string>& containerMap);
    std::vector<int> addImage(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& cont,
        const fs::path& scriptPath,
        const std::map<AutoscanMediaMode, std::string>& containerMap);
    std::vector<int> addAudio(const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& cont,
        const fs::path& scriptPath,
        const std::map<AutoscanMediaMode, std::string>& containerMap);
#ifdef ONLINE_SERVICES
    std::vector<int> addOnlineItem(const std::shared_ptr<CdsObject>& obj,
        const fs::path& scriptPath,
        const std::map<AutoscanMediaMode, std::string>& containerMap);
#endif

    bool setRefId(
        const std::shared_ptr<CdsObject>& cdsObj,
        const std::shared_ptr<CdsObject>& origObject,
        int pcdId) override;
    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(
        const std::shared_ptr<CdsObject>& origObject,
        const std::string& rootPath) override
    {
        return { dukObject2cdsObject(origObject), INVALID_OBJECT_ID };
    }

private:
    std::vector<int> callFunction(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsContainer>& cont,
        const std::string& function,
        const fs::path& scriptPath,
        AutoscanMediaMode mediaMode,
        const std::map<AutoscanMediaMode, std::string>& containerMap);
};

#endif // __SCRIPTING_IMPORT_SCRIPT_H__

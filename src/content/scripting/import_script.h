/*MT*

    MediaTomb - http://www.mediatomb.cc/

    import_script.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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

/// \file import_script.h

#ifndef __SCRIPTING_IMPORT_SCRIPT_H__
#define __SCRIPTING_IMPORT_SCRIPT_H__

#include <memory>

#include "common.h"
#include "script.h"

// forward declaration
enum class AutoscanMediaMode;
class ContentManager;
class ScriptingRuntime;

class ImportScript : public Script {
public:
    ImportScript(const std::shared_ptr<ContentManager>& content, const std::string& parent);

    void processCdsObject(const std::shared_ptr<CdsObject>& obj, const fs::path& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap);
    void addVideo(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& cont, const fs::path& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap);
    void addImage(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& cont, const fs::path& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap);
    void addAudio(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& cont, const fs::path& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap);
#ifdef ONLINE_SERVICES
    void addTrailer(const std::shared_ptr<CdsObject>& obj, const fs::path& scriptPath, const std::map<AutoscanMediaMode, std::string>& containerMap);
#endif

    bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) override;
    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) override
    {
        return { dukObject2cdsObject(origObject), INVALID_OBJECT_ID };
    }
    bool hasImportFunctions() const;

private:
    void callFunction(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& cont, const std::string& function, const fs::path& scriptPath, AutoscanMediaMode mediaMode, const std::map<AutoscanMediaMode, std::string>& containerMap);
};

#endif // __SCRIPTING_IMPORT_SCRIPT_H__

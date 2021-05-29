/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    playlist_parser_script.h - this file is part of MediaTomb.
    
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

/// \file playlist_parser_script.h

#ifndef __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__
#define __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__

#include <memory>

#include "common.h"
#include "script.h"

// forward declaration
class CdsObject;
class ContentManager;
class GenericTask;

class PlaylistParserScript : public Script {
public:
    PlaylistParserScript(std::shared_ptr<ContentManager> content,
        const std::shared_ptr<ScriptingRuntime>& runtime);

    std::string readln();
    void processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task);
    script_class_t whoami() override { return S_PLAYLIST; }

private:
    FILE* currentHandle;
    int currentObjectID;
    char* currentLine;
    std::shared_ptr<GenericTask> currentTask;
};

#endif // __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__

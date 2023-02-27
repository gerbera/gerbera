/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.h - this file is part of MediaTomb.

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

/// \file playlist_parser_script.h

#ifndef __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__
#define __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__

#include <memory>
#include <pugixml.hpp>

#include "common.h"
#include "script.h"

// forward declaration
class CdsObject;
class ContentManager;
class GenericTask;

extern "C" {
duk_ret_t jsReadXml(duk_context* ctx);
duk_ret_t jsReadln(duk_context* ctx);
duk_ret_t jsGetCdsObject(duk_context* ctx);
duk_ret_t jsUpdateCdsObject(duk_context* ctx);
}

class ParserScript : public Script {
public:
    std::pair<std::string, bool> readLine();
    pugi::xml_node& readXml(int direction);

protected:
    ParserScript(const std::shared_ptr<ContentManager>& content, const std::string& name, const std::string& objName);

    std::FILE* currentHandle {};
    int currentObjectID { INVALID_OBJECT_ID };
    char* currentLine {};
    std::shared_ptr<GenericTask> currentTask;
    std::unique_ptr<pugi::xml_document> xmlDoc { std::make_unique<pugi::xml_document>() };
    pugi::xml_node root;
};

class PlaylistParserScript : public ParserScript {
public:
    PlaylistParserScript(const std::shared_ptr<ContentManager>& content);
    void processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& rootPath);

    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) override;
    bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) override;

protected:
    void handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item) override;
    void handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont) override;
};

class MetafileParserScript : public ParserScript {
public:
    MetafileParserScript(const std::shared_ptr<ContentManager>& content);
    void processObject(const std::shared_ptr<CdsObject>& obj, const fs::path& path);

    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) override;
    bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) override;

protected:
    std::shared_ptr<CdsObject> createObject(const std::shared_ptr<CdsObject>& pcd) override;
    void handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item) override;
};

#endif // __SCRIPTING_PLAYLIST_PARSER_SCRIPT_H__

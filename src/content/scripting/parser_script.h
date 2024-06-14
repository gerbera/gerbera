/*MT*

    MediaTomb - http://www.mediatomb.cc/

    parser_script.h - this file is part of MediaTomb.

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

/// \file parser_script.h

#ifndef __PARSER_SCRIPT_H__
#define __PARSER_SCRIPT_H__

#include "common.h"
#include "script.h"

#include <memory>
#include <pugixml.hpp>

// forward declaration
class CdsObject;
class Content;
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
    ParserScript(const std::shared_ptr<Content>& content, const std::string& parent, const std::string& name, const std::string& objName);

    static pugi::xml_node nullNode;
    static constexpr int ONE_TEXTLINE_BYTES = 1024;

    std::FILE* currentHandle {};
    int currentObjectID { INVALID_OBJECT_ID };
    char* currentLine {};
    std::shared_ptr<GenericTask> currentTask;
    std::unique_ptr<pugi::xml_document> xmlDoc { std::make_unique<pugi::xml_document>() };
    pugi::xml_node root;
    bool scriptMode { false };
};

#endif // __PARSER_SCRIPT_H__

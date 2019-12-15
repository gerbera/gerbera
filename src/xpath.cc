/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xpath.cc - this file is part of MediaTomb.
    
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

/// \file xpath.cc

#include "xpath.h"
#include "common.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

XPath::XPath(Ref<Element> context)
    : Object()
{
    this->context = context;
}

Ref<Element> XPath::getElement(std::string xpath)
{
    std::string axisPart = getAxisPart(xpath);
    if (!axisPart.empty()) {
        throw _Exception("XPath::getElement: unexpected axis in " + xpath);
    }
    return elementAtPath(xpath);
}

std::string XPath::getText(std::string xpath)
{
    std::string axisPart = getAxisPart(xpath);
    std::string pathPart = getPathPart(xpath);

    Ref<Element> el = elementAtPath(pathPart);
    if (el == nullptr)
        return "";

    if (axisPart.empty())
        return el->getText();

    std::string axis = getAxis(axisPart);
    std::string spec = getSpec(axisPart);

    if (axis != "attribute") {
        throw _Exception("XPath::getText: unexpected axis: " + axis);
    }

    return el->getAttribute(spec);
}

std::string XPath::getPathPart(std::string xpath)
{
    size_t slashPos = xpath.rfind('/');
    if (slashPos == std::string::npos)
        return xpath;
    if (strstr(xpath.c_str() + slashPos, "::")) {
        return xpath.substr(0, slashPos);
    }
    return xpath;
}

Ref<Element> XPath::elementAtPath(std::string path)
{
    Ref<Element> cur = context;
    std::vector<std::string> parts = split_string(path, '/');

    for (size_t i = 0; i < parts.size(); i++) {
        std::string part = parts[i];
        cur = cur->getChildByName(part);
        if (cur == nullptr)
            break;
    }
    return cur;
}

std::string XPath::getAxisPart(std::string xpath)
{
    size_t slashPos = xpath.rfind('/');
    if (slashPos == std::string::npos)
        slashPos = 0;
    if (strstr(xpath.c_str() + slashPos, "::")) {
        return xpath.substr(slashPos + 1);
    }
    return "";
}

std::string XPath::getAxis(std::string axisPart)
{
    const char* pos = strstr(axisPart.c_str(), "::");
    return axisPart.substr(0, pos - axisPart.c_str());
}

std::string XPath::getSpec(std::string axisPart)
{
    const char* pos = strstr(axisPart.c_str(), "::");
    return pos + 2;
}

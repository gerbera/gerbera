/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_to_json.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    Copyright (C) 2020 Patrick Ammann <pammann@gmx.net>
    
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

/// \file xml_to_json.cc

#include "xml_to_json.h"

#include <iostream>
#include <regex>

#include "util/tools.h"

std::string Xml2Json::getJson(const pugi::xml_node& root, const Hints& hints)
{
    std::ostringstream buf;
    buf << '{';
    handleElement(buf, root, hints);
    buf << '}';
    return buf.str();
}

void Xml2Json::handleElement(std::ostringstream& buf, const pugi::xml_node& node, const Hints& hints)
{
    bool firstChild = true;

    auto attrs = node.attributes();
    size_t attributeCount = std::distance(attrs.begin(), attrs.end());
    if (attributeCount > 0) {
        for (const pugi::xml_attribute& at : attrs) {
            if (!firstChild)
                buf << ',';
            else
                firstChild = false;
            buf << getAsString(at.name()) << ':' << getValue(at.name(), at.as_string(), hints);
        }
    }

    std::string nodeName;
    bool array = isArray(node, hints, &nodeName);

    if (array) {
        if (!firstChild)
            buf << ',';
        buf << getAsString(nodeName.c_str()) << ':';
        buf << '[';
        firstChild = true;
    }

    for (const pugi::xml_node& child : node.children()) {
        pugi::xml_node_type type = child.type();

        if (type == pugi::node_element) {
            if (!firstChild)
                buf << ',';
            else
                firstChild = false;

            // look ahead
            auto childAttrs = child.attributes();
            size_t childAttributeCount = std::distance(childAttrs.begin(), childAttrs.end());
            size_t childElementCount = std::count_if(child.children().begin(), child.children().end(), [&](const auto& el) { return el.type() == pugi::node_element; });

            if (array) {
                if (nodeName != child.name())
                    throw_std_runtime_error("if an element is of arrayType, all children have to have the same name");
            } else {
                buf << getAsString(child.name()) << ':';
            }

            if (childAttributeCount > 0 || childElementCount > 0 || isArray(child, hints, nullptr)) {
                buf << '{';
                handleElement(buf, child, hints);
                buf << '}';
            } else {
                buf << getValue(child.name(), child.text().as_string(), hints);
            }
        }
    }

    if (array)
        buf << ']';
}

static const std::string& replaceAllString(std::string& str, std::string_view from, const std::string& to)
{
    size_t start_pos = str.find(from);
    while (start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos = str.find(from, start_pos + to.length());
    }
    return str;
}

std::string Xml2Json::getAsString(const char* str)
{
    auto escaped = escape(str, '\\', '"');

    escaped = replaceAllString(escaped, "\n", "\\n");
    escaped = replaceAllString(escaped, "\r", "\\r");
    return "\"" + escaped + '"';
}

std::string Xml2Json::getValue(const std::string& name, const char* text, const Hints& hints)
{
    std::string str = text;
    auto& hintsType = hints.asType;

    auto hint = hintsType.find(name);

    if (hint != hintsType.end()) {
        if (hint->second == "string") {
            return getAsString(text);
        }

        if (hint->second == "bool") {
            return text;
        }

        if (hint->second == "number") {
            return text;
        }
    }

    // boolean
    if (str == "true" || str == "false")
        return str;

    // number
    if (std::regex_match(str, std::regex("-?\\d+")))
        return str;

    return getAsString(text);
}

bool Xml2Json::isArray(const pugi::xml_node& node, const Hints& hints, std::string* arrayName)
{
    auto& hintsArray = hints.asArray;
    auto hint = hintsArray.find(node);

    if (hint == hintsArray.end()) {
        return false;
    }

    if (arrayName != nullptr)
        *arrayName = hint->second;

    return true;
}

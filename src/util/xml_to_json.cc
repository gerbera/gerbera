/*MT*

    MediaTomb - http://www.mediatomb.cc/

    xml_to_json.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2020 Patrick Ammann <pammann@gmx.net>

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

/// \file xml_to_json.cc
#define GRB_LOG_FAC GrbLogFacility::cds
#include "xml_to_json.h"

#include "exceptions.h"
#include "util/tools.h"

#include <fmt/format.h>
#include <regex>

std::string Xml2Json::getJson(const pugi::xml_node& node)
{
    std::vector<std::string> nodeBuf;
    nodeBuf.reserve(std::distance(node.attributes().begin(), node.attributes().end()));
    for (auto&& at : node.attributes()) {
        nodeBuf.push_back(fmt::format("{}:{}", getAsString(at.name()), getValue(at.name(), at.as_string())));
    }

    auto [array, nodeName] = isArray(node);
    std::vector<std::string> childBuf;
    if (array)
        childBuf.reserve(std::distance(node.children().begin(), node.children().end()));
    else
        nodeBuf.reserve(nodeBuf.size() + std::distance(node.children().begin(), node.children().end()));

    for (auto&& child : node.children()) {
        pugi::xml_node_type type = child.type();

        if (type == pugi::node_element) {
            if (array && nodeName != child.name()) {
                throw_std_runtime_error("if an element {} is of arrayType, all children have to have the same name (found: {})", nodeName, child.name());
            }

            // look ahead
            bool haveChildAttribute = std::distance(child.attributes().begin(), child.attributes().end()) > 0;
            bool haveChildElement = std::any_of(child.children().begin(), child.children().end(), [](auto&& el) { return el.type() == pugi::node_element; });
            auto childValue = (haveChildAttribute || haveChildElement || isArray(child).first)
                ? getJson(child)
                : getValue(child.name(), child.text().as_string());
            if (array) {
                childBuf.push_back(childValue);
            } else {
                nodeBuf.push_back(fmt::format("{}:{}", getAsString(child.name()), childValue));
            }
        }
    }

    if (array) {
        nodeBuf.push_back(fmt::format("{}:[{}]", getAsString(nodeName), fmt::join(childBuf, ",")));
    }

    return fmt::format("{{{}}}", fmt::join(nodeBuf, ","));
}

std::string Xml2Json::getAsString(std::string_view str)
{
    auto escaped = escape(str, '\\', '"');

    replaceAllString(escaped, "\n", "\\n");
    replaceAllString(escaped, "\r", "\\r");
    return fmt::format(R"("{}")", escaped);
}

std::string Xml2Json::getValue(const std::string& name, const char* text)
{
    auto hint = asType.find(name);

    if (hint != asType.end()) {
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

    std::string str = text;

    // boolean
    if (str == "true" || str == "false")
        return str;

    // number
    if (std::regex_match(str, std::regex("-?\\d+")))
        return str;

    return getAsString(text);
}

std::pair<bool, std::string_view> Xml2Json::isArray(const pugi::xml_node& node)
{
    auto hint = asArray.find(node);

    if (hint == asArray.end())
        return {};

    return { true, hint->second };
}

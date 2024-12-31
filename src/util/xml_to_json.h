/*MT*

    MediaTomb - http://www.mediatomb.cc/

    xml_to_json.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2020 Patrick Ammann <pammann@gmx.net>

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

/// \file xml_to_json.h

#ifndef __UTIL_XML_TO_JSON_H__
#define __UTIL_XML_TO_JSON_H__

#include <map>

#include <pugixml.hpp>

enum class FieldType {
    STRING,
    ENCSTRING,
    NUMBER,
    BOOL,
};

class Xml2Json {
public:
    std::string getJson(const pugi::xml_node& node);

    void setArrayName(const pugi::xml_node& node, std::string_view name) { asArray.insert_or_assign(node, name); }
    void setFieldType(const std::string& node, FieldType type) { asType.insert_or_assign(node, type); }

private:
    static std::string getAsString(std::string_view str);
    static std::string getEncString(std::string_view str);
    std::string getValue(const std::string& name, const char* text);
    std::pair<bool, std::string_view> isArray(const pugi::xml_node& node);

    std::map<pugi::xml_node, std::string> asArray;
    std::map<std::string, FieldType> asType;
};

#endif // __UTIL_XML_TO_JSON_H__

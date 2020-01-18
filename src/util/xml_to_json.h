/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_to_json.h - this file is part of MediaTomb.
    
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

/// \file xml_to_json.h

#ifndef __UTIL_XML_TO_JSON_H__
#define __UTIL_XML_TO_JSON_H__

#include <string>
#include <stack>
#include <sstream>
#include <memory>

#include <pugixml.hpp>

class Xml2Json
{
public:
    class Hints {
    public:
        void setArrayName(pugi::xml_node& node, const std::string& name) { asArray[node] = name; }
    private:
        std::map<pugi::xml_node, std::string> asArray;
        friend class Xml2Json;
    };

    static std::string getJson(pugi::xml_node& root, const Hints* hints);

private:
    static void handleElement(std::ostringstream& buf, pugi::xml_node& node, const Hints* hints);
    static std::string getAsString(const char* str);
    static std::string getValue(const char* text);
    static bool isArray(pugi::xml_node& node, const Hints* hints, std::string* arrayName);
};

#endif // __UTIL_XML_TO_JSON_H__

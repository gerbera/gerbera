/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_to_json.h - this file is part of MediaTomb.
    
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

/// \file xml_to_json.h

#ifndef __MXML_XML_TO_JSON_H__
#define __MXML_XML_TO_JSON_H__

#include "zmm/zmmf.h"

#include "mxml.h"

namespace mxml
{

class XML2JSON : public zmm::Object
{
protected:
    static void handleElement(std::ostringstream &buf, zmm::Ref<Element> el);
    static std::string getValue(std::string text, enum mxml_value_type type);
public:
    static std::string getJSON(zmm::Ref<Element> root);
    
};

} // namespace

#endif // __MXML_XML_TO_JSON_H__

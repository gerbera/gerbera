/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    attribute.cc - this file is part of MediaTomb.
    
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

/// \file attribute.cc

#include "mxml.h"
#include "attribute.h"

using namespace mxml;
using namespace zmm;

/* XmlAttr methods */
Attribute::Attribute(std::string name, std::string value) : Object()
{
    this->name = name;
    this->value = value;
    vtype = mxml_string_type;
}
Attribute::Attribute(std::string name) : Object()
{
    this->name = name;
    vtype = mxml_string_type;
}
Attribute::Attribute(std::string name, std::string value, enum mxml_value_type vtype) : Object()
{
    this->name = name;
    this->value = value;
    this->vtype = vtype;
}
void Attribute::setValue(std::string value)
{
    this->value = value;
}

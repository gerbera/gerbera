/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_text.cc - this file is part of MediaTomb.
    
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

/// \file xml_text.cc

#include "mxml.h"
#include "xml_text.h"

using namespace zmm;
using namespace mxml;

Text::Text(String text) : Node()
{
    type = mxml_node_text;
    this->text = text;
    vtype = mxml_string_type;
}

Text::Text(String text, enum mxml_value_type vtype) : Node()
{
    type = mxml_node_text;
    this->text = text;
    this->vtype = vtype;
}

void Text::print_internal(Ref<StringBuffer> buf, int indent)
{
    *buf << escape(text);
}

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_text.h - this file is part of MediaTomb.
    
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

/// \file xml_text.h

#ifndef __MXML_XML_TEXT_H__
#define __MXML_XML_TEXT_H__

#include "zmmf/zmmf.h"

#include "mxml.h"

#include "node.h"

namespace mxml
{

class Text : public Node
{
protected:
    zmm::String text;
    enum mxml_value_type vtype;

public:
    Text(zmm::String text);
    Text(zmm::String text, enum mxml_value_type vtype);
    inline zmm::String getText() { return text; }
    inline void setText(zmm::String text) { this->text = text; }
    inline enum mxml_value_type getVType() { return vtype; }
    inline void setVType(enum mxml_value_type vtype) { this->vtype = vtype; }

protected:
    virtual void print_internal(zmm::Ref<zmm::StringBuffer> buf, int indent);
};

} // namespace

#endif // __MXML_XML_TEXT_H__

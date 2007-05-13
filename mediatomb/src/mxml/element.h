/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    element.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file element.h

#ifndef __MXML_ELEMENT_H__
#define __MXML_ELEMENT_H__

#include "zmmf/zmmf.h"

#include "mxml.h"

namespace mxml
{

class Element : public zmm::Object
{
public:
    zmm::String name;
    zmm::String text;
    zmm::Ref<zmm::Array<Attribute> > attributes;
    zmm::Ref<zmm::Array<Element> > children;
    zmm::Ref<Context> context;

public:
    Element(zmm::String name);
    Element(zmm::String name, zmm::Ref<Context> context);
    zmm::String getAttribute(zmm::String name);
    void addAttribute(zmm::Ref<Attribute> attr);
    void addAttribute(zmm::String name, zmm::String value);
    void setAttribute(zmm::String name, zmm::String value);
    zmm::String getText();

    zmm::String getName();
    void setName(zmm::String name);

    int childCount();
    int attributeCount();
    zmm::Ref<Element> getChild(int index);
    zmm::Ref<Attribute> getAttribute(int index);
        
    zmm::Ref<Element> getFirstChild();

    void appendChild(zmm::Ref<Element> child);
    void setText(zmm::String text);

    void appendTextChild(zmm::String name, zmm::String text);
    zmm::String print();

    zmm::Ref<Element> getChild(zmm::String name);
    zmm::String getChildText(zmm::String name);
    
protected:
    void print(zmm::Ref<zmm::StringBuffer> buf, int indent);

    static zmm::String escape(zmm::String str);
};


} // namespace

#endif // __MXML_ELEMENT_H__

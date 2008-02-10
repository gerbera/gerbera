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

#include "node.h"

namespace mxml
{

class Element : public Node
{
protected:
    zmm::String name;
    zmm::Ref<zmm::Array<Attribute> > attributes;

public:
    Element(zmm::String name);
    Element(zmm::String name, zmm::Ref<Context> context);
    zmm::String getAttribute(zmm::String name);
    void addAttribute(zmm::Ref<Attribute> attr);
    void addAttribute(zmm::String name, zmm::String value);
    void setAttribute(zmm::String name, zmm::String value);
    zmm::String getText();

    inline zmm::String getName() { return name; }
    void setName(zmm::String name) { this->name = name; }

    int attributeCount();
    zmm::Ref<Attribute> getAttribute(int index);
        
    void setText(zmm::String text);

    int childCount(enum mxml_node_types type = mxml_node_all);
    zmm::Ref<Node> getChild(int index, enum mxml_node_types type = mxml_node_all, bool remove = false);
    zmm::Ref<Node> getFirstChild(enum mxml_node_types type = mxml_node_all) { return getChild(0, type); }
    void removeChild(int index, enum mxml_node_types type = mxml_node_all);
    void appendChild(zmm::Ref<Node> child);
    void insertChild(int index, zmm::Ref<Node> child);
    
    void removeWhitespace();
    void indent(int level = 0);
    
    zmm::Ref<Element> getFirstElementChild() { return getElementChild(0); }
    zmm::Ref<Element> getElementChild(int index) { return RefCast(getChild(index, mxml_node_element), Element); }
    int elementChildCount() { return childCount(mxml_node_element); }
    
    void removeElementChild(int index) { removeChild(index, mxml_node_element); }
    bool removeElementChild(zmm::String name, bool removeAll);
    
    void appendElementChild(zmm::Ref<Element> child) { appendChild(RefCast(child, Node)); };
    void appendTextChild(zmm::String name, zmm::String text);

    int getChildIdByName(zmm::String name);
    zmm::Ref<Element> getChildByName(zmm::String name);
    zmm::String getChildText(zmm::String name);
    
protected:
    virtual void print_internal(zmm::Ref<zmm::StringBuffer> buf, int indent);
};


} // namespace

#endif // __MXML_ELEMENT_H__

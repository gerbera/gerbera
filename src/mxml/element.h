/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    element.h - this file is part of MediaTomb.
    
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

/// \file element.h

#ifndef __MXML_ELEMENT_H__
#define __MXML_ELEMENT_H__

#include "zmm/zmmf.h"

#include "mxml.h"

#include "node.h"

namespace mxml
{

class Element : public Node
{
protected:
    std::string name;
    zmm::Ref<zmm::Array<Attribute> > attributes;
    void addAttribute(zmm::Ref<Attribute> attr);
    void addAttribute(std::string name, std::string value, enum mxml_value_type type = mxml_string_type);
    bool arrayType; // for JSON support
    std::string arrayName; // for JSON support
    std::string textKey; // for JSON support
    
public:
    Element(std::string name);
    Element(std::string name, zmm::Ref<Context> context);
    std::string getAttribute(std::string name);
    void setAttribute(std::string name, std::string value, enum mxml_value_type type = mxml_string_type);
    std::string getText();
    enum mxml_value_type getVTypeText();

    inline std::string getName() { return name; }
    void setName(std::string name) { this->name = name; }

    int attributeCount();
    zmm::Ref<Attribute> getAttribute(int index);
        
    void setText(std::string text, enum mxml_value_type type = mxml_string_type);

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
    bool removeElementChild(std::string name, bool removeAll);
    
    void appendElementChild(zmm::Ref<Element> child) { appendChild(RefCast(child, Node)); };
    void appendTextChild(std::string name, std::string text, enum mxml_value_type type = mxml_string_type);

    int getChildIdByName(std::string name);
    zmm::Ref<Element> getChildByName(std::string name);
    std::string getChildText(std::string name);
    
    bool isArrayType() { return arrayType; }
    //void setArrayType(bool arrayType) { this->arrayType = arrayType; }
    
    std::string getArrayName() { return arrayName; }
    void setArrayName(std::string arrayName) { arrayType = true; this->arrayName = arrayName; }
    
    std::string getTextKey() { return textKey; }
    void setTextKey(std::string textKey) { this->textKey = textKey; }
    
protected:
    void print_internal(std::ostringstream &buf, int indent) override;
    
    friend class Parser;
};


} // namespace

#endif // __MXML_ELEMENT_H__

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    element.cc - this file is part of MediaTomb.
    
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

/// \file element.cc

#include <assert.h>

#include "element.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

Element::Element(String name) : Node()
{
    type = mxml_node_element;
    this->name = name;
    arrayType = false;
    arrayName = nil;
    textKey = nil;
}
Element::Element(String name, Ref<Context> context) : Node()
{
    type = mxml_node_element;
    this->name = name;
    this->context = context;
    arrayType = false;
    arrayName = nil;
    textKey = nil;
}
String Element::getAttribute(String name)
{
    if(attributes == nil)
        return nil;
    int len = attributes->size();
    for(int i = 0; i < len; i++)
    {
        Ref<Attribute> attr = attributes->get(i);
        if(attr->name == name)
            return attr->value;
    }
    return nil;
}
void Element::addAttribute(String name, String value, enum mxml_value_type type)
{
    Ref<Attribute> attr = Ref<Attribute>(new Attribute(name, value, type));
    addAttribute(attr);
}

void Element::addAttribute(Ref<Attribute> attr)
{
    if (attributes == nil)
        attributes = Ref<Array<Attribute> >(new Array<Attribute>());
    attributes->append(attr);
}

void Element::setAttribute(String name, String value, enum mxml_value_type type)
{
    if (attributes == nil)
        attributes = Ref<Array<Attribute> >(new Array<Attribute>());
    int len = attributes->size();
    for(int i = 0; i < len; i++)
    {
        Ref<Attribute> attr = attributes->get(i);
        if(attr->name == name)
        {
            attr->setValue(value);
            attr->setVType(type);
            return;
        }
    }
    addAttribute(name, value, type);
}

int Element::childCount(enum mxml_node_types type)
{
    if (children == nil)
        return 0;
    
    if (type == mxml_node_all)
        return children->size();
    
    int countElements = 0;
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Node> nd = children->get(i);
        if (nd->getType() == type)
        {
            countElements++;
        }
    }
    return countElements;
}

Ref<Node> Element::getChild(int index, enum mxml_node_types type, bool remove)
{
    if (children == nil)
        return nil;
    int countElements = 0;
    
    if (type == mxml_node_all)
    {
        if (index >= children->size())
            return nil;
        else
        {
            Ref<Node> node = children->get(index);
            if (remove)
                children->remove(index);
            return node;
        }
    }
    
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Node> nd = children->get(i);
        if (nd->getType() == type)
        {
            if (countElements++ == index)
            {
                if (remove)
                    children->remove(i);
                return nd;
            }
        }
    }
    return nil;
}

bool Element::removeElementChild(String name, bool removeAll)
{
    int id = getChildIdByName(name);
    if (id < 0)
        return false;
    Ref<Node> child = getChild(id, mxml_node_all, true);
    if (child == nil)
        return false;
    if (! removeAll)
        return true;
    removeElementChild(name, true);
    return true;
}

void Element::appendChild(Ref<Node> child)
{
    if(children == nil)
        children = Ref<Array<Node> >(new Array<Node>());
    children->append(child);
}

void Element::insertChild(int index, Ref<Node> child)
{
    if (children == nil)
        children = Ref<Array<Node> >(new Array<Node>());
    children->insert(index, child);
}

void Element::removeChild(int index, enum mxml_node_types type)
{
    if (type == mxml_node_all)
        children->remove(index);
    else
    {
        getChild(index, type, true);
    }
}

void Element::removeWhitespace()
{
    int numChildren = childCount();
    for (int i = 0; i < numChildren; i++)
    {
        Ref<Node> node = getChild(i);
        if (node->getType() == mxml_node_text)
        {
            Ref<Text> text = RefCast(node, Text);
            String trimmed = trim_string(text->getText());
            if (string_ok(trimmed))
            {
                text->setText(trimmed);
            }
            else
            {
                if (numChildren != 1)
                {
                    removeChild(i--);
                    --numChildren;
                }
            }
        }
        else if (node->getType() == mxml_node_element)
        {
            Ref<Element> el = RefCast(node, Element);
            el->removeWhitespace();
        }
    }
}

void Element::indent(int level)
{
    assert(level >= 0);
    
    removeWhitespace();
    
    int numChildren = childCount();
    if (! numChildren)
        return;
    
    bool noTextChildren = true;
    for (int i = 0; i < numChildren; i++)
    {
        Ref<Node> node = getChild(i);
        if (node->getType() == mxml_node_element)
        {
            Ref<Element> el = RefCast(node, Element);
            el->indent(level+1);
        }
        else if (node->getType() == mxml_node_text)
        {
            noTextChildren = false;
        }
    }
    
    if (noTextChildren)
    {
        static const char *ind_str = "                                                               ";
        static const char *ind = ind_str + strlen(ind_str);
        const char *ptr = ind - (level + 1) * 2;
        if (ptr < ind_str)
            ptr = ind_str;
        
        for (int i = 0; i < numChildren; i++)
        {
            bool newlineBefore = true;
            if (getChild(i)->getType() == mxml_node_comment)
            {
                Ref<Comment> comment = RefCast(getChild(i), Comment);
                newlineBefore = comment->getIndentWithLFbefore();
            }
            if (newlineBefore)
            {
                Ref<Text> indentText(new Text(_("\n")+ptr));
                insertChild(i++,RefCast(indentText, Node));
                numChildren++;
            }
        }
        
        ptr += 2;
        
        Ref<Text> indentTextAfter(new Text(_("\n")+ptr));
        appendChild(RefCast(indentTextAfter, Node));
    }
}

String Element::getText()
{
    Ref<StringBuffer> buf(new StringBuffer());
    Ref<Text> text;
    int i = 0;
    bool someText = false;
    while ((text = RefCast(getChild(i++, mxml_node_text), Text)) != nil)
    {
        someText = true;
        *buf << text->getText();
    }
    if (someText)
        return buf->toString();
    else
        return nil;
}

enum mxml_value_type Element::getVTypeText()
{
    Ref<Text> text;
    int i = 0;
    bool someText = false;
    enum mxml_value_type vtype = mxml_string_type;
    while ((text = RefCast(getChild(i++, mxml_node_text), Text)) != nil)
    {
        if (! someText)
        {
            someText = true;
            vtype = text->getVType();
        }
        else
        {
            if (vtype != text->getVType())
                vtype = mxml_string_type;
        }
    }
    return vtype;
}

int Element::attributeCount()
{
    if (attributes == nil)
        return 0;
    return attributes->size();
}

Ref<Attribute> Element::getAttribute(int index)
{
    if (attributes == nil)
        return nil;
    if (index >= attributes->size())
        return nil;
    return attributes->get(index);
}

void Element::setText(String str, enum mxml_value_type type)
{
    if (childCount() > 1)
        throw _Exception(_("Element::setText() cannot be called on an element which has more than one child"));
    
    if (childCount() == 1)
    {
        Ref<Node> child = getChild(0);
        if (child == nil || child->getType() != mxml_node_text)
            throw _Exception(_("Element::setText() cannot be called on an element which has a non-text child"));
        Ref<Text> text = RefCast(child, Text);
        text->setText(str);
    }
    else
    {
        Ref<Text> text(new Text(str, type));
        appendChild(RefCast(text, Node));
    }
}

void Element::appendTextChild(String name, String text, enum mxml_value_type type)
{
    Ref<Element> el = Ref<Element>(new Element(name));
    el->setText(text, type);
    appendElementChild(el);
}



int Element::getChildIdByName(String name)
{
    if(children == nil)
        return -1;
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Node> nd = children->get(i);
        if (nd->getType() == mxml_node_element)
        {
            Ref<Element> el = RefCast(nd, Element);
            if (name == nil || el->name == name)
                return i;
        }
    }
    return -1;
}

Ref<Element> Element::getChildByName(String name)
{
    int id = getChildIdByName(name);
    if (id < 0)
        return nil;
    return RefCast(getChild(id), Element);
}

String Element::getChildText(String name)
{
    Ref<Element> el = getChildByName(name);
    if(el == nil)
        return nil;
    return el->getText();
}

void Element::print_internal(Ref<StringBuffer> buf, int indent)
{
    /*
    static char *ind_str = "                                                               ";
    static char *ind = ind_str + strlen(ind_str);
    char *ptr = ind - indent * 2;
    *buf << ptr;
    */
    
    int i;
    
    *buf << "<" << name;
    if (attributes != nil)
    {
        for(i = 0; i < attributes->size(); i++)
        {
            *buf << ' ';
            Ref<Attribute> attr = attributes->get(i);
            *buf << attr->name << "=\"" << escape(attr->value) << '"';
        }
    }
    
    if (children != nil && children->size())
    {
        *buf << ">";
        
        for(i = 0; i < children->size(); i++)
        {
            children->get(i)->print_internal(buf, indent + 1);
        }
        
        *buf << "</" << name << ">";
    }
    else
    {
        *buf << "/>";
    }
}

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    element.cc - this file is part of MediaTomb.
    
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

/// \file element.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "element.h"

#include <string.h>

using namespace zmm;
using namespace mxml;

Element::Element(String name) : Node()
{
    type = mxml_node_element;
    this->name = name;
}
Element::Element(String name, Ref<Context> context) : Node()
{
    type = mxml_node_element;
    this->name = name;
    this->context = context;
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
void Element::addAttribute(String name, String value)
{
    Ref<Attribute> attr = Ref<Attribute>(new Attribute(name, value));
    addAttribute(attr);
}

void Element::addAttribute(Ref<Attribute> attr)
{
    if (attributes == nil)
        attributes = Ref<Array<Attribute> >(new Array<Attribute>());
    attributes->append(attr);
}

void Element::setAttribute(String name, String value)
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
            return;
        }
    }
    addAttribute(name, value);
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

void Element::setText(String str)
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
        Ref<Text> text(new Text(str));
        appendChild(RefCast(text, Node));
    }
}

void Element::appendTextChild(String name, String text)
{
    Ref<Element> el = Ref<Element>(new Element(name));
    el->setText(text);
    appendElementChild(el);
}

Ref<Element> Element::getChildByName(String name)
{
    if(children == nil)
        return nil;
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Node> nd = children->get(i);
        if (nd->getType() == mxml_node_element)
        {
            Ref<Element> el = RefCast(nd, Element);
            if (name == nil || el->name == name)
                return el;
        }
    }
    return nil;
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
    if(attributes != nil)
    {
        for(i = 0; i < attributes->size(); i++)
        {
            *buf << ' ';
            Ref<Attribute> attr = attributes->get(i);
            *buf << attr->name << "=\"" << escape(attr->value) << '"';
        }
    }
    
    if(children != nil && children->size())
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
        *buf << "/>\n";
    }
}


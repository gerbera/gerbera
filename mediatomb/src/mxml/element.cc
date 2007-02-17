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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
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

Element::Element(String name) : Object()
{
    this->name = name;
}
Element::Element(String name, Ref<Context> context) : Object()
{
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
    return text;
}

int Element::childCount()
{
    if (children == nil)
        return 0;
    return children->size();
}

int Element::attributeCount()
{
    if (attributes == nil)
        return 0;
    return attributes->size();
}

Ref<Element> Element::getChild(int index)
{
    if (children == nil)
        return nil;
    if (index >= children->size())
        return nil;
    return children->get(index);
}

Ref<Attribute> Element::getAttribute(int index)
{
    if (attributes == nil)
        return nil;
    if (index >= attributes->size())
        return nil;
    return attributes->get(index);
}



Ref<Element> Element::getFirstChild()
{
    return children->get(0);
}

void Element::appendChild(Ref<Element> child)
{
    if(children == nil)
        children = Ref<Array<Element> >(new Array<Element>());
    children->append(child);
}
void Element::setText(String text)
{
    this->text = text;
}

void Element::appendTextChild(String name, String text)
{
    Ref<Element> el = Ref<Element>(new Element(name));
    el->setText(text);
    appendChild(el);
}

Ref<Element> Element::getChild(String name)
{
    if(children == nil)
        return nil;
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Element> el = children->get(i);
        if(el->name == name)
            return el;
    }
    return nil;
}

String Element::getChildText(String name)
{
    Ref<Element> el = getChild(name);
    if(el == nil)
        return nil;
    return el->getText();
}

String Element::getName()
{
    return name;
}
void Element::setName(String name)
{
    this->name = name;
}

String Element::print()
{
    Ref<StringBuffer> buf(new StringBuffer());
    print(buf, 0);
    return buf->toString();
}

void Element::print(Ref<StringBuffer> buf, int indent)
{
    static char *ind_str = "                                                               ";
    static char *ind = ind_str + strlen(ind_str);
    char *ptr = ind - indent * 2;
    *buf << ptr;

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
    if(text != nil)
    {
        *buf << '>' << escape(text) << "</" << name << ">\n";
    }
    else if(children != nil && children->size())
    {
        *buf << ">\n";
        for(i = 0; i < children->size(); i++)
        {
            children->get(i)->print(buf, indent + 1);
        }
        *buf << ptr << "</" << name << ">\n";
    }
    else
    {
        *buf << "/>\n";
    }
}


String Element::escape(String str)
{
    Ref<StringBuffer> buf(new StringBuffer(str.length()));
    signed char *ptr = (signed char *)str.c_str();
    while (*ptr)
    {
        switch (*ptr)
        {
            case '<' : *buf << "&lt;"; break;
            case '>' : *buf << "&gt;"; break;
            case '&' : *buf << "&amp;"; break;
            case '"' : *buf << "&quot;"; break;
            case '\'' : *buf << "&apos;"; break;
                       // handle control codes
            default  : if (((*ptr >= 0x00) && (*ptr <= 0x1f) && 
                            (*ptr != 0x09) && (*ptr != 0x0d) && 
                            (*ptr != 0x0a)) || (*ptr == 0x7f))
                       {
                           *buf << '.';
                       }
                       else
                           *buf << *ptr;
                       break;
        }
        ptr++;
    }
    return buf->toString();
}



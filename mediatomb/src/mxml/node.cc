/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    node.cc - this file is part of MediaTomb.
    
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

/// \file node.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "mxml.h"

#include <string.h>

using namespace zmm;
using namespace mxml;

int Node::childCount(enum mxml_node_types type)
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

Ref<Node> Node::getChild(int index, enum mxml_node_types type)
{
    if (children == nil)
        return nil;
    int countElements = 0;
    
    if (type == mxml_node_all)
    {
        if (index >= children->size())
            return nil;
        else
            return children->get(index);
    }
    
    for(int i = 0; i < children->size(); i++)
    {
        Ref<Node> nd = children->get(i);
        if (nd->getType() == type)
        {
            if (countElements++ == index)
                return nd;
        }
    }
    return nil;
}

void Node::appendChild(Ref<Node> child)
{
    if(children == nil)
        children = Ref<Array<Node> >(new Array<Node>());
    children->append(child);
}

String Node::print()
{
    Ref<StringBuffer> buf(new StringBuffer());
    print_internal(buf, 0);
    return buf->toString();
}

String Node::escape(String str)
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

/*
void Node::print_internal(Ref<StringBuffer> buf, int indent)
{
    static char *ind_str = "                                                               ";
    static char *ind = ind_str + strlen(ind_str);
    char *ptr = ind - indent * 2;
    *buf << ptr;
}
*/

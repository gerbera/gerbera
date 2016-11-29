/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    document.cc - this file is part of MediaTomb.
    
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

/// \file document.cc

#include "mxml.h"
#include "document.h"

using namespace zmm;
using namespace mxml;

Document::Document() : Node()
{
    type = mxml_node_document;
    root = nil;
}

void Document::setRoot(Ref<Element> root)
{
    this->root = root;
    appendChild(RefCast(root, Node));
}

Ref<Element> Document::getRoot()
{
    return root;
}

void Document::appendChild(Ref<Node> child)
{
    if(children == nil)
        children = Ref<Array<Node> >(new Array<Node>());
    children->append(child);
}

void Document::print_internal(Ref<StringBuffer> buf, int indent)
{
    if (children != nil && children->size())
    {
        for(int i = 0; i < children->size(); i++)
        {
            children->get(i)->print_internal(buf, indent + 1);
        }
    }
}

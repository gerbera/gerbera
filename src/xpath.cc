/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xpath.cc - this file is part of MediaTomb.
    
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

/// \file xpath.cc

#include "common.h"
#include "tools.h"
#include "xpath.h"

using namespace zmm;
using namespace mxml;

XPath::XPath(Ref<Element> context) : Object()
{
    this->context = context;
}

Ref<Element> XPath::getElement(String xpath)
{
    String axisPart = getAxisPart(xpath);
    if (axisPart != nil)
    {
        throw _Exception(_("XPath::getElement: unexpected axis in ") + xpath);
    }
    return elementAtPath(xpath);
}

String XPath::getText(String xpath)
{
    String axisPart = getAxisPart(xpath);
    String pathPart = getPathPart(xpath);
    
    Ref<Element> el = elementAtPath(pathPart);
    if (el == nil)
        return nil;
    
    if (axisPart == nil)
        return el->getText();
    
    String axis = getAxis(axisPart);
    String spec = getSpec(axisPart);

    if (axis != "attribute")
    {
        throw _Exception(_("XPath::getText: unexpected axis: ") + axis);
    }
   
    return el->getAttribute(spec);
}

String XPath::getPathPart(String xpath)
{
    int slashPos = xpath.rindex('/');
    if (slashPos < 0)
        return xpath;
    if (strstr(xpath.c_str() + slashPos, "::"))
    {
        return xpath.substring(0, slashPos);
    }
    return xpath;
}

Ref<Element> XPath::elementAtPath(String path)
{
    Ref<Element> cur = context;
    Ref<Array<StringBase> > parts = split_string(path, '/');
   
    for (int i = 0; i < parts->size(); i++)
    {
        String part = parts->get(i);
        cur = cur->getChildByName(part);
        if (cur == nil)
            break;
    }
    return cur;
}

String XPath::getAxisPart(String xpath)
{
    int slashPos = xpath.rindex('/');
    if (slashPos < 0)
        slashPos = 0;
    if (strstr(xpath.c_str() + slashPos, "::"))
    {
        return xpath.substring(slashPos + 1);
    }
    return nil;
}

String XPath::getAxis(String axisPart)
{
    char *pos = strstr(axisPart.c_str(), "::");
    return axisPart.substring(0, pos - axisPart.c_str());
}

String XPath::getSpec(String axisPart)
{
    char *pos = strstr(axisPart.c_str(), "::");
    return pos + 2;
}

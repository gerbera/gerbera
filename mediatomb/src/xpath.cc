/*  xpath.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

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
        throw Exception(_("XPath::getElement: unexpected axis in ") + xpath);
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
        throw Exception(String("XPath::getText: unexpected axis: ") + axis);
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
        cur = cur->getChild(part);
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
    return String(pos + 2);
}


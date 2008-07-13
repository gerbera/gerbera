/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_to_json.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file xml_to_json.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "xml_to_json.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

String XML2JSON::getJSON(Ref<Element> root)
{
    Ref<StringBuffer> buf(new StringBuffer());
    *buf << '{';
    handleElement(buf, root);
    *buf << '}';
    return buf->toString();
}


void XML2JSON::handleElement(Ref<StringBuffer> buf, Ref<Element> el)
{
    bool firstChild = true;
    int attributeCount = el->attributeCount();
    if (attributeCount > 0)
    {
        for (int i = 0; i < attributeCount; i++)
        {
            Ref<Attribute> at = el->getAttribute(i);
            if (! firstChild)
                *buf << ',';
            else
                firstChild = false;
            *buf << '"' << escape(at->name, '\\', '"') << "\":\"" << escape(at->value, '\\', '"') << '"';
        }
    }
    
    int childCount = el->childCount();
    
    bool array = false;
    String nodeName = nil;
    
    for (int i = 0; i < childCount; i++)
    {
        Ref<Node> node = el->getChild(i);
        mxml_node_types type = node->getType();
        if (type != mxml_node_element)
        {
            if (childCount == 1 && type == mxml_node_text)
            {
                if (! firstChild)
                    *buf << ',';
                else
                    firstChild = false;
                *buf << "\"value\":\"" << escape(el->getText(), '\\', '"') << '"';
            }
            else
                throw _Exception(_("XML2JSON cannot handle an element which consists of text AND element children - element: ") + el->getName() + "; has type: " + type);
        }
        else
        {
            Ref<Element> childEl = RefCast(node, Element);
            int childAttributeCount = childEl->attributeCount();
            int childElementCount = childEl->elementChildCount();
            
            if (! firstChild)
                *buf << ',';
            else
                firstChild = false;
            
            if (i == 0)
            {
                if (childCount > 1)
                {
                    Ref<Node> nextNode = el->getChild(1);
                    if (nextNode->getType() != mxml_node_element)
                        throw _Exception(_("XML2JSON cannot handle an element which consists of text AND element children"));
                    Ref<Element> nextChildEl = RefCast(nextNode, Element);
                    if (nextChildEl->getName() == childEl->getName())
                        array = true;
                }
                //else
                //    array = true;
                
                if (array)
                {
                    nodeName = childEl->getName();
                    *buf << '[';
                    firstChild = false;
                }
            }
            
            if (array)
            {
                if (i > 1) // we got the name from 0 and already checked with 1
                {
                    if (nodeName != childEl->getName())
                        throw _Exception(_("XML2JSON: if there are multiple elements of the same name (->array), there are no other elements allowed"));
                }
            }
            else
                *buf << '"' << escape(el->getName(), '\\', '"') << "\":";
            
            if (childAttributeCount > 0 || childElementCount > 0)
            {
                *buf << '{';
                handleElement(buf, childEl);
                *buf << '}';
            }
            else
            {
                *buf << '"' << escape(el->getText(), '\\', '"') << '"';
            }
        }
    }
    
    if (array)
        *buf << ']';
    
}

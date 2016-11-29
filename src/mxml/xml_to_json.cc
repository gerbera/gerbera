/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xml_to_json.cc - this file is part of MediaTomb.
    
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

/// \file xml_to_json.cc

#include <assert.h>

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
            *buf << '"' << escape(at->name, '\\', '"') << "\":" << getValue(at->value, at->getVType());
        }
    }
    
    bool array = el->isArrayType();
    String nodeName = nil;
    
    if (array)
    {
        nodeName = el->getArrayName();
        if (! string_ok(nodeName))
            throw _Exception(_("XML2JSON: Element ") + el->getName() + " was of arrayType, but had no arrayName set");
        
        if (! firstChild)
            *buf << ',';
        *buf << '"' << escape(nodeName, '\\', '"') << "\":";
        *buf << '[';
        firstChild = true;
    }
    
    int childCount = el->childCount();
    
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
                String key = el->getTextKey();
                if (! string_ok(key))
                    throw _Exception(_("XML2JSON: Element ") + el->getName() + " had a text child, but had no textKey set");
                    //key = _("value");
                
                *buf << '"' << key << "\":" << getValue(el->getText(), el->getVTypeText());
            }
            else
                throw _Exception(_("XML2JSON cannot handle an element which consists of text AND element children - element: ") + el->getName() + "; has type: " + type);
        }
        else
        {
            if (! firstChild)
                *buf << ',';
            else
                firstChild = false;
            
            /*
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
            }
            */
            
            /*
            if (array)
            {
                if (i > 1) // we got the name from 0 and already checked with 1
                {
                    if (nodeName != childEl->getName())
                        throw _Exception(_("XML2JSON: if there are multiple elements of the same name (->array), there are no other elements allowed"));
                }
            }
            */
            
            
            
            Ref<Element> childEl = RefCast(node, Element);
            int childAttributeCount = childEl->attributeCount();
            int childElementCount = childEl->elementChildCount();
            
            if (array)
            {
                if (nodeName != childEl->getName())
                    throw _Exception(_("XML2JSON: if an element is of arrayType, all children have to have the same name"));
            }
            else
                *buf << '"' << escape(childEl->getName(), '\\', '"') << "\":";
            
            if (childAttributeCount > 0 || childElementCount > 0 || childEl->isArrayType())
            {
                *buf << '{';
                handleElement(buf, childEl);
                *buf << '}';
            }
            else
            {
                *buf << getValue(childEl->getText(), childEl->getVTypeText());
            }
        }
    }
    
    if (array)
        *buf << ']';
    
}

String XML2JSON::getValue(String text, enum mxml_value_type type)
{
    if (type == mxml_string_type)
        return _("\"") + escape(text, '\\', '"') + '"';
    if (type == mxml_bool_type)
    {
        assert(string_ok(text)); // must be real string
        assert(text == "0" || text == "1");  // must be bool type
        return text == "0" ? _("false") : _("true");
    }
    if (type == mxml_null_type)
    {
        assert(! string_ok(text)); // must not contain text
        return _("null");
    }
    
    if (type == mxml_int_type)
    {
        /// \todo should we check if really int?
        return text;
    }
    return nil;
}

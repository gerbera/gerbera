#include "common.h"
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
        throw Exception(String("XPath::getElement: unexpected axis in ") + xpath);
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
        throw Exception(String("XPath::getText: unexpected axus: ") + axis);
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


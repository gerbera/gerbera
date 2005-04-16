#ifndef __XPATH_H__
#define __XPATH_H__

#include "zmmf/zmmf.h"
#include "mxml/mxml.h"

class XPath : public zmm::Object
{
protected:
    zmm::Ref<mxml::Element> context;
public:
    XPath(zmm::Ref<mxml::Element> context);
    zmm::String getText(zmm::String xpath);
    zmm::Ref<mxml::Element> getElement(zmm::String xpath);

    static zmm::String getPathPart(zmm::String xpath);
    static zmm::String getAxisPart(zmm::String xpath);
    static zmm::String getAxis(zmm::String axisPart);
    static zmm::String getSpec(zmm::String axisPart);
protected:
    zmm::Ref<mxml::Element> elementAtPath(zmm::String path);
};

#endif // __XPATH_H__


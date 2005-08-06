#ifndef __CDS_RESOURCE_H__
#define __CDS_RESOURCE_H__

#include "common.h"
#include "dictionary.h"

#define RESOURCE_SEP '|'
#define RESOURCE_PART_SEP '~'

class CdsResource : public zmm::Object
{
protected:
    int handlerType;
    zmm::Ref<Dictionary> attributes;
    zmm::Ref<Dictionary> parameters;

public:
    CdsResource(int handlerType);
    CdsResource(int handlerType,
                zmm::Ref<Dictionary> attributes,
                zmm::Ref<Dictionary> parameters);
    
    void addAttribute(zmm::String name, zmm::String value);
    void addParameter(zmm::String name, zmm::String value);

    // urlencode into string
    int getHandlerType();
    zmm::Ref<Dictionary> getAttributes();
    zmm::Ref<Dictionary> getParameters();

    bool equals(zmm::Ref<CdsResource> other);

    zmm::String encode();
    static zmm::Ref<CdsResource> decode(zmm::String serial);

};

#endif // __CDS_RESOURCE_H__


/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    cds_resource.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
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

/// \file cds_resource.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "tools.h"
#include "cds_resource.h"

using namespace zmm;

CdsResource::CdsResource(int handlerType) : Object()
{
    this->handlerType = handlerType;
    this->attributes = Ref<Dictionary>(new Dictionary());
    this->parameters = Ref<Dictionary>(new Dictionary());
}
CdsResource::CdsResource(int handlerType,
                         Ref<Dictionary> attributes,
                         Ref<Dictionary> parameters)
{
    this->handlerType = handlerType;
    this->attributes = attributes;
    this->parameters = parameters;
}

void CdsResource::addAttribute(zmm::String name, zmm::String value)
{
    attributes->put(name, value);
}
void CdsResource::addParameter(zmm::String name, zmm::String value)
{
    parameters->put(name, value);
}

int CdsResource::getHandlerType() 
{
    return handlerType;
}
Ref<Dictionary> CdsResource::getAttributes()
{
    return attributes;
}
Ref<Dictionary> CdsResource::getParameters()
{
    return parameters;
}
String CdsResource::getAttribute(String name)
{
    return attributes->get(name);
}
String CdsResource::getParameter(String name)
{
    return parameters->get(name);
}

bool CdsResource::equals(Ref<CdsResource> other)
{
    return (
        handlerType == other->handlerType &&
        attributes->equals(other->attributes) &&
        parameters->equals(other->parameters)
    );
}

Ref<CdsResource> CdsResource::clone()
{
    return Ref<CdsResource>(new CdsResource(handlerType,
                                            attributes,
                                            parameters));
}

String CdsResource::encode()
{
    // encode resources
    Ref<StringBuffer> buf(new StringBuffer());
    *buf << handlerType;
    *buf << RESOURCE_PART_SEP;
    *buf << attributes->encode();
    *buf << RESOURCE_PART_SEP;
    *buf << parameters->encode();
    return buf->toString();
}

Ref<CdsResource> CdsResource::decode(String serial)
{
    Ref<Array<StringBase> > parts = split_string(serial, RESOURCE_PART_SEP);
    int size = parts->size();
    if (size != 2 && size != 3)
        throw _Exception(_("CdsResource::decode: Could not parse resources"));

    int handlerType = String(parts->get(0)).toInt();

    Ref<Dictionary> attr(new Dictionary());
    attr->decode(parts->get(1));

    Ref<Dictionary> par(new Dictionary());

    if (size == 3)
        par->decode(parts->get(2));
    
    Ref<CdsResource> resource(new CdsResource(handlerType, attr, par));
    
    return resource;
}

void CdsResource::optimize()
{
    attributes->optimize();
    parameters->optimize();
}


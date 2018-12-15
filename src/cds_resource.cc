/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource.cc - this file is part of MediaTomb.
    
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

/// \file cds_resource.cc

#include "cds_resource.h"
#include "tools.h"

#include <sstream>

#define RESOURCE_PART_SEP '~'

using namespace zmm;

CdsResource::CdsResource(int handlerType)
    : Object()
{
    this->handlerType = handlerType;
    this->attributes = Ref<Dictionary>(new Dictionary());
    this->parameters = Ref<Dictionary>(new Dictionary());
    this->options = Ref<Dictionary>(new Dictionary());
}
CdsResource::CdsResource(int handlerType,
    Ref<Dictionary> attributes,
    Ref<Dictionary> parameters,
    Ref<Dictionary> options)
{
    this->handlerType = handlerType;
    this->attributes = attributes;
    this->parameters = parameters;
    this->options = options;
}

void CdsResource::addAttribute(String name, String value)
{
    attributes->put(name, value);
}

void CdsResource::removeAttribute(String name)
{
    attributes->remove(name);
}

void CdsResource::mergeAttributes(Ref<Dictionary> additional)
{
    attributes->merge(additional);
}

void CdsResource::addParameter(String name, String value)
{
    parameters->put(name, value);
}

void CdsResource::addOption(String name, String value)
{
    options->put(name, value);
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

Ref<Dictionary> CdsResource::getOptions()
{
    return options;
}

String CdsResource::getAttribute(String name)
{
    return attributes->get(name);
}

String CdsResource::getParameter(String name)
{
    return parameters->get(name);
}

String CdsResource::getOption(String name)
{
    return options->get(name);
}

bool CdsResource::equals(Ref<CdsResource> other)
{
    return (
        handlerType == other->handlerType && attributes->equals(other->attributes) && parameters->equals(other->parameters) && options->equals(other->options));
}

Ref<CdsResource> CdsResource::clone()
{
    return Ref<CdsResource>(new CdsResource(handlerType,
        attributes->clone(),
        parameters->clone(),
        options->clone()));
}

String CdsResource::encode()
{
    // encode resources
    std::ostringstream buf;
    buf << handlerType;
    buf << RESOURCE_PART_SEP;
    buf << attributes->encode();
    buf << RESOURCE_PART_SEP;
    buf << parameters->encode();
    buf << RESOURCE_PART_SEP;
    buf << options->encode();
    return buf.str().c_str();
}

Ref<CdsResource> CdsResource::decode(String serial)
{
    Ref<Array<StringBase>> parts = split_string(serial, RESOURCE_PART_SEP, true);
    int size = parts->size();
    if (size < 2 || size > 4)
        throw _Exception(_("CdsResource::decode: Could not parse resources"));

    int handlerType = String(parts->get(0)).toInt();

    Ref<Dictionary> attr(new Dictionary());
    attr->decode(parts->get(1));

    Ref<Dictionary> par(new Dictionary());

    if (size >= 3)
        par->decode(parts->get(2));

    Ref<Dictionary> opt(new Dictionary());

    if (size >= 4)
        opt->decode(parts->get(3));

    Ref<CdsResource> resource(new CdsResource(handlerType, attr, par, opt));

    return resource;
}

void CdsResource::optimize()
{
    attributes->optimize();
    parameters->optimize();
    options->optimize();
}

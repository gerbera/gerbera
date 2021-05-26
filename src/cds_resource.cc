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

#include "cds_resource.h" // API

#include <sstream>

#include "util/tools.h"

#define RESOURCE_PART_SEP '~'

CdsResource::CdsResource(int handlerType)
    : handlerType(handlerType)
{
}
CdsResource::CdsResource(int handlerType,
    std::map<std::string, std::string> attributes,
    std::map<std::string, std::string> parameters,
    std::map<std::string, std::string> options)
    : handlerType(handlerType)
    , attributes(std::move(attributes))
    , parameters(std::move(parameters))
    , options(std::move(options))
{
}

void CdsResource::addAttribute(resource_attributes_t res, std::string value)
{
    attributes[MetadataHandler::getResAttrName(res)] = std::move(value);
}

void CdsResource::removeAttribute(resource_attributes_t res)
{
    attributes.erase(MetadataHandler::getResAttrName(res));
}

void CdsResource::mergeAttributes(const std::map<std::string, std::string>& additional)
{
    for (auto&& [key, val] : additional) {
        attributes[key] = val;
    }
}

void CdsResource::addParameter(const std::string& name, std::string value)
{
    parameters[name] = std::move(value);
}

void CdsResource::addOption(const std::string& name, std::string value)
{
    options[name] = std::move(value);
}

int CdsResource::getHandlerType() const
{
    return handlerType;
}

std::map<std::string, std::string> CdsResource::getAttributes() const
{
    return attributes;
}

std::map<std::string, std::string> CdsResource::getParameters() const
{
    return parameters;
}

std::map<std::string, std::string> CdsResource::getOptions() const
{
    return options;
}

std::string CdsResource::getAttribute(resource_attributes_t res) const
{
    return getValueOrDefault(attributes, MetadataHandler::getResAttrName(res));
}

std::string CdsResource::getParameter(const std::string& name) const
{
    return getValueOrDefault(parameters, name);
}

std::string CdsResource::getOption(const std::string& name) const
{
    return getValueOrDefault(options, name);
}

bool CdsResource::equals(const std::shared_ptr<CdsResource>& other) const
{
    return (
        handlerType == other->handlerType
        && std::equal(attributes.begin(), attributes.end(), other->attributes.begin())
        && std::equal(parameters.begin(), parameters.end(), other->parameters.begin())
        && std::equal(options.begin(), options.end(), other->options.begin()));
}

std::shared_ptr<CdsResource> CdsResource::clone()
{
    return std::make_shared<CdsResource>(handlerType, attributes, parameters, options);
}

std::string CdsResource::encode()
{
    // encode resources
    std::ostringstream buf;
    buf << handlerType;
    buf << RESOURCE_PART_SEP;
    buf << dictEncode(attributes);
    buf << RESOURCE_PART_SEP;
    buf << dictEncode(parameters);
    buf << RESOURCE_PART_SEP;
    buf << dictEncode(options);
    return buf.str();
}

std::shared_ptr<CdsResource> CdsResource::decode(const std::string& serial)
{
    std::vector<std::string> parts = splitString(serial, RESOURCE_PART_SEP, true);
    size_t size = parts.size();
    if (size < 2 || size > 4)
        throw_std_runtime_error("Could not parse resources");

    int handlerType = std::stoi(parts[0]);

    std::map<std::string, std::string> attr;
    dictDecode(parts[1], &attr);

    std::map<std::string, std::string> par;
    if (size >= 3)
        dictDecode(parts[2], &par);

    std::map<std::string, std::string> opt;
    if (size >= 4)
        dictDecode(parts[3], &opt);

    return std::make_shared<CdsResource>(handlerType, attr, par, opt);
}

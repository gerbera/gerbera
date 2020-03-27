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
#include <utility>

#include "util/tools.h"

#define RESOURCE_PART_SEP '~'

CdsResource::CdsResource(int handlerType)
{
    this->handlerType = handlerType;
}
CdsResource::CdsResource(int handlerType,
    const std::map<std::string, std::string>& attributes,
    const std::map<std::string, std::string>& parameters,
    const std::map<std::string, std::string>& options)
{
    this->handlerType = handlerType;
    this->attributes = attributes;
    this->parameters = parameters;
    this->options = options;
}

void CdsResource::addAttribute(const std::string& name, std::string value)
{
    attributes[name] = std::move(value);
}

void CdsResource::removeAttribute(const std::string& name)
{
    attributes.erase(name);
}

void CdsResource::mergeAttributes(const std::map<std::string, std::string>& additional)
{
    for (const auto& it : additional) {
        attributes[it.first] = it.second;
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

std::string CdsResource::getAttribute(const std::string& name) const
{
    return getValueOrDefault(attributes, name);
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
    buf << dict_encode(attributes);
    buf << RESOURCE_PART_SEP;
    buf << dict_encode(parameters);
    buf << RESOURCE_PART_SEP;
    buf << dict_encode(options);
    return buf.str();
}

std::shared_ptr<CdsResource> CdsResource::decode(const std::string& serial)
{
    std::vector<std::string> parts = split_string(serial, RESOURCE_PART_SEP, true);
    int size = parts.size();
    if (size < 2 || size > 4)
        throw_std_runtime_error("Could not parse resources");

    int handlerType = std::stoi(parts[0]);

    std::map<std::string, std::string> attr;
    dict_decode(parts[1], &attr);

    std::map<std::string, std::string> par;
    if (size >= 3)
        dict_decode(parts[2], &par);

    std::map<std::string, std::string> opt;
    if (size >= 4)
        dict_decode(parts[3], &opt);

    auto resource = std::make_shared<CdsResource>(handlerType, attr, par, opt);
    return resource;
}

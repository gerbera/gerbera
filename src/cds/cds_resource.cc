/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_resource.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

#define GRB_LOG_FAC GrbLogFacility::content
#include "cds_resource.h" // API

#include "exceptions.h"
#include "metadata/resolution.h"
#include "util/grb_time.h"
#include "util/logger.h"
#include "util/tools.h"
#include "util/url_utils.h"

#define RESOURCE_PART_SEP '~'

CdsResource::CdsResource(
    ContentHandler handlerType,
    ResourcePurpose purpose,
    std::string_view options,
    std::string_view parameters)
    : purpose(purpose)
    , handlerType(handlerType)
{
    this->options = URLUtils::dictDecode(options);
    this->parameters = URLUtils::dictDecode(parameters);
}

CdsResource::CdsResource(
    ContentHandler handlerType, ResourcePurpose purpose,
    std::map<ResourceAttribute, std::string> attributes,
    std::map<std::string, std::string> parameters,
    std::map<std::string, std::string> options)
    : purpose(purpose)
    , handlerType(handlerType)
    , attributes(std::move(attributes))
    , parameters(std::move(parameters))
    , options(std::move(options))
{
}

void CdsResource::addAttribute(ResourceAttribute res, std::string value)
{
    attributes[res] = std::move(value);
}

void CdsResource::mergeAttributes(const std::map<ResourceAttribute, std::string>& additional)
{
    for (auto&& [key, val] : additional) {
        attributes[key] = val;
    }
}

void CdsResource::addParameter(std::string name, std::string value)
{
    parameters.insert_or_assign(std::move(name), std::move(value));
}

void CdsResource::addOption(std::string name, std::string value)
{
    options.insert_or_assign(std::move(name), std::move(value));
}

// deprecated
const std::map<ResourceAttribute, std::string>& CdsResource::getAttributes() const
{
    return attributes;
}

const std::map<std::string, std::string>& CdsResource::getParameters() const
{
    return parameters;
}

const std::map<std::string, std::string>& CdsResource::getOptions() const
{
    return options;
}

std::string CdsResource::getAttribute(ResourceAttribute attr) const
{
    return getValueOrDefault(attributes, attr, { "" });
}

struct ProfMapping {
    std::string_view val;
    unsigned int mx;
    unsigned int my;
};

static const std::vector<struct ProfMapping> resSteps {
    { RESOURCE_IMAGE_STEP_ICO, 48, 48 },
    { RESOURCE_IMAGE_STEP_LICO, 120, 120 },
    { RESOURCE_IMAGE_STEP_TN, 160, 160 },
    { RESOURCE_IMAGE_STEP_SD, 640, 480 },
    { RESOURCE_IMAGE_STEP_HD, 1920, 1920 },
    { RESOURCE_IMAGE_STEP_UHD, 4096, 4096 },
};
static const std::vector<std::string_view> sizeUnits { "kB", "MB", "GB", "TB" };
static const std::vector<std::string_view> freqUnits { "kHz", "MHz", "GHz" };
static const std::vector<std::string_view> orientation { "Top-left", "Top-right", "Bottom-right", "Bottom-left", "Left-top", "Right-top", "Right-bottom", "Left-bottom" };
std::string CdsResource::formatSizeValue(double value)
{
    auto result = fmt::format("{} B", value);
    for (auto&& unit : sizeUnits) {
        value /= 1024;
        if (value < 1)
            return result;
        result = fmt::format("{:3.2f} {}", value, unit);
    }
    return result;
}

std::string CdsResource::getAttributeValue(ResourceAttribute attr) const
{
    auto result = getValueOrDefault(attributes, attr, { "" });
    if (result.empty())
        return result;
    switch (attr) {
    case ResourceAttribute::DURATION: {
        auto msecs = HMSFToMilliseconds(result);
        result = millisecondsToString(msecs);
        break;
    }
    case ResourceAttribute::BITRATE:
        // UPNP is silly and Bitrate is actually Bytes/Sec
        result = fmt::format("{} Kbps", stoulString(result) * 8 / 1000);
        break;
    case ResourceAttribute::SAMPLEFREQUENCY: {
        double size = stoulString(result);
        result = fmt::format("{} Hz", size);
        for (auto&& unit : freqUnits) {
            size /= 1000;
            if (size < 1)
                return result;
            result = fmt::format("{:3.1f} {}", size, unit);
        }
        break;
    }
    case ResourceAttribute::SIZE: {
        result = formatSizeValue(stoulString(result));
        break;
    }
    case ResourceAttribute::RESOLUTION: {
        try {
            auto res = Resolution(result);
            for (auto&& [val, mx, my] : resSteps) {
                if (res.x() <= mx && res.y() <= my) {
                    return val.data();
                }
            }
            return RESOURCE_IMAGE_STEP_XHD; // Image is larger with no step defined
        } catch (const std::runtime_error& e) {
            log_warning("Resource attribute for resolution {} is invalid", result);
            return fmt::format("???", result);
        }
        break;
    }
    case ResourceAttribute::ORIENTATION: {
        try {
            std::size_t value = stoiString(result);
            return (value > 0 && value <= orientation.size()) ? orientation.at(value - 1).data() : result;
        } catch (const std::runtime_error& e) {
            log_warning("Resource attribute for orientation {} is invalid", result);
            return result;
        }
        break;
    }
    default:
        return result;
    }
    return result;
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
        && purpose == other->purpose
        && std::equal(attributes.begin(), attributes.end(), other->attributes.begin())
        && std::equal(parameters.begin(), parameters.end(), other->parameters.begin())
        && std::equal(options.begin(), options.end(), other->options.begin()));
}

std::shared_ptr<CdsResource> CdsResource::clone()
{
    return std::make_shared<CdsResource>(handlerType, purpose, attributes, parameters, options);
}

std::shared_ptr<CdsResource> CdsResource::decode(const std::string& serial)
{
    std::vector<std::string> parts = splitString(serial, RESOURCE_PART_SEP, true);
    auto size = parts.size();
    if (size < 2 || size > 4)
        throw_std_runtime_error("Could not parse resources");

    auto handlerType = EnumMapper::remapContentHandler(std::stoi(parts[0]));

    std::map<std::string, std::string> attr;
    std::map<std::string, std::string> par;
    std::map<std::string, std::string> opt;

    attr = URLUtils::dictDecode(parts[1]);
    std::map<ResourceAttribute, std::string> attrParsed;
    for (auto [k, v] : attr) {
        attrParsed[EnumMapper::mapAttributeName(k)] = v;
    }

    if (size >= 3)
        par = URLUtils::dictDecode(parts[2]);

    if (size >= 4)
        opt = URLUtils::dictDecode(parts[3]);

    return std::make_shared<CdsResource>(
        handlerType,
        handlerType == ContentHandler::DEFAULT ? ResourcePurpose::Content : ResourcePurpose::Thumbnail,
        std::move(attrParsed),
        std::move(par),
        std::move(opt));
}

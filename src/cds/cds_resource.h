/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_resource.h - this file is part of MediaTomb.

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

/// \file cds_resource.h

#ifndef __CDS_RESOURCE_H__
#define __CDS_RESOURCE_H__

#include "cds_enums.h"
#include "util/enum_iterator.h"

#include <map>
#include <memory>

#define RESOURCE_OPTION_FOURCC "4cc"

#define RESOURCE_IMAGE_STEP_ICO "ICO"
#define RESOURCE_IMAGE_STEP_LICO "LICO"
#define RESOURCE_IMAGE_STEP_TN "TN"
#define RESOURCE_IMAGE_STEP_SD "SD"
#define RESOURCE_IMAGE_STEP_HD "HD"
#define RESOURCE_IMAGE_STEP_UHD "UHD"
#define RESOURCE_IMAGE_STEP_XHD "XHD"
#define RESOURCE_IMAGE_STEP_DEF "*"

class CdsResource {
public:
    /// \brief creates a new resource object.
    ///
    /// The CdsResource object represents a \<res\> tag in the DIDL-Lite XML.
    ///
    /// \param handlerType of the associated handler
    /// \param purpose of the associated handler
    /// \param options options of resource
    /// \param parameters parameters of resource
    explicit CdsResource(
        ContentHandler handlerType,
        ResourcePurpose purpose,
        std::string_view options = {},
        std::string_view parameters = {});
    CdsResource(
        ContentHandler handlerType, ResourcePurpose purpose,
        std::map<ResourceAttribute, std::string> attributes,
        std::map<std::string, std::string> parameters,
        std::map<std::string, std::string> options);

    int getResId() const { return resId; }
    void setResId(int rId) { resId = rId; }
    /// \brief Adds a resource attribute.
    ///
    /// This maps to an attribute of the \<res\> tag in the DIDL-Lite XML.
    ///
    /// \param res attribute name
    /// \param value attribute value
    void addAttribute(ResourceAttribute res, std::string value);

    /// \brief Merge existing attributes with new ones
    void mergeAttributes(const std::map<ResourceAttribute, std::string>& additional);

    /// \brief Adds a parameter (will be appended to the URL)
    ///
    /// The parameters will be appended to the object URL in the DIDL-Lite XML.
    /// This is useful for cases, where you need to identify specific options,
    /// i.e. something that is only relevant to a particular metadata handler
    /// and so on. The parameters will be automatically URL encoded.
    ///
    /// \param name parameter name
    /// \param value parameter value
    void addParameter(std::string name, std::string value);

    /// \brief Add an option to the resource.
    ///
    /// The options are internal, they do not appear in the URL or in the
    /// XML but can be used for any purpose.
    void addOption(std::string name, std::string value);

    /// \brief Type of resource handler
    ContentHandler getHandlerType() const { return handlerType; }

    const std::map<ResourceAttribute, std::string>& getAttributes() const;
    const std::map<std::string, std::string>& getParameters() const;
    const std::map<std::string, std::string>& getOptions() const;
    std::string getAttribute(ResourceAttribute attr) const;
    std::string getAttributeValue(ResourceAttribute attr) const;
    std::string getParameter(const std::string& name) const;
    std::string getOption(const std::string& name) const;
    ResourcePurpose getPurpose() const { return purpose; }
    void setPurpose(ResourcePurpose purpose) { this->purpose = purpose; }

    bool equals(const std::shared_ptr<CdsResource>& other) const;
    std::shared_ptr<CdsResource> clone();

    static std::shared_ptr<CdsResource> decode(const std::string& serial);
    static std::string formatSizeValue(double value);

protected:
    ResourcePurpose purpose { ResourcePurpose::Content };
    ContentHandler handlerType;
    int resId { -1 };
    std::map<ResourceAttribute, std::string> attributes;
    std::map<std::string, std::string> parameters;
    std::map<std::string, std::string> options;
};

using ResourceAttributeIterator = EnumIterator<ResourceAttribute, ResourceAttribute::SIZE, ResourceAttribute::MAX>;

#endif // __CDS_RESOURCE_H__

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource.h - this file is part of MediaTomb.
    
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

/// \file cds_resource.h

#ifndef __CDS_RESOURCE_H__
#define __CDS_RESOURCE_H__

#include <map>
#include <memory>
#include <string>

#include "common.h"
#include "metadata/metadata_handler.h"

/// \brief name for external urls that can appear in object resources (i.e.
/// a YouTube thumbnail)
#define RESOURCE_OPTION_URL "url"

/// \brief if set, overrides the OBJECT_FLAG_PROXY_URL setting for the given
/// resource
#define RESOURCE_OPTION_PROXY_URL "prx"

#define RESOURCE_OPTION_FOURCC "4cc"

class CdsResource {
protected:
    int handlerType;
    std::map<std::string, std::string> attributes;
    std::map<std::string, std::string> parameters;
    std::map<std::string, std::string> options;

public:
    /// \brief creates a new resource object.
    ///
    /// The CdsResource object represents a <res> tag in the DIDL-Lite XML.
    ///
    /// \param handler_type id of the associated handler
    explicit CdsResource(int handlerType);
    CdsResource(int handlerType,
        std::map<std::string, std::string> attributes,
        std::map<std::string, std::string> parameters,
        std::map<std::string, std::string> options);

    /// \brief Adds a resource attribute.
    ///
    /// This maps to an attribute of the <res> tag in the DIDL-Lite XML.
    ///
    /// \param name attribute name
    /// \param value attribute value
    void addAttribute(resource_attributes_t res, std::string value);

    /// \brief Removes a resource attribute.
    ///
    /// \param name attribute name
    void removeAttribute(resource_attributes_t res);

    /// \brief Merge existing attributes with new ones
    void mergeAttributes(const std::map<std::string, std::string>& additional);

    /// \brief Adds a parameter (will be appended to the URL)
    ///
    /// The parameters will be appended to the object URL in the DIDL-Lite XML.
    /// This is useful for cases, where you need to identify specific options,
    /// i.e. something that is only relevant to a particular metadata handler
    /// and so on. The parameters will be automatically URL encoded.
    ///
    /// \param name parameter name
    /// \param value parameter value
    void addParameter(const std::string& name, std::string value);

    /// \brief Add an option to the resource.
    ///
    /// The options are internal, they do not appear in the URL or in the
    /// XML but can be used for any purpose.
    void addOption(const std::string& name, std::string value);

    int getHandlerType() const;
    std::map<std::string, std::string> getAttributes() const;
    std::map<std::string, std::string> getParameters() const;
    std::map<std::string, std::string> getOptions() const;
    std::string getAttribute(resource_attributes_t res) const;
    std::string getParameter(const std::string& name) const;
    std::string getOption(const std::string& name) const;
    bool isMetaResource(const char* rct) const
    {
        return ((handlerType == CH_ID3 || handlerType == CH_MP4 || handlerType == CH_FLAC || handlerType == CH_FANART || handlerType == CH_CONTAINERART) && getParameter(RESOURCE_CONTENT_TYPE) == rct) || (handlerType == CH_EXTURL && getOption(RESOURCE_CONTENT_TYPE) == rct);
    }

    bool equals(const std::shared_ptr<CdsResource>& other) const;
    std::shared_ptr<CdsResource> clone();

    /// \brief urlencode into string
    std::string encode();
    static std::shared_ptr<CdsResource> decode(const std::string& serial);
};

#endif // __CDS_RESOURCE_H__

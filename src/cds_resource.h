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

#include "common.h"
#include "dictionary.h"

/// \brief name for external urls that can appear in object resources (i.e.
/// a YouTube thumbnail)
#define RESOURCE_OPTION_URL "url"

/// \brief if set, overrides the OBJECT_FLAG_PROXY_URL setting for the given
/// resource
#define RESOURCE_OPTION_PROXY_URL "prx"

#define RESOURCE_OPTION_FOURCC "4cc"

class CdsResource : public zmm::Object {
protected:
    int handlerType;
    zmm::Ref<Dictionary> attributes;
    zmm::Ref<Dictionary> parameters;
    zmm::Ref<Dictionary> options;

public:
    /// \brief creates a new resource object.
    ///
    /// The CdsResource object represents a <res> tag in the DIDL-Lite XML.
    ///
    /// \param handler_type id of the associated handler
    CdsResource(int handlerType);
    CdsResource(int handlerType,
        zmm::Ref<Dictionary> attributes,
        zmm::Ref<Dictionary> parameters,
        zmm::Ref<Dictionary> options);

    /// \brief Adds a resource attribute.
    ///
    /// This maps to an attribute of the <res> tag in the DIDL-Lite XML.
    ///
    /// \param name attribute name
    /// \param value attribute value
    void addAttribute(zmm::String name, zmm::String value);

    /// \brief Removes a resource attribute.
    ///
    /// \param name attribute name
    void removeAttribute(zmm::String name);

    /// \brief Merge existing attributes with new ones
    void mergeAttributes(zmm::Ref<Dictionary> additional);

    /// \brief Adds a parameter (will be appended to the URL)
    ///
    /// The parameters will be appended to the object URL in the DIDL-Lite XML.
    /// This is useful for cases, where you need to identify specific options,
    /// i.e. something that is only relevant to a particular metadata handler
    /// and so on. The parameters will be automatically URL encoded.
    ///
    /// \param name parameter name
    /// \param value parameter value
    void addParameter(zmm::String name, zmm::String value);

    /// \brief Add an option to the resource.
    ///
    /// The options are internal, they do not appear in the URL or in the
    /// XML but can be used for any purpose.
    void addOption(zmm::String name, zmm::String value);

    // urlencode into string
    int getHandlerType();
    zmm::Ref<Dictionary> getAttributes();
    zmm::Ref<Dictionary> getParameters();
    zmm::Ref<Dictionary> getOptions();
    zmm::String getAttribute(zmm::String name);
    zmm::String getParameter(zmm::String name);
    zmm::String getOption(zmm::String name);

    bool equals(zmm::Ref<CdsResource> other);
    zmm::Ref<CdsResource> clone();

    zmm::String encode();
    static zmm::Ref<CdsResource> decode(zmm::String serial);

    /// \brief Frees unnecessary memory
    void optimize();
};

#endif // __CDS_RESOURCE_H__

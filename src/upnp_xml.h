/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_xml.h - this file is part of MediaTomb.
    
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

/// \file upnp_xml.h
/// \brief Provides various XML related functions, basically a toolkit.
#ifndef __UPNP_XML_H__
#define __UPNP_XML_H__

#include <memory>
#include <pugixml.hpp>

#include "cds_objects.h"
#include "common.h"
#include "context.h"
#include "util/upnp_quirks.h"

class UpnpXMLBuilder {
public:
    explicit UpnpXMLBuilder(const std::shared_ptr<Context>& context,
        std::string virtualUrl, std::string presentationURL);

    /// \brief Renders XML for the action response header.
    /// \param actionName Name of the action.
    /// \param serviceType Type of service.
    /// \return pugi::xml_document representing the newly created XML.
    ///
    /// Basically it renders an XML that looks like the following:
    /// <u:actionNameResponse xmlns:u="serviceType"/>
    /// Further response information (various parameters, DIDL-Lite or
    /// whatever can then be adapted to it.
    static std::unique_ptr<pugi::xml_document> createResponse(const std::string& actionName, const std::string& serviceType);

    /// \brief Renders the DIDL-Lite representation of an object in the content directory.
    /// \param obj Object to be rendered as XML.
    ///
    /// This function looks at the object, and renders the DIDL-Lite representation of it -
    /// either a container or an item
    void renderObject(const std::shared_ptr<CdsObject>& obj, size_t stringLimit, pugi::xml_node* parent, const std::shared_ptr<Quirks>& quirks = nullptr);

    /// \brief Renders XML for the event property set.
    /// \return pugi::xml_document representing the newly created XML.
    static std::unique_ptr<pugi::xml_document> createEventPropertySet();

    /// \brief Renders the device description XML.
    /// \return pugi::xml_document representing the newly created device description.
    ///
    /// Some elements are statically defined in common.h, others are loaded
    /// from the config with the help of the ConfigManager.
    std::unique_ptr<pugi::xml_document> renderDeviceDescription();

    /// \brief Renders a resource tag (part of DIDL-Lite XML)
    /// \param URL download location of the item (will be child element of the <res> tag)
    /// \param attributes Dictionary containing the <res> tag attributes (like resolution, etc.)
    static void renderResource(const std::string& URL, const std::map<std::string, std::string>& attributes, pugi::xml_node* parent);

    static bool renderContainerImage(const std::string& virtualURL, const std::shared_ptr<CdsContainer>& cont, std::string& url);
    static bool renderItemImage(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, std::string& url);
    static bool renderSubtitle(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, std::string& url);

    void addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node* parent);

    // FIXME: This needs to go, once we sort a nicer way for the webui code to access this
    static std::string getFirstResourcePath(const std::shared_ptr<CdsItem>& item);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    const std::string virtualURL;
    const std::string presentationURL;

    // Holds a part of path and bool which says if we need to append the resource
    // TODO: Remove this and use centralised routing instead of building URLs all over the place
    class PathBase {
    public:
        std::string pathBase;
        bool addResID;
    };

    static std::unique_ptr<PathBase> getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal = false);
    static std::string renderExtension(const std::string& contentType, const std::string& location);
    std::string getArtworkUrl(const std::shared_ptr<CdsItem>& item) const;
    static void addField(pugi::xml_node& entry, const std::string& key, const std::string& val);
    static metadata_fields_t remapMetaDataField(const std::string& fieldName);
};
#endif // __UPNP_XML_H__

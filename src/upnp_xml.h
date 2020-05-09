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

// forward declaration
class Config;
class Storage;

class UpnpXMLBuilder {
public:
    explicit UpnpXMLBuilder(std::shared_ptr<Config> config,
        std::shared_ptr<Storage> storage,
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
    /// \param renderActions If true, also render special elements of an active item.
    ///
    /// This function looks at the object, and renders the DIDL-Lite representation of it -
    /// either a container or an item. The renderActions parameter tells us whether to also
    /// show the special fields of an active item in the XML. This is currently used when
    /// providing the XML representation of an active item to a trigger/toggle script.
    void renderObject(const std::shared_ptr<CdsObject>& obj, bool renderActions, size_t stringLimit, pugi::xml_node* parent);

    /// \todo change the text string to element, parsing should be done outside
    static void updateObject(const std::shared_ptr<CdsObject>& obj, const std::string& text);

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

    /// \brief Renders a subtitle resource tag
    /// \param URL download location of the video item
    void renderCaptionInfo(const std::string& URL, pugi::xml_node* parent) const;

    void addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node* parent);

    // FIXME: This needs to go, once we sort a nicer way for the webui code to access this
    static std::string getFirstResourcePath(const std::shared_ptr<CdsItem>& item);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Storage> storage;

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
};
#endif // __UPNP_XML_H__

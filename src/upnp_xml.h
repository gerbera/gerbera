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

#include <deque>
#include <memory>
#include <pugixml.hpp>
#include <vector>

#include "cds_objects.h"
#include "common.h"
#include "config/config.h"
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
    void renderObject(const std::shared_ptr<CdsObject>& obj, std::size_t stringLimit, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks = nullptr);

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
    static void renderResource(const std::string& url, const std::map<std::string, std::string>& attributes, pugi::xml_node& parent);

    std::pair<std::string, bool> renderContainerImage(const std::string& virtualURL, const std::shared_ptr<CdsContainer>& cont);
    std::pair<std::string, bool> renderItemImage(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item);
    static std::pair<std::string, bool> renderSubtitle(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item);
    static std::string renderOneResource(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, const std::shared_ptr<CdsResource>& res);

    void addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks);

    /// \brief build path for first resource from item
    /// depending on the item type it returns the url to the media
    static std::string getFirstResourcePath(const std::shared_ptr<CdsItem>& item);

    /// \brief convert xml tree to string
    static std::string printXml(const pugi::xml_node& entry, const char* indent = PUGIXML_TEXT("\t"), int flags = pugi::format_default);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    std::vector<int> orderedHandler;

    const std::string virtualURL;
    const std::string presentationURL;
    std::string entrySeparator;
    bool multiValue {};
    std::map<std::string, std::string> ctMappings;

    /// \brief Holds a part of path and bool which says if we need to append the resource
    struct PathBase {
        PathBase(std::string pathBase, bool addResID)
            : pathBase(std::move(pathBase))
            , addResID(addResID)
        {
        }
        std::string pathBase;
        bool addResID;
    };
    std::deque<std::shared_ptr<CdsResource>> getOrderedResources(const std::shared_ptr<CdsObject>& object);
    std::pair<bool, int> insertTempTranscodingResource(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Quirks>& quirks, std::deque<std::shared_ptr<CdsResource>>& orderedResources, bool skipURL);

    static std::unique_ptr<PathBase> getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal = false);
    static std::string renderExtension(const std::string& contentType, const fs::path& location);
    static void addField(pugi::xml_node& entry, const std::string& key, const std::string& val);
    void addPropertyList(pugi::xml_node& result, const std::vector<std::pair<std::string, std::string>>& meta, const std::map<std::string, std::string>& auxData, config_option_t itemProps, config_option_t nsProp);
    std::string dlnaProfileString(const std::shared_ptr<CdsResource>& res, const std::string& contentType);
};
#endif // __UPNP_XML_H__

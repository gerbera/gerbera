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

class Quirks;

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
    std::unique_ptr<pugi::xml_document> createResponse(const std::string& actionName, const std::string& serviceType) const;

    /// \brief Renders the DIDL-Lite representation of an object in the content directory.
    /// \param obj Object to be rendered as XML.
    ///
    /// This function looks at the object, and renders the DIDL-Lite representation of it -
    /// either a container or an item
    void renderObject(const std::shared_ptr<CdsObject>& obj, std::size_t stringLimit, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks = nullptr) const;

    /// \brief Renders XML for the event property set.
    /// \return pugi::xml_document representing the newly created XML.
    std::unique_ptr<pugi::xml_document> createEventPropertySet() const;

    /// \brief Renders the device description XML.
    /// \return pugi::xml_document representing the newly created device description.
    ///
    /// Some elements are statically defined in common.h, others are loaded
    /// from the config with the help of the ConfigManager.
    std::unique_ptr<pugi::xml_document> renderDeviceDescription() const;

    /// \brief Renders a resource tag (part of DIDL-Lite XML)
    /// \param URL download location of the item (will be child element of the <res> tag)
    /// \param resource The CDSResource itself
    /// \param parent Parent node to render the result into
    /// \param clientSpecificAttrs A map containing extra client specific res attributes (like resolution, etc.)
    void renderResource(const std::string& url, const CdsResource& resource, pugi::xml_node& parent, const std::map<std::string, std::string>& clientSpecificAttrs) const;

    std::pair<std::string, bool> renderContainerImage(const std::shared_ptr<CdsContainer>& cont) const;
    std::pair<std::string, bool> renderItemImage(const std::shared_ptr<CdsItem>& item) const;
    std::pair<std::string, bool> renderSubtitle(const std::shared_ptr<CdsItem>& item, const Quirks* quirks) const;
    std::string renderOneResource(const std::shared_ptr<CdsItem>& item, const std::shared_ptr<CdsResource>& res) const;

    void addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks) const;

    /// \brief build path for first resource from item
    /// depending on the item type it returns the url to the media
    std::string getFirstResourcePath(const std::shared_ptr<CdsItem>& item) const;

    /// \brief convert xml tree to string
    static std::string printXml(const pugi::xml_node& entry, const char* indent = PUGIXML_TEXT("\t"), int flags = pugi::format_default);

    std::string getDLNAContentHeader(const std::string& contentType, const std::shared_ptr<CdsResource>& res) const;
    std::string getDLNATransferHeader(const std::string& mimeType) const;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    std::vector<ContentHandler> orderedHandler;

    const std::string virtualURL;
    const std::string presentationURL;
    std::string entrySeparator;
    bool multiValue {};
    std::map<std::string, std::string> ctMappings;
    std::vector<std::vector<std::pair<std::string, std::string>>> profMappings;
    std::map<std::string, std::string> transferMappings;

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
    std::deque<std::shared_ptr<CdsResource>> getOrderedResources(const std::shared_ptr<CdsObject>& object) const;
    std::pair<bool, int> insertTempTranscodingResource(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Quirks>& quirks, std::deque<std::shared_ptr<CdsResource>>& orderedResources, bool skipURL) const;

    std::unique_ptr<PathBase> getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal = false) const;
    std::string renderExtension(const std::string& contentType, const fs::path& location, const Quirks* quirks) const;
    void addField(pugi::xml_node& entry, const std::string& key, const std::string& val) const;
    void addPropertyList(pugi::xml_node& result, const std::vector<std::pair<std::string, std::string>>& meta, const std::map<std::string, std::string>& auxData, config_option_t itemProps, config_option_t nsProp) const;
    std::string findDlnaProfile(const std::shared_ptr<CdsResource>& res, const std::string& contentType) const;
    std::string dlnaProfileString(const std::shared_ptr<CdsResource>& res, const std::string& contentType, bool formatted = true) const;
};
#endif // __UPNP_XML_H__

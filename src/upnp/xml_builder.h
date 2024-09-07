/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp/xml_builder.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file upnp/xml_builder.h
/// \brief Provides various XML related functions, basically a toolkit.
#ifndef __UPNP_XML_H__
#define __UPNP_XML_H__

#include "util/grb_fs.h"

#include <deque>
#include <map>
#include <memory>
#include <pugixml.hpp>
#include <vector>

#define CONTENT_MEDIA_HANDLER "media"
#define CONTENT_ONLINE_HANDLER "online"
#define CONTENT_UI_HANDLER "interface"

class CdsContainer;
class CdsItem;
class CdsObject;
class CdsResource;
class Config;
class Context;
class Database;
enum class ContentHandler;
enum class ConfigVal;
class Quirks;

class UpnpXMLBuilder {
public:
    explicit UpnpXMLBuilder(const std::shared_ptr<Context>& context, std::string virtualUrl);

    /// \brief Renders XML for the action response header.
    /// \param actionName Name of the action.
    /// \param serviceType Type of service.
    /// \return pugi::xml_document representing the newly created XML.
    ///
    /// Basically it renders an XML that looks like the following:
    /// \<u:actionNameResponse xmlns:u="serviceType"/\>
    /// Further response information (various parameters, DIDL-Lite or
    /// whatever can then be adapted to it.
    std::unique_ptr<pugi::xml_document> createResponse(const std::string& actionName, const std::string& serviceType) const;

    /// \brief Renders the DIDL-Lite representation of an object in the content directory.
    /// \param obj Object to be rendered as XML.
    /// \param stringLimit maximum length of string
    /// \param parent parent xml node
    /// \param quirks inject special handling for clients
    ///
    /// This function looks at the object, and renders the DIDL-Lite representation of it -
    /// either a container or an item
    void renderObject(const std::shared_ptr<CdsObject>& obj, std::size_t stringLimit, pugi::xml_node& parent, const std::shared_ptr<Quirks>& quirks = nullptr) const;

    /// \brief Renders XML for the event property set.
    /// \return pugi::xml_document representing the newly created XML.
    std::unique_ptr<pugi::xml_document> createEventPropertySet() const;

    /// \brief Renders a resource tag (part of DIDL-Lite XML)
    /// \param obj Object containing this resource
    /// \param resource The CDSResource itself
    /// \param parent Parent node to render the result into
    /// \param clientSpecificAttrs A map containing extra client specific res attributes (like resolution, etc.)
    /// \param clientGroup The clients group for play tracking
    /// \param mimeMappings mappings of mime-types
    void renderResource(const CdsObject& obj,
        const CdsResource& resource, pugi::xml_node& parent,
        const std::map<std::string, std::string>& clientSpecificAttrs,
        const std::string& clientGroup,
        const std::map<std::string, std::string>& mimeMappings) const;

    std::optional<std::string> renderContainerImageURL(const std::shared_ptr<CdsContainer>& cont) const;
    std::optional<std::string> renderItemImageURL(const std::shared_ptr<CdsItem>& item) const;
    std::optional<std::string> renderSubtitleURL(const std::shared_ptr<CdsItem>& item, const std::map<std::string, std::string>& mimeMappings) const;

    std::string renderResourceURL(const CdsObject& item, const CdsResource& res, const std::map<std::string, std::string>& mimeMappings, const std::string& clientGroup = "") const;

    void addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node& parent, const std::shared_ptr<Quirks>& quirks) const;

    /// \brief build path for first resource from item
    /// depending on the item type it returns the url to the media
    std::string getFirstResourcePath(const std::shared_ptr<CdsItem>& item) const;

    /// \brief convert xml tree to string
    static std::string printXml(const pugi::xml_node& entry, const char* indent = PUGIXML_TEXT("\t"), int flags = pugi::format_default);

    std::string getDLNAContentHeader(const std::string& contentType, const std::shared_ptr<CdsResource>& res, const std::shared_ptr<Quirks>& quirks) const;
    std::string getDLNATransferHeader(const std::string& mimeType) const;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    std::vector<ContentHandler> orderedHandler;

    std::string virtualURL;
    std::string entrySeparator;
    bool multiValue {};
    std::map<std::string, std::string> ctMappings;
    std::vector<std::vector<std::pair<std::string, std::string>>> profMappings;
    std::map<std::string, std::string> transferMappings;

    std::deque<std::shared_ptr<CdsResource>> getOrderedResources(const CdsObject& object) const;
    std::pair<bool, int> insertTempTranscodingResource(const std::shared_ptr<CdsItem>& item, const std::shared_ptr<Quirks>& quirks, std::deque<std::shared_ptr<CdsResource>>& orderedResources, bool skipURL) const;

    std::string renderExtension(const std::string& contentType, const fs::path& location, const std::string& language) const;
    void addField(pugi::xml_node& entry, const std::string& key, const std::string& val) const;
    void addPropertyList(bool strictXml,
        std::size_t stringLimit,
        pugi::xml_node& result,
        const std::vector<std::pair<std::string, std::string>>& meta,
        const std::map<std::string, std::string>& auxData,
        ConfigVal itemProps,
        ConfigVal nsProp) const;
    std::string findDlnaProfile(const CdsResource& res, const std::string& contentType, const std::shared_ptr<Quirks>& quirks) const;
    std::string dlnaProfileString(const CdsResource& res, const std::string& contentType, const std::shared_ptr<Quirks>& quirks, bool formatted = true) const;

    std::string buildProtocolInfo(CdsResource& res, const std::map<std::string, std::string>& mimeMappings, const std::shared_ptr<Quirks>& quirks) const;
    std::string getMimeType(const CdsResource& resource, const std::map<std::string, std::string>& mimeMappings) const;
};
#endif // __UPNP_XML_H__

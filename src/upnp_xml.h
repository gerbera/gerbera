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

#include "cds_objects.h"
#include "common.h"
#include "mxml/mxml.h"

class UpnpXMLBuilder {
public:
    explicit UpnpXMLBuilder(zmm::String virtualUrl);

    /// \brief Renders XML for the action response header.
    /// \param actionName Name of the action.
    /// \param serviceType Type of service.
    /// \return mxml::Element representing the newly created XML.
    ///
    /// Basically it renders an XML that looks like the following:
    /// <u:actionNameResponse xmlns:u="serviceType"/>
    /// Further response information (various parameters, DIDL-Lite or
    /// whatever can then be adapted to it.
    zmm::Ref<mxml::Element> createResponse(zmm::String actionName, zmm::String serviceType);

    /// \brief Renders the DIDL-Lite representation of an object in the content directory.
    /// \param obj Object to be rendered as XML.
    /// \param renderActions If true, also render special elements of an active item.
    /// \return mxml::Element representing the newly created XML.
    ///
    /// This function looks at the object, and renders the DIDL-Lite representation of it -
    /// either a container or an item. The renderActions parameter tells us whether to also
    /// show the special fields of an active item in the XML. This is currently used when
    /// providing the XML representation of an active item to a trigger/toggle script.
    zmm::Ref<mxml::Element> renderObject(zmm::Ref<CdsObject> obj, bool renderActions = false, int stringLimit = -1);

    /// \todo change the text string to element, parsing should be done outside
    void updateObject(zmm::Ref<CdsObject> obj, zmm::String text);

    /// \brief Renders XML for the event property set.
    /// \return mxml::Element representing the newly created XML.
    zmm::Ref<mxml::Element> createEventPropertySet();

    /// \brief Renders the device description XML.
    /// \return mxml::Element representing the newly created device description.
    ///
    /// Some elements are statically defined in common.h, others are loaded
    /// from the config with the help of the ConfigManager.
    zmm::Ref<mxml::Element> renderDeviceDescription(zmm::String presentationUTL = nullptr);

    /// \brief Renders a resource tag (part of DIDL-Lite XML)
    /// \param URL download location of the item (will be child element of the <res> tag)
    /// \param attributes Dictionary containing the <res> tag attributes (like resolution, etc.)
    zmm::Ref<mxml::Element> renderResource(zmm::String URL, zmm::Ref<Dictionary> attributes);

    /// \brief Renders a subtitle resource tag (Samsung proprietary extension)
    /// \param URL download location of the video item
    zmm::Ref<mxml::Element> renderCaptionInfo(zmm::String URL);

    zmm::Ref<mxml::Element> renderCreator(zmm::String creator);

    zmm::Ref<mxml::Element> renderAlbumArtURI(zmm::String uri);

    zmm::Ref<mxml::Element> renderComposer(zmm::String composer);

    zmm::Ref<mxml::Element> renderConductor(zmm::String conductor);

    zmm::Ref<mxml::Element> renderOrchestra(zmm::String orchestra);

    zmm::Ref<mxml::Element> renderAlbumDate(zmm::String date);

    void addResources(zmm::Ref<CdsItem> item, zmm::Ref<mxml::Element> element);

    // FIXME: This needs to go, once we sort a nicer way for the webui code to access this
    static zmm::String getFirstResourcePath(zmm::Ref<CdsItem> item);

protected:
    zmm::String virtualUrl;

    // Holds a part of path and bool which says if we need to append the resource
    // TODO: Remove this and use centralised routing instead of building URLs all over the place
    class PathBase : public zmm::Object {
    public:
        zmm::String pathBase;
        bool addResID;
    };

    static zmm::Ref<PathBase> getPathBase(zmm::Ref<CdsItem> item, bool forceLocal = false);
    zmm::String renderExtension(zmm::String contentType, zmm::String location);
    zmm::String getArtworkUrl(zmm::Ref<CdsItem> item);
};
#endif // __UPNP_XML_H__

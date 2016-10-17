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

#include "common.h"
#include "mxml/mxml.h"
#include "cds_objects.h"

/// \brief Renders XML for the action response header.
/// \param actionName Name of the action.
/// \param serviceType Type of service.
/// \return mxml::Element representing the newly created XML.
///
/// Basically it renders an XML that looks like the following:
/// <u:actionNameResponse xmlns:u="serviceType"/>
/// Further response information (various parameters, DIDL-Lite or
/// whatever can then be adapted to it.
zmm::Ref<mxml::Element> UpnpXML_CreateResponse(zmm::String actionName, zmm::String serviceType);

/// \brief Renders the DIDL-Lite representation of an object in the content directory.
/// \param obj Object to be rendered as XML.
/// \param renderActions If true, also render special elements of an active item.
/// \return mxml::Element representing the newly created XML.
///
/// This function looks at the object, and renders the DIDL-Lite representation of it - 
/// either a container or an item. The renderActions parameter tells us whether to also
/// show the special fields of an active item in the XML. This is currently used when
/// providing the XML representation of an active item to a trigger/toggle script.
zmm::Ref<mxml::Element> UpnpXML_DIDLRenderObject(zmm::Ref<CdsObject> obj, bool renderActions = false, int stringLimit = -1);

/// \todo change the text string to element, parsing should be done outside
void UpnpXML_DIDLUpdateObject(zmm::Ref<CdsObject> obj, zmm::String text);

/// \brief Renders XML for the event property set.
/// \return mxml::Element representing the newly created XML.
zmm::Ref<mxml::Element> UpnpXML_CreateEventPropertySet();

/// \brief Renders the device description XML.
/// \return mxml::Element representing the newly created device description.
///
/// Some elements are statically defined in common.h, others are loaded
/// from the config with the help of the ConfigManager.
zmm::Ref<mxml::Element> UpnpXML_RenderDeviceDescription(zmm::String presentationUTL = nil);

/// \brief Renders a resource tag (part of DIDL-Lite XML)
/// \param URL download location of the item (will be child element of the <res> tag)
/// \param attributes Dictionary containing the <res> tag attributes (like resolution, etc.)
zmm::Ref<mxml::Element> UpnpXML_DIDLRenderResource(zmm::String URL, zmm::Ref<Dictionary> attributes);

/// \brief Renders a subtitle resource tag (Samsung proprietary extension)
/// \param URL download location of the video item
zmm::Ref<mxml::Element> UpnpXML_DIDLRenderCaptionInfo(zmm::String URL);

zmm::Ref<mxml::Element> UpnpXML_DIDLRenderCreator(zmm::String creator);
#endif // __UPNP_XML_H__

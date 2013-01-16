/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource_manager.h - this file is part of MediaTomb.
    
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

/// \file cds_resource_manager.h
/// \brief Definition of the CdsResourceManager class.
#ifndef __CDS_RESOURCE_MANAGER_H__
#define __CDS_RESOURCE_MANAGER_H__

#include "singleton.h"
#include "mxml/mxml.h"
#include "common.h"
#include "cds_objects.h"
#include "strings.h"

/// \brief This class is responsible for handling the DIDL-Lite res tags.
class CdsResourceManager : public zmm::Object
{
public:
    /// \brief Constructor, currently empty.
    CdsResourceManager();

    /// \brief Adds a resource tag to the item.
    /// \param item Item for which the resources should be added.
    /// \param element Element in the XML to which the resource should be appended.
    ///
    /// This function figures out what resources should be added to what files.
    /// It looks at the server configuration to find out what it needs. For example,
    /// if you want to add another mime/type alias for an existing mime/type, this
    /// function would do it. Also, when transcoding will be implemented, the
    /// various transcoded streams will be identified here.
    static void addResources(zmm::Ref<CdsItem> item, zmm::Ref<mxml::Element> element);
    
    /// \brief Gets the URL of the first resource of the CfsItem.
    /// \param item Item for which the resources should be built.
    /// \return The URL
    static zmm::String getFirstResource(zmm::Ref<CdsItem> item);
    
protected:
    class UrlBase : public zmm::Object
    {
        public:
        zmm::String urlBase;
        bool addResID;
    };
    
    /// \brief Gets the baseUrl for a CdsItem.
    /// \param item Item for which the baseUrl should be built.
    ///
    /// This function gets the baseUrl for the CdsItem and sets addResID
    /// to true if the resource id needs to be added to the URL.
    static zmm::Ref<UrlBase> addResources_getUrlBase(zmm::Ref<CdsItem> item,
            bool forceLocal = false);
   
    /// \brief renders an ext=.extension string, where the extension is 
    /// determined either from content type or from the filename
    static zmm::String renderExtension(zmm::String contentType, zmm::String location);
};

#endif // __CDS_RESOURCE_MANAGER_H__

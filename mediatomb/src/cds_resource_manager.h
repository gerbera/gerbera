/*  cds_resource_manager.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file cds_resource_manager.h
/// \brief Definition of the CdsResourceManager class.
#ifndef __CDS_RESOURCE_MANAGER_H__
#define __CDS_RESOURCE_MANAGER_H__

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

    /// \brief Returns the class instance.
    ///
    /// This class can be instantiated only once, if no previous
    /// instances exist getInstance() will create a new one,
    /// else the existing instance is returned.
    static zmm::Ref<CdsResourceManager> getInstance();
    
    /// \brief Adds a resource tag to the item.
    /// \param item Item for which the resources should be added.
    /// \param element Element in the XML to which the resource should be appended.
    ///
    /// This function figures out what resources should be added to what files.
    /// It looks at the server configuration to find out what it needs. For example,
    /// if you want to add another mime/type alias for an existing mime/type, this
    /// function would do it. Also, when transcoding will be implemented, the
    /// various transcoded streams will be identified here.
    void addResources(zmm::Ref<CdsItem> item, zmm::Ref<mxml::Element> element);
    
    /// \brief Gets the URL of the first resource of the CfsItem.
    /// \param item Item for which the resources should be built.
    /// \return The URL
    zmm::String getFirstResource(zmm::Ref<CdsItem> item);
    
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
    zmm::Ref<UrlBase> addResources_getUrlBase(zmm::Ref<CdsItem> item);
    
    
};

#endif // __CDS_RESOURCE_MANAGER_H__


/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service_content_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file online_service_content_handler.h
/// \brief Definitions of the OnlineServiceContentHandler interface class.

#if defined(YOUTUBE) // make sure to add more ifdefs when we get more services

#ifndef __ONLINE_SERVICE_DATA_HANDLER_H__
#define __ONLINE_SERVICE_DATA_HANDLER_H__

#include "zmm/zmm.h"
#include "mxml/mxml.h"
#include "cds_objects.h"

/// \brief this class is an interface for parsing content data of various
/// online services.
class OnlineServiceContentHandler : public zmm::Object
{
public:
    /// \brief Sets the service XML from which we will extract the objects.
    ///
    /// \todo specify rules of what to extarct (i.e. only a specific genre,etc)
    virtual void setServiceContent(zmm::Ref<mxml::Element> service) = 0;

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nil.
    ///
    /// \return CdsObject or nil if there are no more objects to parse.
    virtual zmm::Ref<CdsObject> getNextObject() = 0;


protected:
    zmm::Ref<mxml::Element> service_xml;
};

#endif//__ONLINE_SERVICE_DATA_HANDLER_H__

#endif//YOUTUBE

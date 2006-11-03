/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    upnp_cds.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file upnp_cds.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "upnp_cds.h"

using namespace zmm;

static Ref<ContentDirectoryService> instance;

ContentDirectoryService::ContentDirectoryService(String serviceType, String serviceID) : Object()
{
    this->serviceType = serviceType;
    this->serviceID = serviceID;
    systemUpdateID = 0;
}

Ref<ContentDirectoryService> ContentDirectoryService::createInstance(String serviceType, String serviceID)
{
    if (instance == nil)
    {
        instance = Ref<ContentDirectoryService>(new ContentDirectoryService(serviceType, serviceID));
    }
    return instance;
}

Ref<ContentDirectoryService> ContentDirectoryService::getInstance()
{
    return instance;
}


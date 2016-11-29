/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_cds.cc - this file is part of MediaTomb.
    
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

/// \file upnp_cds.cc

#include "upnp_cds.h"
#include "config_manager.h"

using namespace zmm;

SINGLETON_MUTEX(ContentDirectoryService, false);

String ContentDirectoryService::serviceType = nil;
String ContentDirectoryService::serviceID = nil;

ContentDirectoryService::ContentDirectoryService() : Singleton<ContentDirectoryService>()
{
    if (serviceType == nil || serviceID == nil)
        throw _Exception(_("serviceType or serviceID not set!"));
    systemUpdateID = 0;

    stringLimit = ConfigManager::getInstance()->getIntOption(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);
}

ContentDirectoryService::~ContentDirectoryService()
{
    serviceType = nil;
    serviceID = nil;
}

void ContentDirectoryService::setStaticArgs(String _serviceType, String _serviceID)
{
    serviceType = _serviceType;
    serviceID = _serviceID;
}

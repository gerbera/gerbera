/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service.cc - this file is part of MediaTomb.
    
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

/// \file online_service.cc

#ifdef ONLINE_SERVICES
#include "online_service.h" // API

#include "util/tools.h"

// DO NOT FORGET TO ADD SERVICE STORAGE PREFIXES TO THIS ARRAY WHEN ADDING
// NEW SERVICES!
static char service_prefixes[] = { '\0', 'Y', 'S', 'W', 'T', '\0' };

OnlineServiceList::OnlineServiceList()
{
    service_list.resize(OS_Max);
}

void OnlineServiceList::registerService(const std::shared_ptr<OnlineService>& service)
{
    if (service == nullptr)
        return;

    if (service->getServiceType() >= OS_Max) {
        throw std::runtime_error("Requested service with illegal type!");
    }

    service_list.at(service->getServiceType()) = service;
}

std::shared_ptr<OnlineService> OnlineServiceList::getService(service_type_t service)
{
    if ((service > OS_Max) || (service < 0))
        return nullptr;

    return service_list.at(service);
}

OnlineService::OnlineService()
{
    taskCount = 0;
    refresh_interval = 0;
    purge_interval = 0;
}

char OnlineService::getStoragePrefix(service_type_t service)
{
    if ((service < 0) || (service >= OS_Max))
        throw std::runtime_error("Illegal service requested\n");

    return service_prefixes[service];
}

char OnlineService::getStoragePrefix()
{
    return getStoragePrefix(getServiceType());
}

#endif //ONLINE_SERVICES

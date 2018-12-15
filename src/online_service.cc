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

#include "online_service.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

// DO NOT FORGET TO ADD SERVICE STORAGE PREFIXES TO THIS ARRAY WHEN ADDING
// NEW SERVICES!
char service_prefixes[] = { '\0', 'Y', 'S', 'W', 'T', '\0' };

OnlineServiceList::OnlineServiceList()
{
    service_list = Ref<Array<OnlineService>>(new Array<OnlineService>(OS_Max));
}

void OnlineServiceList::registerService(Ref<OnlineService> service)
{
    if (service == nullptr)
        return;

    if (service->getServiceType() >= OS_Max) {
        throw _Exception(_("Requested service with illegal type!"));
    }

    service_list->set(service, service->getServiceType());
}

Ref<OnlineService> OnlineServiceList::getService(service_type_t service)
{
    if ((service > OS_Max) || (service < 0))
        return nullptr;

    return service_list->get(service);
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
        throw _Exception(_("Illegal service requested\n"));

    return service_prefixes[service];
}

char OnlineService::getStoragePrefix()
{
    return getStoragePrefix(getServiceType());
}

String OnlineService::getCheckAttr(Ref<Element> xml, String attrname)
{
    String temp = xml->getAttribute(attrname);
    if (string_ok(temp))
        return temp;
    else
        throw _Exception(getServiceName() + _(": Tag <") + xml->getName() + _("> is missing the required \"") + attrname + _("\" attribute!"));
    return nullptr;
}

int OnlineService::getCheckPosIntAttr(Ref<Element> xml, String attrname)
{
    int itmp;
    String temp = xml->getAttribute(attrname);
    if (string_ok(temp))
        itmp = temp.toInt();
    else
        throw _Exception(getServiceName() + _(": Tag <") + xml->getName() + _("> is missing the required \"") + attrname + _("\" attribute!"));

    if (itmp < 1)
        throw _Exception(_("Invalid value in ") + attrname + _(" for <") + xml->getName() + _("> tag"));

    return itmp;
}

#endif //ONLINE_SERVICES

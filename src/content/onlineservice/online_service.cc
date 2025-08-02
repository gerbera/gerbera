/*MT*

    MediaTomb - http://www.mediatomb.cc/

    online_service.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::online
#include "online_service.h" // API

#include "content/content.h"
#include "context.h"
#include "exceptions.h"

// DO NOT FORGET TO ADD SERVICE STORAGE PREFIXES TO THIS ARRAY WHEN ADDING
// NEW SERVICES!
// keep old entries to avoid future collisions
static const std::map<OnlineServiceType, char> servicePrefixes {
    { OnlineServiceType::None, '\0' },
#ifdef YOUTUBE
    { OnlineServiceType::YouTube, 'Y' },
#endif
#ifdef SOPCAST
    { OnlineServiceType::SopCast, 'S' },
#endif
#ifdef WEBORAMA
    { OnlineServiceType::Weborama, 'W' },
#endif
#ifdef ATRAILERS
    { OnlineServiceType::ATrailers, 'T' },
#endif
    { OnlineServiceType::Max, '\0' }
};

void OnlineServiceList::registerService(const std::shared_ptr<OnlineService>& service)
{
    if (!service)
        return;

    if (service->getServiceType() >= OnlineServiceType::Max) {
        throw_std_runtime_error("Requested service with illegal type");
    }

    service_list[service->getServiceType()] = service;
}

std::shared_ptr<OnlineService> OnlineServiceList::getService(OnlineServiceType service) const
{
    if (service > OnlineServiceType::Max)
        return nullptr;

    return service_list.at(service);
}

OnlineService::OnlineService(const std::shared_ptr<Content>& content)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , content(content)
{
}

char OnlineService::getDatabasePrefix(OnlineServiceType service)
{
    if (service >= OnlineServiceType::Max)
        throw_std_runtime_error("Illegal service requested");

    return servicePrefixes.at(service);
}

char OnlineService::getDatabasePrefix() const
{
    return getDatabasePrefix(getServiceType());
}

#endif // ONLINE_SERVICES

/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    online_service_helper.cc - this file is part of MediaTomb.
    
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

/// \file online_service_helper.cc

#ifdef ONLINE_SERVICES
#include "online_service_helper.h" // API

#include "cds_objects.h"
#include "config/config_manager.h"
#include "online_service.h"

std::string OnlineServiceHelper::resolveURL(const std::shared_ptr<CdsItemExternalURL>& item)
{
    if (!item->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
        throw_std_runtime_error("The given item does not belong to an online service");

    auto service = service_type_t(std::stoi(item->getAuxData(ONLINE_SERVICE_AUX_ID)));
    if (service > OS_Max)
        throw_std_runtime_error("Invalid service id");

#ifdef SOPCAST
    if (service == OS_SopCast) {
        return item->getLocation().string();
    }
#endif
#ifdef ATRAILERS
    if (service == OS_ATrailers) {
        return item->getLocation().string();
    }
#endif
    throw_std_runtime_error("No handler for this service");
}

#endif //ONLINE_SERVICES

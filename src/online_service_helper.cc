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
/// \brief Definition of the OnlineServiceHelper class.

#ifdef ONLINE_SERVICES

#include "online_service_helper.h"
#include "config_manager.h"
#include "mxml/mxml.h"
#include "online_service.h"
#include "zmm/zmm.h"
#include "zmm/zmmf.h"

using namespace zmm;

OnlineServiceHelper::OnlineServiceHelper()
{
}

String OnlineServiceHelper::resolveURL(Ref<CdsItemExternalURL> item)
{
    if (!item->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
        throw _Exception(_("The given item does not belong to an online service"));

    service_type_t service = (service_type_t)(item->getAuxData(_(ONLINE_SERVICE_AUX_ID)).toInt());
    if (service > OS_Max)
        throw _Exception(_("Invalid service id!"));

    String url;

    switch (service) {
#ifdef SOPCAST
    case OS_SopCast:
        url = item->getLocation();
        break;
#endif
#ifdef ATRAILERS
    case OS_ATrailers:
        url = item->getLocation();
        break;
#endif
    case OS_Max:
    default:
        throw _Exception(_("No handler for this service!"));
    }

    return url;
}

#endif //ONLINE_SERVICES

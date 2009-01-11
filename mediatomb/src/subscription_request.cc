/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    subscription_request.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2009 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file subscription_request.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "mxml/mxml.h"
#include "subscription_request.h"

using namespace zmm;
using namespace mxml;

SubscriptionRequest::SubscriptionRequest(Upnp_Subscription_Request *upnp_request) : Object()
{
    this->upnp_request = upnp_request;

    serviceID = String(upnp_request->ServiceId);
    UDN = String(upnp_request->UDN);
    sID = String(upnp_request->Sid);
}

String SubscriptionRequest::getServiceID()
{
    return serviceID;
}

String SubscriptionRequest::getUDN()
{
    return UDN;
}

String SubscriptionRequest::getSubscriptionID()
{
    return sID;
}

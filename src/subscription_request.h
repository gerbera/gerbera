/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    subscription_request.h - this file is part of MediaTomb.
    
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

/// \file subscription_request.h
/// \brief Definition of the SubscriptionRequest class.

#ifndef __SUBSCRIPTION_REQUEST_H__
#define __SUBSCRIPTION_REQUEST_H__

#include <upnp.h>

#include "common.h"

/// \brief This class represents the Upnp_Subscription_Request type from the SDK.
///
/// When we get a Upnp_Subscription_Request from the SDK we convert it to our
/// structure. We then have the possibility to easily access various
/// information inside it.
class SubscriptionRequest : public zmm::Object {
protected:
    /// \brief Upnp_Subscription_Request that comes from the SDK.
    UpnpSubscriptionRequest* upnp_request;

    /// \brief ID of the service.
    ///
    /// Returned by getServiceID()
    zmm::String serviceID;

    /// \brief UDN of the recepient device (it should be our UDN)
    ///
    /// Returned by getUDN()
    zmm::String UDN;

    /// \brief Subscription ID.
    ///
    /// Returned by getSubscriptionID()
    zmm::String sID;

public:
    /// \brief The Constructor takes the values from the upnp_request and fills in internal variables.
    /// \param *upnp_request Pointer to the Upnp_Subscription_Request structure.
    SubscriptionRequest(UpnpSubscriptionRequest* upnp_request);

    /// \brief Returns the service ID (should be one of the services that we support).
    zmm::String getServiceID();

    /// \brief Returns the UDN of the recepient device (should be ours)
    zmm::String getUDN();

    /// \brief Returns the subscription ID.
    zmm::String getSubscriptionID();
};

#endif // __SUBSCRIPTION_REQUEST_H__

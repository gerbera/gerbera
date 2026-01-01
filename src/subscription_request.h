/*MT*

    MediaTomb - http://www.mediatomb.cc/

    subscription_request.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file subscription_request.h
/// @brief Definition of the SubscriptionRequest class.

#ifndef __SUBSCRIPTION_REQUEST_H__
#define __SUBSCRIPTION_REQUEST_H__

#include <string>
#include <upnp.h>

/// @brief This class represents the SubscriptionRequest type from the SDK.
///
/// When we get a SubscriptionRequest from the SDK we convert it to our
/// structure. We then have the possibility to easily access various
/// information inside it.
class SubscriptionRequest {
protected:
    /// @brief UpnpSubscriptionRequest that comes from the SDK.
    UpnpSubscriptionRequest* upnp_request;

    /// @brief ID of the service.
    ///
    /// Returned by getServiceID()
    std::string serviceID;

    /// @brief UDN of the recepient device (it should be our UDN)
    ///
    /// Returned by getUDN()
    std::string UDN;

    /// @brief Subscription ID.
    ///
    /// Returned by getSubscriptionID()
    std::string sID;

public:
    /// @brief The Constructor takes the values from the upnpRequest and fills in internal variables.
    /// @param upnpRequest Pointer to the UpnpSubscriptionRequest structure.
    explicit SubscriptionRequest(UpnpSubscriptionRequest* upnpRequest);

    /// @brief Returns the service ID (should be one of the services that we support).
    const std::string& getServiceID() const;

    /// @brief Returns the UDN of the recipient device (should be ours)
    const std::string& getUDN() const;

    /// @brief Returns the subscription ID.
    const std::string& getSubscriptionID() const;
};

#endif // __SUBSCRIPTION_REQUEST_H__

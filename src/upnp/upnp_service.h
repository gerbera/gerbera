/*GRB*

    Gerbera - https://gerbera.io/

    upnp_service.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file upnp/upnp_service.h
/// @brief Definition of the UpnpService class.

#ifndef __UPNP_SERVICE_H__
#define __UPNP_SERVICE_H__

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <upnp.h>

class ActionRequest;
class Config;
class SubscriptionRequest;
class UpnpXMLBuilder;

using ActionRequestHandler = std::function<void(ActionRequest& request)>;

/// @brief This class is responsible for the Upnp Service base
///
/// Handles subscription and action invocation requests
class UpnpService {
protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;

    UpnpDevice_Handle deviceHandle;
    /// @brief ID of the service.
    std::string serviceID;

    std::map<std::string, ActionRequestHandler> actionMap;
    bool offline { false };

public:
    /// @brief Constructor for UpnpService
    UpnpService(const std::shared_ptr<Config>& config,
        std::shared_ptr<UpnpXMLBuilder> xmlBuilder,
        UpnpDevice_Handle deviceHandle,
        std::string serviceId,
        bool offline = false);
    virtual ~UpnpService();

    UpnpService(const UpnpService&) = delete;
    UpnpService& operator=(const UpnpService&) = delete;

    std::string getServiceId() const { return serviceID; }
    bool isActiveMatch(const std::string& id) const { return !offline && id == serviceID; }
    /// @brief Dispatches the ActionRequest between the available actions.
    /// @param request Incoming ActionRequest.
    ///
    /// This function looks at the incoming ActionRequest and passes it on
    /// to the appropriate action for processing.
    void processActionRequest(ActionRequest& request) const;

    /// @brief Processes an incoming SubscriptionRequest.
    /// @param request Incoming SubscriptionRequest.
    ///
    /// Looks at the incoming SubscriptionRequest and accepts the subscription
    /// if everything is ok.
    virtual bool processSubscriptionRequest(const SubscriptionRequest& request) const
    {
        return false;
    }

    /// @brief Sends out an event to all subscribed devices.
    /// @param sourceProtocolCsv Comma Separated Value list of protocol information
    ///
    /// Sends out an update with protocol information to all subscribed devices
    virtual bool sendSubscriptionUpdate(const std::string& sourceProtocolCsv)
    {
        return false;
    }
};

#endif // __UPNP_SERVICE_H__

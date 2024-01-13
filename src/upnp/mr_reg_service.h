/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp_mrreg.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file upnp_mrreg.h
/// \brief Definition of the ConnectionManagerService class.

#ifndef __UPNP_MRREG_H__
#define __UPNP_MRREG_H__

#include <memory>

#include <upnp.h>

#include "common.h"
#include "context.h"

class ActionRequest;
class CdsObject;
class SubscriptionRequest;
class UpnpXMLBuilder;

/// \brief This class is responsible for the UPnP Connection Manager Service operations.
///
/// Handles subscription and action invocation requests for the Media Reciver Registrar.
/// This is not a full implementation of the service, the IsAuthorized and IsValidated
/// functions will always return true.
/// These functions were only implemented to enable Xbox360 support.
/// \todo the whole service class should be rewritten with the use of inheritance
class MRRegistrarService {
protected:
    /// \brief ID of the service.
    std::string serviceID;

    /// \brief Media Receiver Registrar service action: IsAuthorized()
    /// \param request Incoming ActionRequest.
    ///
    /// IsAuthorized(string DeviceID, i4 Result)
    ///
    /// This is currently unsupported (always returns 1)
    void doIsAuthorized(ActionRequest& request) const;

    /// \brief Media Receiver Registrar service action: RegisterDevice()
    /// \param request Incoming ActionRequest.
    ///
    /// RegisterDevice(bin.base64 RegistrationReqMsg, bin.base64 RegistrationRespMsg)
    ///
    /// This action is currently unsupported.
    void doRegisterDevice(ActionRequest& request) const;

    /// \brief Media Receiver Registrar service action: IsValidated()
    /// \param request Incoming ActionRequest.
    ///
    /// IsValidated(string DeviceID, i4 Result)
    void doIsValidated(ActionRequest& request) const;

    std::shared_ptr<Config> config;

    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;
    UpnpDevice_Handle deviceHandle;

public:
    /// \brief Constructor for MRReg
    /// in internal variables.
    MRRegistrarService(const std::shared_ptr<Context>& context,
        std::shared_ptr<UpnpXMLBuilder> xmlBuilder, UpnpDevice_Handle deviceHandle);

    /// \brief Dispatches the ActionRequest between the available actions.
    /// \param request Incoming ActionRequest.
    ///
    /// This function looks at the incoming ActionRequest and passes it on
    /// to the appropriate action for processing.
    void processActionRequest(ActionRequest& request);

    /// \brief Processes an incoming SubscriptionRequest.
    /// \param request Incoming SubscriptionRequest.
    ///
    /// Looks at the incoming SubscriptionRequest and accepts the subscription
    /// if everything is ok. Currently we do not support any updates.
    void processSubscriptionRequest(const SubscriptionRequest& request);

    // subscriptions for this service are not supported
    // subscriptions for this service are not supported

    /// \brief Sends out an event to all subscribed devices.
    /// \param sourceProtocol_CSV Comma Separated Value list of protocol information
    ///
    /// Sends out an update with protocol information to all subscribed devices
    //    void sendSubscriptionUpdate(std::string sourceProtocol_CSV);
};

#endif // __UPNP_MRREG_H__

/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    upnp_mrreg.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file upnp_mrreg.h
/// \brief Definition of the ConnectionManagerService class.
#ifndef __UPNP_MRREG_H__
#define __UPNP_MRREG_H__

#include "common.h"
#include "singleton.h"
#include "action_request.h"
#include "subscription_request.h"
#include "upnp_xml.h"

/// \brief This class is responsible for the UPnP Connection Manager Service operations.
///
/// Handles subscription and action invocation requests for the Media Reciver Registrar.
/// This is not a full implementation of the service, the IsAuthorized and IsValidated
/// functions will always return true.
/// These functions were only implemented to enable Xbox360 support.
/// \todo the whole service class should be rewritten with the use of inheritance
class MRRegistrarService : public Singleton<MRRegistrarService>
{

protected:
    /// \brief UPnP standard defined service type
    /// \todo Check if it makes sense to use it as it is done now...why not define constants here?
    static zmm::String serviceType;
    
    /// \brief ID of the service.
    static zmm::String serviceID;    

    /// \brief Media Receiver Registrar service action: IsAuthorized()
    /// \param request Incoming ActionRequest.
    ///
    /// IsAuthorized(string DeviceID, i4 Result)
    ///
    /// This is currently unsupported (always returns 1)
    void upnp_action_IsAuthorized(zmm::Ref<ActionRequest> request);

    /// \brief Media Receiver Registrar service action: RegisterDevice() 
    /// \param request Incoming ActionRequest.
    ///
    /// RegisterDevice(bin.base64 RegistrationReqMsg, bin.base64 RegistrationRespMsg)
    ///
    /// This action is currently unsupported.
    void upnp_action_RegisterDevice(zmm::Ref<ActionRequest> request);

    /// \brief Media Receiver Registrar service action: IsValidated()
    /// \param request Incoming ActionRequest.
    ///
    /// IsValidated(string DeviceID, i4 Result)
    void upnp_action_IsValidated(zmm::Ref<ActionRequest> request);
    
public:
    /// \brief Constructor for the CMS, saves the service type and service id
    /// in internal variables.
    /// \todo Check if it makes sense to use it as it is done now...why not define them as constants?
    MRRegistrarService();
    virtual ~MRRegistrarService();
    
    static void setStaticArgs(zmm::String serviceType, zmm::String serviceID);
    
    /// \brief Dispatches the ActionRequest between the available actions.
    /// \param request Incoming ActionRequest.
    ///
    /// This function looks at the incoming ActionRequest and passes it on
    /// to the appropriate action for processing.
    void process_action_request(zmm::Ref<ActionRequest> request);

    /// \brief Processes an incoming SubscriptionRequest.
    /// \param request Incoming SubscriptionRequest.
    ///
    /// Looks at the incoming SubscriptionRequest and accepts the subscription
    /// if everything is ok. Currently we do not support any updates.
    void process_subscription_request(zmm::Ref<SubscriptionRequest> request);

    // subscriptions for this service are not supporte  // subscriptions for this service are not supportedd

    /// \brief Sends out an event to all subscribed devices.
    /// \param sourceProtocol_CSV Comma Separated Value list of protocol information
    ///
    /// Sends out an update with protocol information to all subscribed devices
//    void subscription_update(zmm::String sourceProtocol_CSV);
        
};

#endif // __UPNP_CM_H__


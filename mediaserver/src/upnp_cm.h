/*  upnp_cm.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
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
*/

/// \file upnp_cm.h
/// \brief Definition of the ConnectionManagerService class.
#ifndef __UPNP_CM_H__
#define __UPNP_CM_H__

#include "common.h"
#include "action_request.h"
#include "subscription_request.h"
#include "upnp_xml.h"

/// \brief This class is responsible for the UPnP Connection Manager Service operations.
///
/// Handles subscription and action invocation requests for the Connection Manager.
class ConnectionManagerService : public zmm::Object
{

protected:
    /// \brief UPnP standard defined service type
    /// \todo Check if it makes sense to use it as it is done now...why not define constants here?
    zmm::String serviceType;
    
    /// \brief ID of the service.
    zmm::String serviceID;    

    /// \brief UPnP standard defined action: GetCurrentConnectionIDs()
    /// \param request Incoming ActionRequest.
    ///
    /// GetCurrentConnectionIDs(string ConnectionIDs)
    ///
    /// This is currently unsupported (returns empty string)
    void upnp_action_GetCurrentConnectionIDs(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: GetCurrentConnectionInfo()
    /// \param request Incoming ActionRequest.
    ///
    /// GetCurrentConnectionInfo(i4 ConnectoinID, i4 RcsID, i4 AVTransportID, string ProtocolInfo,
    /// string PeerConnectionManager, i4 PeerConnectionID, string Direction, string Status)
    ///
    /// This action is currently unsupported.
    void upnp_action_GetCurrentConnectionInfo(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: GetProtocolInfo()
    /// \param request Incoming ActionRequest.
    ///
    /// GetProtocolInfo(string Source, string Sink)
    void upnp_action_GetProtocolInfo(zmm::Ref<ActionRequest> request);
    
public:
    /// \brief Constructor for the CMS, saves the service type and service id
    /// in internal variables.
    /// \todo Check if it makes sense to use it as it is done now...why not define them as constants?
    ConnectionManagerService(zmm::String serviceType, zmm::String serviceID);

    /// \todo what is that here?? only getIntsance should be available, creating a new instance if called for the 1st time.
    static zmm::Ref<ConnectionManagerService> createInstance(zmm::String serviceType, zmm::String serviceID);
    static zmm::Ref<ConnectionManagerService> getInstance();

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
    /// if everything is ok.
    void process_subscription_request(zmm::Ref<SubscriptionRequest> request);

    /// \brief Sends out an event to all subscribed devices.
    /// \param sourceProtocol_CSV Comma Separated Value list of protocol information
    ///
    /// Sends out an update with protocol information to all subscribed devices
    void subscription_update(zmm::String sourceProtocol_CSV);
        
};

#endif // __UPNP_CM_H__


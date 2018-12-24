/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp_cds.h - this file is part of MediaTomb.

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

/// \file upnp_cds.h
/// \brief Definition of the ContentDirectoryService class.
#ifndef __UPNP_CDS_H__
#define __UPNP_CDS_H__

#include "action_request.h"
#include "common.h"
#include "singleton.h"
#include "subscription_request.h"
#include "upnp_xml.h"

/// \brief This class is responsible for the UPnP Content Directory Service operations.
///
/// Handles subscription and action invocation requests for the CDS.
class ContentDirectoryService {
protected:
    /// \brief The system update ID indicates changes in the content directory.
    ///
    /// Whenever something in the content directory changes, the value of
    /// systemUpdateID is increased and an event is sent out to all subscribed
    /// devices.
    /// Also, this variable is returned by the upnp_action_GetSystemUpdateID()
    /// action.
    int systemUpdateID;

    /// \brief All strings in the XML will be cut at this length.
    int stringLimit;

    /// \brief UPnP standard defined action: Browse()
    /// \param request Incoming ActionRequest.
    ///
    /// Browse(string ObjectID, string BrowseFlag, string Filter, ui4 StartingIndex,
    /// ui4 RequestedCount, string SortCriteria, string Result, ui4 NumberReturned,
    /// ui4 TotalMatches, ui4 UpdateID)
    void doBrowse(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: Search()
    /// \param request Incoming ActionRequest.
    ///
    /// Search(string ContainerID, string SearchCriteria, string Filter, ui4 StartingIndex,
    /// ui4 RequestedCount, string SortCriteria, string Result, ui4 NumberReturned,
    /// ui4 TotalMatches, ui4 UpdateID)
    void doSearch(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: GetSearchCapabilities()
    /// \param request Incoming ActionRequest.
    ///
    /// GetSearchCapabilities(string SearchCaps)
    void doGetSearchCapabilities(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: GetSortCapabilities()
    /// \param request Incoming ActionRequest.
    ///
    /// GetSortCapabilities(string SortCaps)
    void doGetSortCapabilities(zmm::Ref<ActionRequest> request);

    /// \brief UPnP standard defined action: GetSystemUpdateID()
    /// \param request Incoming ActionRequest.
    ///
    /// GetSystemUpdateID(ui4 Id)
    void doGetSystemUpdateID(zmm::Ref<ActionRequest> request);

    UpnpDevice_Handle deviceHandle;

    UpnpXMLBuilder* xmlBuilder;

public:
    /// \brief Constructor for the CDS, saves the service type and service id
    /// in internal variables.
    explicit ContentDirectoryService(UpnpXMLBuilder* builder, UpnpDevice_Handle deviceHandle, int stringLimit);
    ~ContentDirectoryService();

    /// \brief Dispatches the ActionRequest between the available actions.
    /// \param request ActionRequest to be processed by the function.
    ///
    /// This function looks at the incoming ActionRequest and passes it on
    /// to the appropriate action for processing.
    void processActionRequest(zmm::Ref<ActionRequest> request);

    /// \brief Processes an incoming SubscriptionRequest.
    /// \param request SubscriptionRequest to be processed by the function.
    ///
    /// Looks at the incoming SubscriptionRequest and accepts the subscription
    /// if everything is ok.
    void processSubscriptionRequest(zmm::Ref<SubscriptionRequest> request);

    /// \brief Sends out an event to all subscribed devices.
    /// \param containerUpdateIDs_CSV Comma Separated Value list of container update ID's (as defined in the UPnP CDS specs)
    ///
    /// When something in the content directory chagnes, we will send out
    /// an event to all subscribed devices. Container updates are supported,
    /// and of course the mimimum required - systemUpdateID.
    void sendSubscriptionUpdate(zmm::String containerUpdateIDs_CSV);
};

#endif // __UPNP_CDS_H__

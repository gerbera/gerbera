/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    action_request.h - this file is part of MediaTomb.
    
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

/// \file action_request.h
/// \brief Definition of the ActionRequest class.

#ifndef __ACTION_REQUEST_H__
#define __ACTION_REQUEST_H__

#include <memory>
#include <pugixml.hpp>
#include <upnp.h>

#include "common.h"

// forward declaration
class Context;
class Quirks;

/// \brief This class represents the Upnp_Action_Request type from the SDK.
///
/// When we get a Upnp_Action_Request from the SDK we convert it to our
/// structure. The idea is to get the XML of the request, process it outside
/// of the class, create a response XML and put it back in. Before passing
/// *upnp_request back to the SDK the update() function MUST be called.
class ActionRequest {
protected:
    /// \brief Upnp_Action_Request that comes from the SDK.
    UpnpActionRequest* upnp_request;

    /// \brief Error code that is returned to the SDK.
    int errCode;

    /// \brief Name of the action.
    ////request///
    /// Returned by getActionName()
    std::string actionName;

    /// \brief UDN of the recipient device (it should be our UDN)
    ///
    /// Returned by getUDN()
    std::string UDN;

    /// \brief ID of the service.
    ///
    /// Returned by getServiceID()
    std::string serviceID;

    /// \brief Client quirks
    std::shared_ptr<Quirks> quirks;

    /// \brief XML holding the response, we fill it in.
    ///
    /// Set by setResponse()
    std::unique_ptr<pugi::xml_document> response;

public:
    /// \brief The Constructor takes the values from the upnp_request and fills in internal variables.
    /// \param *upnp_request Pointer to the Upnp_Action_Request structure.
    explicit ActionRequest(std::shared_ptr<Context> context, UpnpActionRequest* upnp_request);

    /// \brief Returns the name of the action.
    std::string getActionName() const;

    /// \brief Returns the UDN of the recipient device (should be ours)
    std::string getUDN() const;

    /// \brief Returns the ID of the service (the action is for this service id)
    std::string getServiceID() const;

    /// \brief Returns the XML representation of the request, that comes to us.
    std::unique_ptr<pugi::xml_document> getRequest() const;

    /// \brief Returns the client quirks
    std::shared_ptr<Quirks> getQuirks() const;

    /// \brief Sets the response (XML created outside as the answer to the request)
    /// \param response XML holding the action response.
    void setResponse(std::unique_ptr<pugi::xml_document> response);

    /// \brief Set the error code for the SDK.
    /// \param errCode UPnP error code.
    ///
    /// In case there was an error processing the request, we can return the
    /// appropriate error code to the SDK.
    void setErrorCode(int errCode);

    /// \brief Updates the upnp_request structure.
    ///
    /// This function writes all changed values into the upnp_request structure, and
    /// must be called at the very end before giving *upnp_request back to the SDK.
    void update();

    std::shared_ptr<Quirks> GetQuirks() { return quirks; }
};

#endif // __ACTION_REQUEST_H__

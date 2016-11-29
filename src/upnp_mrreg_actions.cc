/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_mrreg_actions.cc - this file is part of MediaTomb.
    
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

/// \file upnp_mrreg_actions.cc

#if defined(ENABLE_MRREG)

#include "tools.h"
#include "upnp_mrreg.h"
#include "storage.h"

using namespace zmm;
using namespace mxml;

void MRRegistrarService::upnp_action_IsAuthorized(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);
    response->appendTextChild(_("Result"), _("1"));

    request->setResponse(response); 
    request->setErrorCode(UPNP_E_SUCCESS);    
   
    log_debug("end\n");
}

void MRRegistrarService::upnp_action_RegisterDevice(Ref<ActionRequest> request)
{
    log_debug("start\n");

    request->setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("upnp_action_GetCurrentConnectionInfo: end\n");
}

void MRRegistrarService::upnp_action_IsValidated(Ref<ActionRequest> request)
{
    log_debug("start\n");
 
    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);
    response->appendTextChild(_("Result"), _("1"));

    request->setResponse(response); 
    request->setErrorCode(UPNP_E_SUCCESS);    
    
    log_debug("end\n");
}

void MRRegistrarService::process_action_request(Ref<ActionRequest> request)
{
    log_debug("start\n");

    if (request->getActionName() == "IsAuthorized")
    {
        upnp_action_IsAuthorized(request);
    }
    else if (request->getActionName() == "RegisterDevice")
    {
        upnp_action_RegisterDevice(request);
    }
    else if (request->getActionName() == "IsValidated")
    {
        upnp_action_IsValidated(request);
    }
    else
    {
        // invalid or unsupported action
        log_debug("unrecognized action %s\n", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        //throw UpnpException(UPNP_E_INVALID_ACTION, _("unrecognized action"));
    }
    
    log_debug("end\n");

}

#endif // ENABLE_MRREG

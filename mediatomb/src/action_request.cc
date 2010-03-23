/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    action_request.cc - this file is part of MediaTomb.
    
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

/// \file action_request.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "action_request.h"

using namespace zmm;
using namespace mxml;

ActionRequest::ActionRequest(Upnp_Action_Request *upnp_request) : Object()
{
    this->upnp_request = upnp_request;

    errCode = UPNP_E_SUCCESS;
    actionName = upnp_request->ActionName;
    UDN = upnp_request->DevUDN;
    serviceID = upnp_request->ServiceID;

    DOMString cxml = ixmlPrintDocument(upnp_request->ActionRequest);
    String xml = cxml;
    ixmlFreeDOMString(cxml);
   
    Ref<Parser> parser(new Parser());

    request = parser->parseString(xml)->getRoot();
}

String ActionRequest::getActionName()
{
    return actionName;
}
String ActionRequest::getUDN()
{
    return UDN;
}
String ActionRequest::getServiceID()
{
    return serviceID;
}
Ref<Element> ActionRequest::getRequest()
{
    return request;
}

void ActionRequest::setResponse(Ref<Element> response)
{
    this->response = response;
}
void ActionRequest::setErrorCode(int errCode)
{
    this->errCode = errCode;
}

void ActionRequest::update()
{
    if(response != nil)
    {
        String xml = response->print();
        int ret;

        //log_debug("ActionRequest::update(): \n%s\n\n", xml.c_str());
        
        ret = ixmlParseBufferEx(xml.c_str(), &upnp_request->ActionResult);
        if (ret != IXML_SUCCESS)
        {
            log_error("ActionRequest::update(): could not convert to iXML\n");
            log_debug("Dump:\n%s\n", xml.c_str());
            upnp_request->ErrCode = UPNP_E_ACTION_FAILED;    
        } 
        else
        {
//            log_debug("ActionRequest::update(): converted to iXML, code %d\n", errCode);
            upnp_request->ErrCode = errCode;    
        }
    }
    else
    {
        // ok, here there can be two cases
        // either the function below already did set an error code,
        // then we keep it
        // if it did not do so - we set an error code of our own
        if (errCode == UPNP_E_SUCCESS) 
        {   
            upnp_request->ErrCode = UPNP_E_ACTION_FAILED;
        }
        
        log_error("ActionRequest::update(): response is nil, code %d\n", errCode);
    }
}

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

#include <sstream>

#include "action_request.h"

ActionRequest::ActionRequest(UpnpActionRequest* upnp_request)
    : upnp_request(upnp_request)
    , errCode(UPNP_E_SUCCESS)
    , actionName(UpnpActionRequest_get_ActionName_cstr(upnp_request))
    , UDN(UpnpActionRequest_get_DevUDN_cstr(upnp_request))
    , serviceID(UpnpActionRequest_get_ServiceID_cstr(upnp_request))
{
}

std::string ActionRequest::getActionName()
{
    return actionName;
}

std::string ActionRequest::getUDN()
{
    return UDN;
}

std::string ActionRequest::getServiceID()
{
    return serviceID;
}

std::unique_ptr<pugi::xml_document> ActionRequest::getRequest()
{
    DOMString cxml = ixmlPrintDocument(UpnpActionRequest_get_ActionRequest(upnp_request));
    auto request = std::make_unique<pugi::xml_document>();
    auto ret = request->load_string(cxml);
    ixmlFreeDOMString(cxml);

    if (ret.status != pugi::xml_parse_status::status_ok)
        throw std::runtime_error("Unable to parse ixml");

    return request;
}

void ActionRequest::setResponse(std::unique_ptr<pugi::xml_document>& response)
{
    this->response = std::move(response);
}

void ActionRequest::setErrorCode(int errCode)
{
    this->errCode = errCode;
}

void ActionRequest::update()
{
    if (response != nullptr) {
        std::ostringstream buf;
        response->print(buf, "", 0);
        std::string xml = buf.str();
        log_debug("ActionRequest::update(): {}", xml.c_str());

        IXML_Document* result = nullptr;
        int err = ixmlParseBufferEx(xml.c_str(), &result);

        if (err != IXML_SUCCESS) {
            log_error("ActionRequest::update(): could not convert to iXML");
            UpnpActionRequest_set_ErrCode(upnp_request, UPNP_E_ACTION_FAILED);
        } else {
            log_debug("ActionRequest::update(): converted to iXML, code {}", errCode);
            UpnpActionRequest_set_ActionResult(upnp_request, result);
            UpnpActionRequest_set_ErrCode(upnp_request, errCode);
        }
    } else {
        // ok, here there can be two cases
        // either the function below already did set an error code,
        // then we keep it
        // if it did not do so - we set an error code of our own
        if (errCode == UPNP_E_SUCCESS) {
            UpnpActionRequest_set_ErrCode(upnp_request, UPNP_E_ACTION_FAILED);
        }

        log_error("ActionRequest::update(): response is nullptr, code {} for {}", errCode, actionName.c_str());
    }
}

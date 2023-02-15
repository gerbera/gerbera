/*MT*

    MediaTomb - http://www.mediatomb.cc/

    action_request.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::requests

#include "action_request.h" // API

#include "upnp_common.h"
#include "upnp_xml.h"
#include "util/grb_net.h"
#include "util/upnp_quirks.h"

ActionRequest::ActionRequest(std::shared_ptr<UpnpXMLBuilder> xmlBuilder, std::shared_ptr<ClientManager> clients, UpnpActionRequest* upnpRequest)
    : upnp_request(upnpRequest)
    , actionName(UpnpActionRequest_get_ActionName_cstr(upnpRequest))
    , UDN(UpnpActionRequest_get_DevUDN_cstr(upnpRequest))
    , serviceID(UpnpActionRequest_get_ServiceID_cstr(upnpRequest))
{
    auto ctrlPtIPAddr = std::make_shared<GrbNet>(UpnpActionRequest_get_CtrlPtIPAddr(upnpRequest));
    std::string userAgent = UpnpActionRequest_get_Os_cstr(upnpRequest);
    quirks = std::make_unique<Quirks>(xmlBuilder, clients, ctrlPtIPAddr, userAgent);
}

std::string ActionRequest::getActionName() const
{
    return actionName;
}

std::string ActionRequest::getUDN() const
{
    return UDN;
}

std::string ActionRequest::getServiceID() const
{
    return serviceID;
}

const std::unique_ptr<Quirks>& ActionRequest::getQuirks() const
{
    return quirks;
}

std::unique_ptr<pugi::xml_document> ActionRequest::getRequest() const
{
    auto request = std::make_unique<pugi::xml_document>();
#if defined(USING_NPUPNP)
    auto ret = request->load_string(upnp_request->xmlAction.c_str());
#else
    DOMString cxml = ixmlPrintDocument(UpnpActionRequest_get_ActionRequest(upnp_request));
    auto ret = request->load_string(cxml);
    ixmlFreeDOMString(cxml);
#endif

    if (ret.status != pugi::xml_parse_status::status_ok)
        throw_std_runtime_error("Unable to parse ixml: {}", ret.description());

    return request;
}

void ActionRequest::setResponse(std::unique_ptr<pugi::xml_document> response)
{
    this->response = std::move(response);
}

void ActionRequest::setErrorCode(int errCode)
{
    this->errCode = errCode;
}

void ActionRequest::update()
{
    if (response) {
        std::string xml = UpnpXMLBuilder::printXml(*response, "", 0);
        log_debug("xml: {}", xml);

#if defined(USING_NPUPNP)
        UpnpActionRequest_set_xmlResponse(upnp_request, xml);
        UpnpActionRequest_set_ErrCode(upnp_request, errCode);
#else
        IXML_Document* result = nullptr;
        int err = ixmlParseBufferEx(xml.c_str(), &result);

        if (err != IXML_SUCCESS) {
            log_error("ActionRequest::update(): could not convert to iXML, code {}", err);
            UpnpActionRequest_set_ErrCode(upnp_request, UPNP_E_ACTION_FAILED);
            if (result)
                ixmlDocument_free(result);
        } else {
            log_debug("ActionRequest::update(): converted to iXML, code {}", errCode);
            UpnpActionRequest_set_ActionResult(upnp_request, result);
            UpnpActionRequest_set_ErrCode(upnp_request, errCode);
        }
#endif
    } else {
        // ok, here there can be two cases
        // either the function below already did set an error code,
        // then we keep it
        // if it did not do so - we set an error code of our own
        if (errCode == UPNP_E_SUCCESS) {
            UpnpActionRequest_set_ErrCode(upnp_request, UPNP_E_ACTION_FAILED);
        }

        log_error("ActionRequest::update(): response is nullptr, code {} for {}", errCode, actionName);
    }
}

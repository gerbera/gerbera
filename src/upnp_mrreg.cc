/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_mrreg.cc - this file is part of MediaTomb.
    
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

/// \file upnp_mrreg.cc

#include "upnp_mrreg.h" // API

#include <utility>

#include "config/config_manager.h"
#include "database/database.h"
#include "server.h"
#include "upnp_xml.h"
#include "util/tools.h"

MRRegistrarService::MRRegistrarService(std::shared_ptr<Config> config,
    UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle)
    : config(std::move(config))
    , xmlBuilder(xmlBuilder)
    , deviceHandle(deviceHandle)
{
}

MRRegistrarService::~MRRegistrarService() = default;

void MRRegistrarService::doIsAuthorized(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_MRREG_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("Result").append_child(pugi::node_pcdata).set_value("1");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void MRRegistrarService::doRegisterDevice(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("upnpActionGetCurrentConnectionInfo: end");
}

void MRRegistrarService::doIsValidated(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = UpnpXMLBuilder::createResponse(request->getActionName(), UPNP_DESC_MRREG_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("Result").append_child(pugi::node_pcdata).set_value("1");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void MRRegistrarService::processActionRequest(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    if (request->getActionName() == "IsAuthorized") {
        doIsAuthorized(request);
    } else if (request->getActionName() == "RegisterDevice") {
        doRegisterDevice(request);
    } else if (request->getActionName() == "IsValidated") {
        doIsValidated(request);
    } else {
        // invalid or unsupported action
        log_debug("unrecognized action {}", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        //throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("end");
}

void MRRegistrarService::processSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request)
{
    auto propset = UpnpXMLBuilder::createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("ValidationRevokedUpdateID").append_child(pugi::node_pcdata).set_value("0");
    property.append_child("ValidationSucceededUpdateID").append_child(pugi::node_pcdata).set_value("0");
    property.append_child("AuthorizationDeniedUpdateID").append_child(pugi::node_pcdata).set_value("0");
    property.append_child("AuthorizationGrantedUpdateID").append_child(pugi::node_pcdata).set_value("0");

    std::ostringstream buf;
    propset->print(buf, "", 0);
    std::string xml = buf.str();

#if defined(USING_NPUPNP)
    UpnpAcceptSubscriptionXML(
        deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_MRREG_SERVICE_ID, xml, request->getSubscriptionID().c_str());
#else
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_MRREG_SERVICE_ID, event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
#endif
}

// TODO: FIXME
#if 0
void MRRegistrarService::prcoessSubscriptionUpdate(std::string sourceProtocol_CSV)
{
    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SourceProtocolInfo").append_child(pugi::node_pcdata).set_value(sourceProtocol_CSV.c_str());
    property->appendTextChild("SourceProtocolInfo", sourceProtocol_CSV);

    std::ostringstream buf;
    propset->print(buf, "", 0);
    std::string xml = buf.str();

    IXML_Document *event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS)
    {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");

    }

    UpnpNotifyExt(deviceHandle,
            config->getOption(CFG_SERVER_UDN).c_str(),
            serviceID.c_str(), event);

    ixmlDocument_free(event);
}
#endif

/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp_cm.cc - this file is part of MediaTomb.

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

/// \file upnp_cm.cc
#define LOG_FAC log_facility_t::connmgr

#include "conn_mgr_service.h" // API

#include "action_request.h"
#include "config/config_manager.h"
#include "database/database.h"
#include "subscription_request.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"

ConnectionManagerService::ConnectionManagerService(const std::shared_ptr<Context>& context,
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder, UpnpDevice_Handle deviceHandle)
    : config(context->getConfig())
    , database(context->getDatabase())
    , xmlBuilder(std::move(xmlBuilder))
    , deviceHandle(deviceHandle)
{
}

void ConnectionManagerService::doGetCurrentConnectionIDs(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CM_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("ConnectionID").append_child(pugi::node_pcdata).set_value("0");

    request.setResponse(std::move(response));
    request.setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void ConnectionManagerService::doGetCurrentConnectionInfo(ActionRequest& request) const
{
    log_debug("start");

    request.setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("doGetCurrentConnectionInfo: end");
}

void ConnectionManagerService::doGetProtocolInfo(ActionRequest& request) const
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request.getActionName(), UPNP_DESC_CM_SERVICE_TYPE);
    auto csv = mimeTypesToCsv(database->getMimeTypes());
    auto root = response->document_element();

    root.append_child("Source").append_child(pugi::node_pcdata).set_value(csv.c_str());
    root.append_child("Sink").append_child(pugi::node_pcdata).set_value("");

    request.setResponse(std::move(response));
    request.setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void ConnectionManagerService::processActionRequest(ActionRequest& request) const
{
    log_debug("start");

    if (request.getActionName() == "GetCurrentConnectionIDs") {
        doGetCurrentConnectionIDs(request);
    } else if (request.getActionName() == "GetCurrentConnectionInfo") {
        doGetCurrentConnectionInfo(request);
    } else if (request.getActionName() == "GetProtocolInfo") {
        doGetProtocolInfo(request);
    } else {
        // invalid or unsupported action
        log_debug("unrecognized action {}", request.getActionName().c_str());
        request.setErrorCode(UPNP_E_INVALID_ACTION);
        //        throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("end");
}

void ConnectionManagerService::processSubscriptionRequest(const SubscriptionRequest& request) const
{
    auto csv = mimeTypesToCsv(database->getMimeTypes());
    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();

    property.append_child("CurrentConnectionIDs").append_child(pugi::node_pcdata).set_value("0");
    property.append_child("SinkProtocolInfo").append_child(pugi::node_pcdata).set_value("");
    property.append_child("SourceProtocolInfo").append_child(pugi::node_pcdata).set_value(csv.c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);

#if defined(USING_NPUPNP)
    UpnpAcceptSubscriptionXML(
        deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CM_SERVICE_ID, xml, request.getSubscriptionID().c_str());
#else
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        UPNP_DESC_CM_SERVICE_ID, event, request.getSubscriptionID().c_str());

    ixmlDocument_free(event);
#endif
}

void ConnectionManagerService::sendSubscriptionUpdate(const std::string& sourceProtocolCsv)
{
    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SourceProtocolInfo").append_child(pugi::node_pcdata).set_value(sourceProtocolCsv.c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);

#if defined(USING_NPUPNP)
    UpnpNotifyXML(deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(), UPNP_DESC_CM_SERVICE_ID, xml);
#else
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpNotifyExt(deviceHandle, config->getOption(CFG_SERVER_UDN).c_str(), UPNP_DESC_CM_SERVICE_ID, event);

    ixmlDocument_free(event);
#endif
}

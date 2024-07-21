/*MT*

    MediaTomb - http://www.mediatomb.cc/

    conn_mgr_service.cc - this file is part of MediaTomb.

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

/// \file conn_mgr_service.cc
#define GRB_LOG_FAC GrbLogFacility::connmgr

#include "conn_mgr_service.h" // API

#include "action_request.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "database/database.h"
#include "exceptions.h"
#include "subscription_request.h"
#include "upnp/compat.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/logger.h"
#include "util/tools.h"

ConnectionManagerService::ConnectionManagerService(const std::shared_ptr<Context>& context,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    UpnpDevice_Handle deviceHandle)
    : UpnpService(context->getConfig(), xmlBuilder, deviceHandle, UPNP_DESC_CM_SERVICE_ID)
    , database(context->getDatabase())
{
    actionMap = {
        { "GetCurrentConnectionIDs", [this](ActionRequest& r) { doGetCurrentConnectionIDs(r); } },
        { "GetCurrentConnectionInfo", [this](ActionRequest& r) { doGetCurrentConnectionInfo(r); } },
        { "GetProtocolInfo", [this](ActionRequest& r) { doGetProtocolInfo(r); } },
    };
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

    log_debug("end");
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

bool ConnectionManagerService::processSubscriptionRequest(const SubscriptionRequest& request)
{
    auto csv = mimeTypesToCsv(database->getMimeTypes());
    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();

    property.append_child("CurrentConnectionIDs").append_child(pugi::node_pcdata).set_value("0");
    property.append_child("SinkProtocolInfo").append_child(pugi::node_pcdata).set_value("");
    property.append_child("SourceProtocolInfo").append_child(pugi::node_pcdata).set_value(csv.c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);

    return UPNP_E_SUCCESS == GrbUpnpAcceptSubscription( //
               deviceHandle, config->getOption(ConfigVal::SERVER_UDN), //
               serviceID, xml, request.getSubscriptionID());
}

bool ConnectionManagerService::sendSubscriptionUpdate(const std::string& sourceProtocolCsv)
{
    auto propset = xmlBuilder->createEventPropertySet();
    auto property = propset->document_element().first_child();
    property.append_child("SourceProtocolInfo").append_child(pugi::node_pcdata).set_value(sourceProtocolCsv.c_str());

    std::string xml = UpnpXMLBuilder::printXml(*propset, "", 0);
    return UPNP_E_SUCCESS == GrbUpnpNotify( //
               deviceHandle, //
               config->getOption(ConfigVal::SERVER_UDN), serviceID, xml);
}

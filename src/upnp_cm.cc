/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_cm.cc - this file is part of MediaTomb.
    
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

/// \file upnp_cm.cc

#include "upnp_cm.h"
#include "config/config_manager.h"
#include "storage/storage.h"
#include "server.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

ConnectionManagerService::ConnectionManagerService(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle)
    : config(config)
    , storage(storage)
    , xmlBuilder(xmlBuilder)
    , deviceHandle(deviceHandle)
{
}

ConnectionManagerService::~ConnectionManagerService() = default;

void ConnectionManagerService::doGetCurrentConnectionIDs(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request->getActionName(), DESC_CM_SERVICE_TYPE);
    auto root = response->document_element();
    root.append_child("ConnectionID").append_child(pugi::node_pcdata).set_value("0");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void ConnectionManagerService::doGetCurrentConnectionInfo(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    request->setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("doGetCurrentConnectionInfo: end");
}

void ConnectionManagerService::doGetProtocolInfo(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    auto response = xmlBuilder->createResponse(request->getActionName(), DESC_CM_SERVICE_TYPE);

    std::vector<std::string> mimeTypes = storage->getMimeTypes();
    std::string CSV = mime_types_to_CSV(mimeTypes);

    auto root = response->document_element();
    root.append_child("Source").append_child(pugi::node_pcdata).set_value(CSV.c_str());
    root.append_child("Sink").append_child(pugi::node_pcdata).set_value("");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end");
}

void ConnectionManagerService::processActionRequest(const std::unique_ptr<ActionRequest>& request)
{
    log_debug("start");

    if (request->getActionName() == "GetCurrentConnectionIDs") {
        doGetCurrentConnectionIDs(request);
    } else if (request->getActionName() == "GetCurrentConnectionInfo") {
        doGetCurrentConnectionInfo(request);
    } else if (request->getActionName() == "GetProtocolInfo") {
        doGetProtocolInfo(request);
    } else {
        // invalid or unsupported action
        log_debug("unrecognized action {}", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        //        throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("end");
}

void ConnectionManagerService::processSubscriptionRequest(const std::unique_ptr<SubscriptionRequest>& request)
{
    int err;
    IXML_Document* event = nullptr;

    Ref<Element> propset, property;

    std::vector<std::string> mimeTypes = storage->getMimeTypes();
    std::string CSV = mime_types_to_CSV(mimeTypes);

    propset = xmlBuilder->createEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild(std::string("CurrentConnectionIDs"), "0");
    property->appendTextChild(std::string("SinkProtocolInfo"), "");
    property->appendTextChild(std::string("SourceProtocolInfo"), CSV);

    std::string xml = propset->print();
    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        DESC_CM_SERVICE_ID, event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
}

void ConnectionManagerService::sendSubscriptionUpdate(std::string sourceProtocol_CSV)
{
    int err;
    IXML_Document* event = nullptr;

    Ref<Element> propset, property;

    propset = xmlBuilder->createEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild("SourceProtocolInfo", sourceProtocol_CSV);

    std::string xml = propset->print();

    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpNotifyExt(deviceHandle,
        config->getOption(CFG_SERVER_UDN).c_str(),
        DESC_CM_SERVICE_ID, event);

    ixmlDocument_free(event);
}

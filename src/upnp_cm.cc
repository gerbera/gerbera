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
#include "server.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

ConnectionManagerService::ConnectionManagerService(UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle)
    : xmlBuilder(xmlBuilder)
    , deviceHandle(deviceHandle)
{
}

ConnectionManagerService::~ConnectionManagerService() = default;

void ConnectionManagerService::doGetCurrentConnectionIDs(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_CM_SERVICE_TYPE);
    response->appendTextChild("ConnectionID", "0");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end\n");
}

void ConnectionManagerService::doGetCurrentConnectionInfo(Ref<ActionRequest> request)
{
    log_debug("start\n");

    request->setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("doGetCurrentConnectionInfo: end\n");
}

void ConnectionManagerService::doGetProtocolInfo(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_CM_SERVICE_TYPE);

    std::vector<std::string> mimeTypes = Storage::getInstance()->getMimeTypes();
    std::string CSV = mime_types_to_CSV(mimeTypes);

    response->appendTextChild("Source", CSV);
    response->appendTextChild("Sink", "");

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end\n");
}

void ConnectionManagerService::processActionRequest(Ref<ActionRequest> request)
{
    log_debug("start\n");

    if (request->getActionName() == "GetCurrentConnectionIDs") {
        doGetCurrentConnectionIDs(request);
    } else if (request->getActionName() == "GetCurrentConnectionInfo") {
        doGetCurrentConnectionInfo(request);
    } else if (request->getActionName() == "GetProtocolInfo") {
        doGetProtocolInfo(request);
    } else {
        // invalid or unsupported action
        log_debug("unrecognized action %s\n", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        //        throw UpnpException(UPNP_E_INVALID_ACTION, "unrecognized action");
    }

    log_debug("end\n");
}

void ConnectionManagerService::processSubscriptionRequest(zmm::Ref<SubscriptionRequest> request)
{
    int err;
    IXML_Document* event = nullptr;

    Ref<Element> propset, property;

    std::vector<std::string> mimeTypes = Storage::getInstance()->getMimeTypes();
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
        ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
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
        ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
        DESC_CM_SERVICE_ID, event);

    ixmlDocument_free(event);
}

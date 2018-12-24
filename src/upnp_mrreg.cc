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

#include "upnp_mrreg.h"
#include "ixml.h"
#include "server.h"
#include "storage.h"
#include "tools.h"
#include "upnp_xml.h"

using namespace zmm;
using namespace mxml;

MRRegistrarService::MRRegistrarService(UpnpXMLBuilder* xmlBuilder, UpnpDevice_Handle deviceHandle)
    : xmlBuilder(xmlBuilder)
    , deviceHandle(deviceHandle)
{
}

MRRegistrarService::~MRRegistrarService() = default;

void MRRegistrarService::doIsAuthorized(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_MRREG_SERVICE_TYPE);
    response->appendTextChild(_("Result"), _("1"));

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end\n");
}

void MRRegistrarService::doRegisterDevice(Ref<ActionRequest> request)
{
    log_debug("start\n");

    request->setErrorCode(UPNP_E_NOT_EXIST);

    log_debug("upnpActionGetCurrentConnectionInfo: end\n");
}

void MRRegistrarService::doIsValidated(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = xmlBuilder->createResponse(request->getActionName(), DESC_MRREG_SERVICE_TYPE);
    response->appendTextChild(_("Result"), _("1"));

    request->setResponse(response);
    request->setErrorCode(UPNP_E_SUCCESS);

    log_debug("end\n");
}

void MRRegistrarService::processActionRequest(Ref<ActionRequest> request)
{
    log_debug("start\n");

    if (request->getActionName() == "IsAuthorized") {
        doIsAuthorized(request);
    } else if (request->getActionName() == "RegisterDevice") {
        doRegisterDevice(request);
    } else if (request->getActionName() == "IsValidated") {
        doIsValidated(request);
    } else {
        // invalid or unsupported action
        log_debug("unrecognized action %s\n", request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
        //throw UpnpException(UPNP_E_INVALID_ACTION, _("unrecognized action"));
    }

    log_debug("end\n");
}

void MRRegistrarService::processSubscriptionRequest(zmm::Ref<SubscriptionRequest> request)
{
    int err;
    IXML_Document* event = NULL;

    Ref<Element> propset, property;

    propset = xmlBuilder->createEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild(_("ValidationRevokedUpdateID"), _("0"));
    property->appendTextChild(_("ValidationSucceededUpdateID"), _("0"));
    property->appendTextChild(_("AuthorizationDeniedUpdateID"), _("0"));
    property->appendTextChild(_("AuthorizationGrantedUpdateID"), _("0"));

    String xml = propset->print();
    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, _("Could not convert property set to ixml"));
    }

    UpnpAcceptSubscriptionExt(deviceHandle,
        ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
        DESC_MRREG_SERVICE_ID, event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
}

// TODO: FIXME
#if 0
void MRRegistrarService::subscription_update(String sourceProtocol_CSV)
{
    int err;
    IXML_Document *event = NULL;

    Ref<Element> propset, property;

    propset = UpnpXML_CreateEventPropertySet();
    property = propset->getFirstChild();
    property->appendTextChild(_("SourceProtocolInfo"), sourceProtocol_CSV);

    String xml = propset->print();

    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS)
    {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, _("Could not convert property set to ixml"));

    }

    UpnpNotifyExt(deviceHandle,
            ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
            serviceID.c_str(), event);

    ixmlDocument_free(event);
}
#endif

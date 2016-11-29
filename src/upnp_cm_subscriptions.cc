/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_cm_subscriptions.cc - this file is part of MediaTomb.
    
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

/// \file upnp_cm_subscriptions.cc

#include "tools.h"
#include "upnp_cm.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

void ConnectionManagerService::process_subscription_request(zmm::Ref<SubscriptionRequest> request)
{
    int err;
    IXML_Document *event = NULL;

    Ref<Element> propset, property;

    Ref<Array<StringBase> > mimeTypes = Storage::getInstance()->getMimeTypes();
    String CSV = mime_types_to_CSV(mimeTypes);


    propset = UpnpXML_CreateEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild(_("CurrentConnectionIDs"), _("0"));
    property->appendTextChild(_("SinkProtocolInfo"), _(""));
    property->appendTextChild(_("SourceProtocolInfo"), CSV);

    String xml = propset->print();
    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS)
    {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, _("Could not convert property set to ixml"));
    }

    UpnpAcceptSubscriptionExt(Server::getInstance()->getDeviceHandle(),
            ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
            serviceID.c_str(), event, request->getSubscriptionID().c_str());

    ixmlDocument_free(event);
}

void ConnectionManagerService::subscription_update(String sourceProtocol_CSV)
{
    int err;
    IXML_Document *event = NULL;

    Ref<Element> propset, property;

    propset = UpnpXML_CreateEventPropertySet();
    property = propset->getFirstElementChild();
    property->appendTextChild(_("SourceProtocolInfo"), sourceProtocol_CSV);

    String xml = propset->print();

    err = ixmlParseBufferEx(xml.c_str(), &event);
    if (err != IXML_SUCCESS)
    {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, _("Could not convert property set to ixml"));

    }

    UpnpNotifyExt(Server::getInstance()->getDeviceHandle(),
            ConfigManager::getInstance()->getOption(CFG_SERVER_UDN).c_str(),
            serviceID.c_str(), event);

    ixmlDocument_free(event);
}

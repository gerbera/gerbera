/*GRB*

    Gerbera - https://gerbera.io/

    upnp_service.cc - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file upnp_service.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#include "upnp_service.h" // API

#include "action_request.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "exceptions.h"
#include "subscription_request.h"
#include "upnp/compat.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/logger.h"
#include "util/tools.h"

UpnpService::UpnpService(const std::shared_ptr<Config>& config,
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder,
    UpnpDevice_Handle deviceHandle,
    std::string serviceId,
    bool offline)
    : config(config)
    , xmlBuilder(std::move(xmlBuilder))
    , deviceHandle(deviceHandle)
    , serviceID(std::move(serviceId))
    , offline(offline)
{
}

void UpnpService::processActionRequest(ActionRequest& request) const
{
    log_debug("start");

    if (actionMap.find(request.getActionName()) != actionMap.end()) {
        log_debug("call {}", request.getActionName());
        actionMap.at(request.getActionName())(request);
    } else {
        // invalid or unsupported action
        log_debug("{}: unrecognized action '{}'", serviceID, request.getActionName());
        request.setErrorCode(UPNP_E_INVALID_ACTION);
    }

    log_debug("end");
}

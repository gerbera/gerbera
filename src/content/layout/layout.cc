/*GRB*

    Gerbera - https://gerbera.io/

    layout.cc - this file is part of Gerbera.
    Copyright (C) 2023-2024 Gerbera Contributors

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

/// \file layout.cc
#define GRB_LOG_FAC GrbLogFacility::layout

#include "layout.h" // API

#include "cds/cds_item.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "upnp/upnp_common.h"
#include "util/tools.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif

#include <algorithm>

void Layout::processCdsObject(const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsContainer>& parent,
    const fs::path& rootpath,
    const std::string& contentType,
    const std::map<AutoscanMediaMode, std::string>& containerMap,
    std::vector<int>& refObjects)
{
    log_debug("Process CDS Object: {}", obj->getTitle());
    auto clone = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(clone);
    clone->setVirtual(true);

    std::vector<int> resObjects;
    switch (obj->getMediaType(contentType)) {
#ifdef ONLINE_SERVICES
    case ObjectType::OnlineService:
        resObjects = addOnlineItem(clone, OnlineServiceType(std::stoi(clone->getAuxData(ONLINE_SERVICE_AUX_ID))), rootpath, containerMap);
        break;
#endif
    case ObjectType::Video:
        resObjects = addVideo(clone, parent, rootpath, containerMap);
        break;
    case ObjectType::Image:
        resObjects = addImage(clone, parent, rootpath, containerMap);
        break;
    case ObjectType::Audio:
        resObjects = addAudio(clone, parent, rootpath, containerMap);
        break;
    case ObjectType::Playlist:
        log_debug("Playlist not handled in layout: {}", obj->getLocation().c_str());
        return;
    default:
        log_warning("Unknown media type {}", obj->getClass());
        break;
    }

    cleanUp(refObjects, resObjects);
}

void Layout::cleanUp(const std::vector<int>& refObjects, const std::vector<int>& resObjects)
{
    // compare ref'd objects
    for (auto&& origId : refObjects) {
        auto newEntry = std::find_if(resObjects.begin(), resObjects.end(), [&](const auto& entry) {
            return origId == entry;
        });
        if (newEntry == resObjects.end()) {
            log_debug("Deleting orphaned virtual item {}", origId);
            content->getContext()->getDatabase()->removeObject(origId, "", false);
        }
    }
}

/*GRB*

    Gerbera - https://gerbera.io/

    layout.cc - this file is part of Gerbera.
    Copyright (C) 2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::layout

#include <map>

#include "layout.h" // API

#include "cds/cds_item.h"
#include "upnp_common.h"
#include "util/tools.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif // ONLINE_SERVICES

void Layout::processCdsObject(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath, const std::string& contentType, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    log_debug("Process CDS Object: {}", obj->getTitle());
    auto clone = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(clone);
    clone->setVirtual(true);

#ifdef ONLINE_SERVICES
    if (clone->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        addTrailer(clone, OnlineServiceType(std::stoi(clone->getAuxData(ONLINE_SERVICE_AUX_ID))), rootpath, containerMap);
    } else {
#endif

        auto objCls = std::static_pointer_cast<CdsItem>(obj)->getClass();
        if (contentType == CONTENT_TYPE_OGG) {
            if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
                addVideo(clone, rootpath, containerMap);
            else
                addAudio(clone, rootpath, containerMap);
        } else if (startswith(objCls, UPNP_CLASS_VIDEO_ITEM)) {
            addVideo(clone, rootpath, containerMap);
        } else if (startswith(objCls, UPNP_CLASS_IMAGE_ITEM)) {
            addImage(clone, rootpath, containerMap);
        } else if (startswith(objCls, UPNP_CLASS_AUDIO_ITEM) && contentType != CONTENT_TYPE_PLAYLIST) {
            addAudio(clone, rootpath, containerMap);
        }

#ifdef ONLINE_SERVICES
    }
#endif
}

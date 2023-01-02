/*GRB*
Gerbera - https://gerbera.io/

    cds_item.cc - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file cds_item.cc

#include "cds_item.h" // API

#include <array>
#include <numeric>

#include "upnp_common.h"
#include "util/grb_time.h"
#include "util/upnp_clients.h"

CdsItem::CdsItem()
{
    objectType = OBJECT_TYPE_ITEM;
    upnpClass = "object.item";
    mtime = currentTime();
}

void CdsItem::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!obj->isItem())
        return;
    auto item = std::static_pointer_cast<CdsItem>(obj);
    //    item->setDescription(description);
    item->setMimeType(mimeType);
    item->setTrackNumber(trackNumber);
    item->setPartNumber(partNumber);
    item->setServiceID(serviceID);
    if (playStatus)
        item->setPlayStatus(playStatus->clone());
}

bool CdsItem::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    auto item = std::static_pointer_cast<CdsItem>(obj);
    if (!CdsObject::equals(obj, exactly))
        return false;
    return (mimeType == item->getMimeType() && partNumber == item->getPartNumber() && trackNumber == item->getTrackNumber() && serviceID == item->getServiceID());
}

void CdsItem::validate() const
{
    CdsObject::validate();
    //    log_info("mime: [{}] loc [{}]", this->mimeType.c_str(), this->location.c_str());
    if (this->mimeType.empty())
        throw_std_runtime_error("Item validation failed: missing mimetype");

    if (this->location.empty())
        throw_std_runtime_error("Item validation failed: missing location");

    if (isExternalItem())
        return;

    std::error_code ec;
    if (!isRegularFile(location, ec))
        throw_std_runtime_error("Item validation failed: file {} not found", location.c_str());
}

//---------

CdsItemExternalURL::CdsItemExternalURL()
{
    objectType |= OBJECT_TYPE_ITEM_EXTERNAL_URL;

    upnpClass = UPNP_CLASS_ITEM;
}

void CdsItemExternalURL::validate() const
{
    CdsItem::validate();
    if (this->mimeType.empty())
        throw_std_runtime_error("URL Item validation failed: missing mimetype");

    if (this->location.empty())
        throw_std_runtime_error("URL Item validation failed: missing URL");
}

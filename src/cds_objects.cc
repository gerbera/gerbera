/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_objects.cc - this file is part of MediaTomb.

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

/// \file cds_objects.cc

#include "cds_objects.h" // API

#include <array>
#include <numeric>

#include "database/database.h"
#include "util/tools.h"
#include "util/upnp_clients.h"

static constexpr bool isCdsItem(unsigned int type) { return type & OBJECT_TYPE_ITEM; }
static constexpr bool isCdsPureItem(unsigned int type) { return type == OBJECT_TYPE_ITEM; }

void CdsObject::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    obj->setID(id);
    obj->setRefID(refID);
    obj->setParentID(parentID);
    obj->setTitle(title);
    obj->setClass(upnpClass);
    obj->setLocation(location);
    obj->setMTime(mtime);
    obj->setSizeOnDisk(sizeOnDisk);
    obj->setVirtual(virt);
    obj->setMetaData(metaData);
    obj->setAuxData(auxdata);
    obj->setFlags(objectFlags);
    obj->setSortPriority(sortPriority);
    for (auto&& resource : resources)
        obj->addResource(resource->clone());
}

bool CdsObject::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    if (id != obj->getID()
        || parentID != obj->getParentID()
        || isRestricted() != obj->isRestricted()
        || title != obj->getTitle()
        || upnpClass != obj->getClass()
        || sortPriority != obj->getSortPriority())
        return false;

    if (!resourcesEqual(obj))
        return false;

    if (metaData != obj->getMetaData())
        return false;

    return !exactly
        || (location == obj->getLocation()
            && mtime == obj->getMTime()
            && sizeOnDisk == obj->getSizeOnDisk()
            && virt == obj->isVirtual()
            && std::equal(auxdata.begin(), auxdata.end(), obj->auxdata.begin())
            && objectFlags == obj->getFlags());
}

bool CdsObject::resourcesEqual(const std::shared_ptr<CdsObject>& obj) const
{
    if (resources.size() != obj->resources.size())
        return false;

    // compare all resources
    return std::equal(resources.begin(), resources.end(), obj->resources.begin(),
        [](auto&& r1, auto&& r2) { return r1->equals(r2); });
}

void CdsObject::validate() const
{
    if (this->title.empty())
        throw_std_runtime_error("Object validation failed: missing title");

    if (this->upnpClass.empty())
        throw_std_runtime_error("Object validation failed: missing upnp class");
}

bool CdsObject::isSubClass(const std::string& cls) const
{
    return startswith(upnpClass, cls);
}

std::shared_ptr<CdsObject> CdsObject::createObject(unsigned int objectType)
{
    if (IS_CDS_CONTAINER(objectType)) {
        return std::make_shared<CdsContainer>();
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        return std::make_shared<CdsItemExternalURL>();
    }

    if (isCdsItem(objectType)) {
        return std::make_shared<CdsItem>();
    }

    throw_std_runtime_error("invalid object type: {}", objectType);
}

/* CdsItem */

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
//---------

CdsContainer::CdsContainer()
{
    objectType = OBJECT_TYPE_CONTAINER;
    upnpClass = UPNP_CLASS_CONTAINER;
    mtime = currentTime();
}

void CdsContainer::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    CdsObject::copyTo(obj);
    if (!obj->isContainer())
        return;
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    cont->setUpdateID(updateID);
}
bool CdsContainer::equals(const std::shared_ptr<CdsObject>& obj, bool exactly) const
{
    auto cont = std::static_pointer_cast<CdsContainer>(obj);
    return CdsObject::equals(obj, exactly) && isSearchable() == cont->isSearchable();
}

/*
void CdsContainer::validate() const
{
    CdsObject::validate();
    /// \todo well.. we have to know if a container is a real directory or just a virtual container in the database
      if (!fs::is_directory(this->location, true))
        throw_std_runtime_error("validation failed");
}
*/

std::string_view CdsObject::mapObjectType(unsigned int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (isCdsPureItem(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    throw_std_runtime_error("illegal objectType: {}", type);
}

static constexpr auto upnpFlags = std::array {
    std::pair("Restricted", OBJECT_FLAG_RESTRICTED),
    std::pair("Searchable", OBJECT_FLAG_SEARCHABLE),
    std::pair("UseResourceRef", OBJECT_FLAG_USE_RESOURCE_REF),
    std::pair("PersistentContainer", OBJECT_FLAG_PERSISTENT_CONTAINER),
    std::pair("PlaylistRef", OBJECT_FLAG_PLAYLIST_REF),
    std::pair("ProxyUrl", OBJECT_FLAG_PROXY_URL),
    std::pair("OnlineService", OBJECT_FLAG_ONLINE_SERVICE),
    std::pair("OggTheora", OBJECT_FLAG_OGG_THEORA),
};

std::string CdsObject::mapFlags(int flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [uLabel, uFlag] : upnpFlags) {
        if (flags & uFlag) {
            myFlags.emplace_back(uLabel);
            flags &= ~uFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

int CdsObject::remapFlags(const std::string& flag)
{
    for (auto [uLabel, uFlag] : upnpFlags) {
        if (toLower(uLabel) == toLower(flag)) {
            return uFlag;
        }
    }
    return stoiString(flag, 0, 0);
}

int CdsObject::makeFlag(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | CdsObject::remapFlags(trimString(i)); });
}

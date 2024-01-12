/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_objects.cc - this file is part of MediaTomb.

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

/// \file cds_objects.cc

#include "cds_objects.h" // API

#include <array>
#include <numeric>

#include "cds_container.h"
#include "cds_item.h"
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

/// \brief Query single auxdata value.
std::string CdsObject::getAuxData(const std::string& key) const
{
    return getValueOrDefault(auxdata, key);
}

int CdsObject::makeFlag(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | CdsObject::remapFlags(trimString(i)); });
}

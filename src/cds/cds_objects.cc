/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_objects.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file cds/cds_objects.cc

#define GRB_LOG_FAC GrbLogFacility::content
#include "cds_objects.h" // API

#include "cds_container.h"
#include "cds_item.h"
#include "exceptions.h"
#include "util/tools.h"

#include <array>
#include <numeric>

static constexpr bool isCdsItem(unsigned int type) { return type & OBJECT_TYPE_ITEM; }
static constexpr bool isCdsPureItem(unsigned int type) { return type == OBJECT_TYPE_ITEM; }

void CdsObject::copyTo(const std::shared_ptr<CdsObject>& obj)
{
    obj->setID(id);
    obj->setRefID(refID);
    obj->setParentID(parentID);
    obj->setTitle(title);
    obj->setClass(upnpClass);
    obj->setLocation(location, obj->getEntryType()); // keep the entry type from object creation
    obj->setMTime(mtime);
    obj->setSizeOnDisk(sizeOnDisk);
    obj->setVirtual(virt);
    obj->setMetaData(metaData);
    obj->setAuxData(auxdata);
    obj->setFlags(objectFlags);
    obj->setSortKey(sortKey);
    obj->setSource(source);
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
        || sortKey != obj->getSortKey())
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
            && source == obj->getSource()
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

std::string CdsObject::getAuxData(const std::string& key) const
{
    return getValueOrDefault(auxdata, key);
}

ObjectType CdsObject::getMediaType(const std::string& contentType) const
{
#ifdef ONLINE_SERVICES
    if (hasFlag(ObjectFlag::OnlineService) && isExternalItem())
        return ObjectType::OnlineService;
#endif
    if (contentType == CONTENT_TYPE_OGG) {
        return (hasFlag(ObjectFlag::OggTheora)) ? ObjectType::Video : ObjectType::Audio;
    }
    if (contentType == CONTENT_TYPE_PLAYLIST)
        return ObjectType::Playlist;
    if (contentType == CONTENT_TYPE_CUESHEET)
        return ObjectType::Cuesheet;
    if (isSubClass(UPNP_CLASS_CONTAINER))
        return ObjectType::Folder;
    if (isSubClass(UPNP_CLASS_VIDEO_ITEM))
        return ObjectType::Video;
    if (isSubClass(UPNP_CLASS_AUDIO_ITEM))
        return ObjectType::Audio;
    if (isSubClass(UPNP_CLASS_IMAGE_ITEM))
        return ObjectType::Image;
    return ObjectType::Unknown;
}

std::shared_ptr<CdsObject> CdsObject::createObject(unsigned int objectType)
{
    if (IS_CDS_CONTAINER(objectType)) {
        return std::make_shared<CdsContainer>(CdsEntryType::VirtualContainer);
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        return std::make_shared<CdsItemExternalURL>();
    }

    if (isCdsItem(objectType)) {
        return std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
    }

    if (objectType == 0) {
        return std::make_shared<CdsObject>(CdsEntryType::Root);
    }

    throw_std_runtime_error("invalid object type: {}", objectType);
}

std::shared_ptr<CdsObject> CdsObject::createObject(CdsEntryType entryType)
{
    if (entryType == CdsEntryType::Directory || entryType == CdsEntryType::VirtualContainer || entryType == CdsEntryType::DynamicFolder || entryType == CdsEntryType::ExtraDirectory) {
        return std::make_shared<CdsContainer>(entryType);
    }

    if (entryType == CdsEntryType::ExternalUrl) {
        return std::make_shared<CdsItemExternalURL>();
    }

    if (entryType == CdsEntryType::File || entryType == CdsEntryType::VirtualItem || entryType == CdsEntryType::ExtraFile) {
        return std::make_shared<CdsItem>(entryType);
    }

    if (entryType == CdsEntryType::Root) {
        return std::make_shared<CdsObject>(entryType);
    }

    throw_std_runtime_error("invalid entry type: {}", mapEntryType(entryType));
}

static const auto entryTypes = std::map<CdsEntryType, std::string> {
    { CdsEntryType::Unset, "Unset" },
    { CdsEntryType::Root, "Root" },
    { CdsEntryType::Directory, "Directory" },
    { CdsEntryType::File, "File" },
    { CdsEntryType::VirtualContainer, "Container" },
    { CdsEntryType::VirtualItem, "Item" },
    { CdsEntryType::ExternalUrl, "ExternalUrl" },
    { CdsEntryType::DynamicFolder, "DynamicFolder" },
    { CdsEntryType::ExtraFile, "ExtraFile" },
    { CdsEntryType::ExtraDirectory, "ExtraDirectory" },
};

std::string CdsObject::mapEntryType(CdsEntryType type)
{
    return entryTypes.at(type);
}

CdsEntryType CdsObject::remapEntryType(const std::string& type)
{
    for (auto [tp, uLabel] : entryTypes) {
        if (toLower(uLabel) == toLower(type)) {
            return tp;
        }
    }
    return CdsEntryType::Unset;
}

std::string CdsObject::mapObjectType(unsigned int type)
{
    if (IS_CDS_CONTAINER(type))
        return STRING_OBJECT_TYPE_CONTAINER;
    if (isCdsPureItem(type))
        return STRING_OBJECT_TYPE_ITEM;
    if (IS_CDS_ITEM_EXTERNAL_URL(type))
        return STRING_OBJECT_TYPE_EXTERNAL_URL;
    throw_std_runtime_error("illegal objectType: {}", type);
}

unsigned int CdsObject::remapObjectType(const std::string& type)
{
    if (type == STRING_OBJECT_TYPE_CONTAINER)
        return OBJECT_TYPE_CONTAINER;
    if (type == STRING_OBJECT_TYPE_ITEM)
        return OBJECT_TYPE_ITEM;
    if (type == STRING_OBJECT_TYPE_EXTERNAL_URL)
        return OBJECT_TYPE_ITEM | OBJECT_TYPE_ITEM_EXTERNAL_URL;
    return 0;
}

static constexpr std::array upnpObjectFlags {
    std::pair("None", ObjectFlag::None),
    std::pair("Restricted", ObjectFlag::Restricted),
    std::pair("Searchable", ObjectFlag::Searchable),
    std::pair("UseResourceRef", ObjectFlag::UseResourceReference),
    std::pair("PersistentContainer", ObjectFlag::PersistentContainer),
    std::pair("PlaylistRef", ObjectFlag::PlaylistReference),
    std::pair("ProxyUrl", ObjectFlag::ProxyUrl),
    std::pair("OnlineService", ObjectFlag::OnlineService),
    std::pair("OggTheora", ObjectFlag::OggTheora),
};

std::string CdsObject::mapFlags(unsigned int flags)
{
    if (!flags)
        return upnpObjectFlags.front().first;

    std::vector<std::string> myFlags;

    for (auto [uLabel, bit] : upnpObjectFlags) {
        auto uFlag = 1U << to_underlying(bit);
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

std::string CdsObject::mapFlag(ObjectFlag flag)
{
    for (auto [uLabel, bit] : upnpObjectFlags) {
        if (flag == bit) {
            return uLabel;
        }
    }

    return upnpObjectFlags.front().first;
}

unsigned int CdsObject::getFlag(ObjectFlag flag)
{
    if (flag == ObjectFlag::None)
        return 0;

    return 1U << to_underlying(flag);
}

unsigned int CdsObject::remapFlags(const std::string& flag)
{
    if (toLower(flag) == toLower(upnpObjectFlags.front().first))
        return 0;

    for (auto [uLabel, bit] : upnpObjectFlags) {
        if (toLower(uLabel) == toLower(flag)) {
            return 1U << to_underlying(bit);
        }
    }
    return stoulString(flag, 0, 0);
}

unsigned int CdsObject::makeFlag(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|');
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0U, [](auto flg, auto&& i) { return flg | CdsObject::remapFlags(trimString(i)); });
}

static const auto sourceNames = std::map<ObjectSource, const char*> {
    { ObjectSource::Import, "Import" },
    { ObjectSource::ImportModified, "Modified" },
    { ObjectSource::User, "User" },
};

std::string CdsObject::mapSource(ObjectSource source)
{
    return sourceNames.at(source);
}

ObjectSource CdsObject::remapSource(const std::string& source)
{
    for (auto [src, uLabel] : sourceNames) {
        if (toLower(uLabel) == toLower(source)) {
            return src;
        }
    }
    return ObjectSource::User;
}

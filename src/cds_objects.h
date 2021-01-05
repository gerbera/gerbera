/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_objects.h - this file is part of MediaTomb.
    
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

/// \file cds_objects.h
/// \brief Definition for the CdsObject, CdsItem and CdsContainer classes.
#ifndef __CDS_OBJECTS_H__
#define __CDS_OBJECTS_H__

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>
namespace fs = std::filesystem;

#include "cds_resource.h"
#include "common.h"
#include "metadata/metadata_handler.h"
#include "util/tools.h"

// ATTENTION: These values need to be changed in web/js/items.js too.
#define OBJECT_TYPE_CONTAINER 0x00000001u
#define OBJECT_TYPE_ITEM 0x00000002u
#define OBJECT_TYPE_ITEM_EXTERNAL_URL 0x00000008u

#define STRING_OBJECT_TYPE_CONTAINER "container"
#define STRING_OBJECT_TYPE_ITEM "item"
#define STRING_OBJECT_TYPE_EXTERNAL_URL "external_url"

static constexpr bool IS_CDS_CONTAINER(unsigned int type)
{
    return type & OBJECT_TYPE_CONTAINER;
};
static constexpr bool IS_CDS_ITEM_EXTERNAL_URL(unsigned int type) { return type & OBJECT_TYPE_ITEM_EXTERNAL_URL; };

#define OBJECT_FLAG_RESTRICTED 0x00000001u
#define OBJECT_FLAG_SEARCHABLE 0x00000002u
#define OBJECT_FLAG_USE_RESOURCE_REF 0x00000004u
#define OBJECT_FLAG_PERSISTENT_CONTAINER 0x00000008u
#define OBJECT_FLAG_PLAYLIST_REF 0x00000010u
#define OBJECT_FLAG_PROXY_URL 0x00000020u
#define OBJECT_FLAG_ONLINE_SERVICE 0x00000040u
#define OBJECT_FLAG_OGG_THEORA 0x00000080u
#define OBJECT_FLAG_PLAYED 0x00000200u

#define OBJECT_AUTOSCAN_NONE 0u
#define OBJECT_AUTOSCAN_UI 1u
#define OBJECT_AUTOSCAN_CFG 2u

/// \brief Generic object in the Content Directory.
class CdsObject {
protected:
    /// \brief ID of the object in the content directory
    int id;

    /// \brief ID of the referenced object
    int refID;

    /// \brief ID of the object's parent
    int parentID;

    /// \brief dc:title
    std::string title;

    /// \brief upnp:class
    std::string upnpClass;

    /// \brief Physical location of the media.
    fs::path location;

    /// \brief Last modification time in the file system.
    /// In seconds since UNIX epoch.
    time_t mtime;

    /// \brief File size on disk (in bytes).
    off_t sizeOnDisk;

    /// \brief virtual object flag
    bool virt;

    /// \brief type of the object: item, container, etc.
    unsigned int objectType;

    /// \brief field which can hold various flags for the object
    unsigned int objectFlags;

    /// \brief flag that allows to sort objects within a container
    int sortPriority;

    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> auxdata;
    std::vector<std::shared_ptr<CdsResource>> resources;

    virtual ~CdsObject() = default;

public:
    /// \brief Constructor. Sets the default values.
    explicit CdsObject();

    /// \brief Set the object ID.
    ///
    /// ID is the object ID that is used by the UPnP Content Directory service.
    void setID(int id) { this->id = id; }

    /// \brief Retrieve the object ID.
    ///
    /// ID is the object ID that is used by the UPnP Content Directory service.
    int getID() const { return id; }

    /// \brief Set the reference object ID.
    ///
    /// This is the reference ID that is used by the UPnP Content Directory service.
    /// It also links the reference and the original objects in the database.
    void setRefID(int refID) { this->refID = refID; }

    /// \brief Retrieve the reference object ID.
    ///
    /// This is the reference ID that is used by the UPnP Content Directory service.
    /// It also links the reference and the original objects in the database.
    int getRefID() const { return refID; }

    /// \brief Set the parent ID of the object.
    void setParentID(int parentID) { this->parentID = parentID; }

    /// \brief Retrieve the objects parent ID.
    int getParentID() const { return parentID; }

    /// \brief Set the restricted flag.
    void setRestricted(bool restricted) { changeFlag(OBJECT_FLAG_RESTRICTED, restricted); }

    /// \brief Query the restricted flag.
    bool isRestricted() const { return getFlag(OBJECT_FLAG_RESTRICTED); }

    /// \brief Set the object title (dc:title)
    void setTitle(const std::string& title) { this->title = title; }

    /// \brief Retrieve the title.
    std::string getTitle() const { return title; }

    /// \brief set the upnp:class
    void setClass(const std::string& upnpClass) { this->upnpClass = upnpClass; }

    /// \brief Retrieve class
    std::string getClass() const { return upnpClass; }

    /// \brief Set the physical location of the media (usually an absolute path)
    void setLocation(fs::path location) { this->location = std::move(location); }

    /// \brief Retrieve media location.
    fs::path getLocation() const { return location; }

    /// \brief Set modification time of the media file.
    void setMTime(time_t mtime) { this->mtime = mtime; }

    /// \brief Retrieve the file modification time (in seconds since UNIX epoch).
    time_t getMTime() const { return mtime; }

    /// \brief Set file size.
    void setSizeOnDisk(off_t sizeOnDisk) { this->sizeOnDisk = sizeOnDisk; }

    /// \brief Retrieve the file size (in bytes).
    off_t getSizeOnDisk() const { return sizeOnDisk; }

    /// \brief Set the virtual flag.
    void setVirtual(bool virt) { this->virt = virt; }

    /// \brief Query the virtual flag.
    bool isVirtual() const { return virt; }

    /// \brief Query information on object type: item, container, etc.
    unsigned int getObjectType() const { return objectType; }
    virtual bool isItem() const { return false; }
    virtual bool isPureItem() const { return false; }
    virtual bool isExternalItem() const { return false; }
    virtual bool isContainer() const { return false; }

    /// \brief Retrieve sort priority setting.
    int getSortPriority() const { return sortPriority; }

    /// \brief Set the sort priority of an object.
    void setSortPriority(int sortPriority) { this->sortPriority = sortPriority; }

    /// \brief Get flags of an object.
    unsigned int getFlags() const { return objectFlags; }

    /// \brief Get a flag of an object.
    unsigned int getFlag(unsigned int mask) const { return objectFlags & mask; }

    /// \brief Set flags for the object.
    void setFlags(unsigned int objectFlags) { this->objectFlags = objectFlags; }

    /// \biref Set a flag of the object.
    void setFlag(unsigned int mask) { objectFlags |= mask; }

    /// \biref Set a flag of the object.
    void changeFlag(unsigned int mask, bool value)
    {
        if (value)
            setFlag(mask);
        else
            clearFlag(mask);
    }

    /// \biref Clears a flag of the object.
    void clearFlag(unsigned int mask) { objectFlags &= ~mask; }

    /// \brief Query single metadata value.
    std::string getMetadata(const metadata_fields_t key) const
    {
        return getValueOrDefault(metadata, MetadataHandler::getMetaFieldName(key));
    }

    /// \brief Query entire metadata dictionary.
    std::map<std::string, std::string> getMetadata() const { return metadata; }

    /// \brief Set entire metadata dictionary.
    void setMetadata(const std::map<std::string, std::string>& metadata)
    {
        this->metadata = metadata;
    }

    /// \brief Set a single metadata value.
    void setMetadata(const metadata_fields_t key, const std::string& value)
    {
        metadata[MetadataHandler::getMetaFieldName(key)] = value;
    }

    /// \brief Removes metadata with the given key
    void removeMetadata(const metadata_fields_t key)
    {
        metadata.erase(MetadataHandler::getMetaFieldName(key));
    }

    /// \brief Query single auxdata value.
    std::string getAuxData(const std::string& key) const
    {
        return getValueOrDefault(auxdata, key);
    }

    /// \brief Query entire auxdata dictionary.
    std::map<std::string, std::string> getAuxData() const { return auxdata; }

    /// \brief Set a single auxdata value.
    void setAuxData(const std::string& key, const std::string& value)
    {
        auxdata[key] = value;
    }

    /// \brief Set entire auxdata dictionary.
    void setAuxData(const std::map<std::string, std::string>& auxdata)
    {
        this->auxdata = auxdata;
    }

    /// \brief Removes auxdata with the given key
    void removeAuxData(const std::string& key)
    {
        auxdata.erase(key);
    }

    /// \brief Get number of resource tags
    size_t getResourceCount() const { return resources.size(); }

    /// \brief Query resources
    std::vector<std::shared_ptr<CdsResource>> getResources() const
    {
        return resources;
    }

    /// \brief Set resources
    void setResources(const std::vector<std::shared_ptr<CdsResource>>& res)
    {
        resources = res;
    }

    /// \brief Search resources for given handler id
    bool hasResource(int id) const
    {
        return std::any_of(resources.begin(), resources.end(), [=](const auto& res) { return id == res->getHandlerType(); });
    }

    /// \brief Remove resource with given handler id
    void removeResource(int id)
    {
        auto index = std::find_if(resources.begin(), resources.end(), [=](const auto& res) { return id == res->getHandlerType(); });
        if (index != resources.end()) {
            resources.erase(index);
        }
    }

    /// \brief Query resource tag with the given index
    std::shared_ptr<CdsResource> getResource(size_t index) const
    {
        return resources.at(index);
    }

    /// \brief Add resource tag
    void addResource(const std::shared_ptr<CdsResource>& resource)
    {
        resources.push_back(resource);
    }

    /// \brief Insert resource tag at index
    void insertResource(size_t index, const std::shared_ptr<CdsResource>& resource)
    {
        resources.insert(resources.begin() + index, resource);
    }

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(const std::shared_ptr<CdsObject>& obj);

    /// \brief Checks if current object is equal to obj.
    /// \param obj object to check against
    /// \param exactly tells to check really everything or only the "public" version
    ///
    /// The difference between setting this flag to true or false is following:
    /// exactly=true checks all fields, also internal ones, exactly=false checks
    /// only the fields that will be visible in DIDL-Lite
    virtual int equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false);

    /// \brief Checks if current object has the same resources as obj
    /// \param obj object to check against
    int resourcesEqual(const std::shared_ptr<CdsObject>& obj);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();

    static std::shared_ptr<CdsObject> createObject(unsigned int objectType);

    static std::string mapObjectType(unsigned int objectType);
};

/// \brief An Item in the content directory.
class CdsItem : public CdsObject {
protected:
    /// \brief mime-type of the media.
    std::string mimeType;

    int trackNumber;

    /// \brief unique service ID
    std::string serviceID;

public:
    /// \brief Constructor, sets the object type and default upnp:class (object.item)
    explicit CdsItem();

    /// \brief Set mime-type information of the media.
    void setMimeType(const std::string& mimeType) { this->mimeType = mimeType; }

    bool isItem() const override { return true; }
    bool isPureItem() const override { return true; }

    /// \brief Query mime-type information.
    std::string getMimeType() const { return mimeType; }

    /// \brief Sets the upnp:originalTrackNumber property
    void setTrackNumber(int trackNumber) { this->trackNumber = trackNumber; }

    int getTrackNumber() const { return trackNumber; }
    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)`
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    int equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;

    /// \brief Set the unique service ID.
    void setServiceID(const std::string& serviceID) { this->serviceID = serviceID; }

    /// \brief Retrieve the unique service ID.
    std::string getServiceID() const { return serviceID; }
};

/// \brief An item that is accessible via a URL.
class CdsItemExternalURL : public CdsItem {
public:
    /// \brief Constructor, sets the object type.
    explicit CdsItemExternalURL();
    bool isPureItem() const override { return false; }
    bool isExternalItem() const override { return true; }

    /// \brief Sets the URL for the item.
    /// \param URL full url to the item: http://somewhere.com/something.mpg
    void setURL(const std::string& URL) { this->location = URL; }

    /// \brief Get the URL of the item.
    std::string getURL() const { return location; }
    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    //void copyTo(std::shared_ptr<CdsObject> obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    //int equals(std::shared_ptr<CdsObject> obj, bool exactly=false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;
};

/// \brief A container in the content directory.
class CdsContainer final : public CdsObject {
protected:
    /// \brief container update id.
    int updateID;

    /// \brief childCount attribute
    int childCount;

    /// \brief whether this container is an autoscan start point.
    int autoscanType;

public:
    /// \brief Constructor, initializes default values for the flags and sets the object type.
    explicit CdsContainer();
    bool isContainer() const override { return true; }

    /// \brief Set the searchable flag.
    void setSearchable(bool searchable) { changeFlag(OBJECT_FLAG_SEARCHABLE, searchable); }

    /// \brief Query searchable flag.
    int isSearchable() const { return getFlag(OBJECT_FLAG_SEARCHABLE); }

    /// \brief Set the container update ID value.
    void setUpdateID(int updateID) { this->updateID = updateID; }

    /// \brief Query container update ID value.
    int getUpdateID() const { return updateID; }

    /// \brief Set container childCount attribute.
    void setChildCount(int childCount) { this->childCount = childCount; }

    /// \brief Retrieve number of children
    int getChildCount() const { return childCount; }

    /// \brief returns whether this container is an autoscan start point.
    int getAutoscanType() const { return autoscanType; }

    /// \brief sets whether this container is an autoscan start point.
    void setAutoscanType(int type) { autoscanType = type; }

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    int equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;
};

#endif // __CDS_OBJECTS_H__

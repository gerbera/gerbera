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
/// \brief Definition for the CdsObject, CdsItem, CdsActiveItem and CdsContainer classes.
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
#include "util/tools.h"

// forward declaration
class Storage;

// ATTENTION: These values need to be changed in web/js/items.js too.
#define OBJECT_TYPE_CONTAINER 0x00000001u
#define OBJECT_TYPE_ITEM 0x00000002u
#define OBJECT_TYPE_ACTIVE_ITEM 0x00000004u
#define OBJECT_TYPE_ITEM_EXTERNAL_URL 0x00000008u
#define OBJECT_TYPE_ITEM_INTERNAL_URL 0x00000010u

#define STRING_OBJECT_TYPE_CONTAINER "container"
#define STRING_OBJECT_TYPE_ITEM "item"
#define STRING_OBJECT_TYPE_ACTIVE_ITEM "active_item"
#define STRING_OBJECT_TYPE_EXTERNAL_URL "external_url"
#define STRING_OBJECT_TYPE_INTERNAL_URL "internal_url"

#define IS_CDS_CONTAINER(type) (type & OBJECT_TYPE_CONTAINER)
#define IS_CDS_ITEM(type) (type & OBJECT_TYPE_ITEM)
#define IS_CDS_ACTIVE_ITEM(type) (type & OBJECT_TYPE_ACTIVE_ITEM)
#define IS_CDS_ITEM_EXTERNAL_URL(type) (type & OBJECT_TYPE_ITEM_EXTERNAL_URL)
#define IS_CDS_ITEM_INTERNAL_URL(type) (type & OBJECT_TYPE_ITEM_INTERNAL_URL)
#define IS_CDS_PURE_ITEM(type) (type == OBJECT_TYPE_ITEM)

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
    std::shared_ptr<Storage> storage;

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

public:
    /// \brief Constructor. Sets the default values.
    explicit CdsObject(std::shared_ptr<Storage> storage);

    /// \brief Set the object ID.
    ///
    /// ID is the object ID that is used by the UPnP Content Directory service.
    inline void setID(int id) { this->id = id; }

    /// \brief Retrieve the object ID.
    ///
    /// ID is the object ID that is used by the UPnP Content Directory service.
    inline int getID() const { return id; }

    /// \brief Set the reference object ID.
    ///
    /// This is the reference ID that is used by the UPnP Content Directory service.
    /// It also links the reference and the original objects in the database.
    inline void setRefID(int refID) { this->refID = refID; }

    /// \brief Retrieve the reference object ID.
    ///
    /// This is the reference ID that is used by the UPnP Content Directory service.
    /// It also links the reference and the original objects in the database.
    inline int getRefID() const { return refID; }

    /// \brief Set the parent ID of the object.
    inline void setParentID(int parentID) { this->parentID = parentID; }

    /// \brief Retrieve the objects parent ID.
    inline int getParentID() const { return parentID; }

    /// \brief Set the restricted flag.
    inline void setRestricted(bool restricted) { changeFlag(OBJECT_FLAG_RESTRICTED, restricted); }

    /// \brief Query the restricted flag.
    inline bool isRestricted() const { return getFlag(OBJECT_FLAG_RESTRICTED); }

    /// \brief Set the object title (dc:title)
    inline void setTitle(const std::string& title) { this->title = title; }

    /// \brief Retrieve the title.
    inline std::string getTitle() const { return title; }

    /// \brief set the upnp:class
    inline void setClass(const std::string& upnpClass) { this->upnpClass = upnpClass; }

    /// \brief Retrieve class
    inline std::string getClass() const { return upnpClass; }

    /// \brief Set the physical location of the media (usually an absolute path)
    inline void setLocation(fs::path location) { this->location = location; }

    /// \brief Retrieve media location.
    inline fs::path getLocation() const { return location; }

    /// \brief Set modification time of the media file.
    inline void setMTime(time_t mtime) { this->mtime = mtime; }

    /// \brief Retrieve the file modification time (in seconds since UNIX epoch).
    inline time_t getMTime() const { return mtime; }

    /// \brief Set file size.
    inline void setSizeOnDisk(off_t sizeOnDisk) { this->sizeOnDisk = sizeOnDisk; }

    /// \brief Retrieve the file size (in bytes).
    inline off_t getSizeOnDisk() const { return sizeOnDisk; }

    /// \brief Set the virtual flag.
    inline void setVirtual(bool virt) { this->virt = virt; }

    /// \brief Query the virtual flag.
    inline bool isVirtual() const { return virt; }

    /// \brief Query information on object type: item, container, etc.
    inline unsigned int getObjectType() const { return objectType; }

    /// \brief Retrieve sort priority setting.
    inline int getSortPriority() const { return sortPriority; }

    /// \brief Set the sort priority of an object.
    inline void setSortPriority(int sortPriority) { this->sortPriority = sortPriority; }

    /// \brief Get flags of an object.
    inline unsigned int getFlags() const { return objectFlags; }

    /// \brief Get a flag of an object.
    inline unsigned int getFlag(unsigned int mask) const { return objectFlags & mask; }

    /// \brief Set flags for the object.
    inline void setFlags(unsigned int objectFlags) { this->objectFlags = objectFlags; }

    /// \biref Set a flag of the object.
    inline void setFlag(unsigned int mask) { objectFlags |= mask; }

    /// \biref Set a flag of the object.
    inline void changeFlag(unsigned int mask, bool value)
    {
        if (value)
            setFlag(mask);
        else
            clearFlag(mask);
    }

    /// \biref Clears a flag of the object.
    inline void clearFlag(unsigned int mask) { objectFlags &= ~mask; }

    /// \brief Query single metadata value.
    inline std::string getMetadata(const std::string& key) const
    {
        return getValueOrDefault(metadata, key);
    }

    /// \brief Query entire metadata dictionary.
    inline std::map<std::string, std::string> getMetadata() const { return metadata; }

    /// \brief Set entire metadata dictionary.
    inline void setMetadata(const std::map<std::string, std::string>& metadata)
    {
        this->metadata = metadata;
    }

    /// \brief Set a single metadata value.
    inline void setMetadata(const std::string& key, const std::string& value)
    {
        metadata[key] = value;
    }

    /// \brief Removes metadata with the given key
    inline void removeMetadata(std::string key)
    {
        metadata.erase(key);
    }

    /// \brief Query single auxdata value.
    inline std::string getAuxData(const std::string& key) const
    {
        return getValueOrDefault(auxdata, key);
    }

    /// \brief Query entire auxdata dictionary.
    inline std::map<std::string, std::string> getAuxData() const { return auxdata; }

    /// \brief Set a single auxdata value.
    inline void setAuxData(std::string key, const std::string& value)
    {
        auxdata[key] = value;
    }

    /// \brief Set entire auxdata dictionary.
    inline void setAuxData(const std::map<std::string, std::string>& auxdata)
    {
        this->auxdata = auxdata;
    }

    /// \brief Removes auxdata with the given key
    inline void removeAuxData(std::string key)
    {
        auxdata.erase(key);
    }

    /// \brief Get number of resource tags
    inline int getResourceCount() const { return resources.size(); }

    /// \brief Query resources
    inline std::vector<std::shared_ptr<CdsResource>> getResources() const
    {
        return resources;
    }

    /// \brief Set resources
    inline void setResources(const std::vector<std::shared_ptr<CdsResource>>& res)
    {
        resources = res;
    }

    /// \brief Query resource tag with the given index
    inline std::shared_ptr<CdsResource> getResource(size_t index) const
    {
        return resources.at(index);
    }

    /// \brief Add resource tag
    inline void addResource(std::shared_ptr<CdsResource> resource)
    {
        resources.push_back(resource);
    }

    /// \brief Insert resource tag at index
    inline void insertResource(int index, std::shared_ptr<CdsResource> resource)
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

    static std::shared_ptr<CdsObject> createObject(const std::shared_ptr<Storage>& storage, unsigned int objectType);

    /// \brief Returns the path to the object as it appears in the database tree.
    virtual std::string getVirtualPath() const = 0;

    static std::string mapObjectType(int objectType);
    static int remapObjectType(const std::string& objectType);
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
    explicit CdsItem(std::shared_ptr<Storage> storage);

    /// \brief Set mime-type information of the media.
    inline void setMimeType(const std::string& mimeType) { this->mimeType = mimeType; }

    /// \brief Query mime-type information.
    inline std::string getMimeType() const { return mimeType; }

    /// \brief Sets the upnp:originalTrackNumber property
    inline void setTrackNumber(int trackNumber) { this->trackNumber = trackNumber; }

    inline int getTrackNumber() const { return trackNumber; }
    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)`
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    int equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;

    /// \brief Returns the path to the object as it appears in the database tree.
    std::string getVirtualPath() const override;

    /// \brief Set the unique service ID.
    inline void setServiceID(const std::string& serviceID) { this->serviceID = serviceID; }

    /// \brief Retrieve the unique service ID.
    inline std::string getServiceID() const { return serviceID; }
};

/// \brief An Active Item in the content directory.
///
/// An active item is something very special, and it is not defined within UPnP.
/// From the UPnP point of view it is a normal Item, but internally it does
/// a little more.
/// When an ActiveItem is played back (HTTP GET request for it's URL), a
/// script is executed on the server.
/// The script gets an XML representation of the item (actually a DIDL-Lite render)
/// to the standard input, and has to return an appropriate XML to the standard
/// output. The script may change most of the values of the Item. The only
/// protected fields are object ID and parent ID. In case changes have taken
/// place, a container update is issued.
///
/// You could use ActiveItems for a variety of things, implementing
/// "toggle" items (ones that change between "on" and "off" with each activation)
/// or just "trigger" items that do not change visible but trigger events on the
/// server. For example, you could write a script and create an item to
/// shutdown your PC when this item is played.
///
/// We plan to extend the ActiveItem functionality, allowing to control
/// various server settings, and also being more flexible on the time
/// of script execution (execute script before or after serving the media, etc.)
class CdsActiveItem : public CdsItem {
protected:
    /// \brief action to be executed (an absolute path to a script that will process the XML)
    std::string action;

    /// \brief a field where you can save any string you wnat.
    std::string state;

public:
    /// \brief Constructor, sets the object type.
    explicit CdsActiveItem(std::shared_ptr<Storage> storage);

    /// \brief Sets the action for the item.
    /// \param action absolute path to the script that will process the XML data.
    inline void setAction(const std::string& action) { this->action = action; }

    /// \brief Get the path of the action script.
    inline std::string getAction() const { return action; }

    /// \brief Set action state.
    /// \param state any string you want.
    ///
    /// This is quite useful to let the script identify what state the item is in.
    /// Think of it as a cookie (did I already mention that I hate web cookies?)
    inline void setState(const std::string& state) { this->state = state; }

    /// \brief Retrieve the item state.
    inline std::string getState() const { return state; }

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

/// \brief An item that is accessible via a URL.
class CdsItemExternalURL : public CdsItem {
public:
    /// \brief Constructor, sets the object type.
    explicit CdsItemExternalURL(std::shared_ptr<Storage> storage);

    /// \brief Sets the URL for the item.
    /// \param URL full url to the item: http://somewhere.com/something.mpg
    inline void setURL(const std::string& URL) { this->location = URL; }

    /// \brief Get the URL of the item.
    inline std::string getURL() const { return location; }
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

/// \brief An item that is pointing to a file located in the "servedir"
/// directory.
///
/// This implementation will alow to easily launch Java games on the
/// Streamium media renderer. Why "internal URL"? The port of the server
/// can change upon restarts, I have seen that the SDK often binds to
/// a new port (no matter what is configured). The location of an
/// internal URL will be specified as /mystuff/myfile.txt and will
/// resolve to http://serverip:serverport/content/serve/mystuff/myfile.txt
class CdsItemInternalURL : public CdsItemExternalURL {
public:
    /// \brief Constructor, sets the object type.
    explicit CdsItemInternalURL(std::shared_ptr<Storage> storage);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;
};

/*
/// \brief A Dynamic Item in the content directory.
///
/// A Dynamic Item differs from a regular Item in the way that
/// it's content is entirely dynamic and is being built on the fly
/// during request
class CdsDynamicItem : public CdsItem
{
protected:
    /// \brief action to be executed (an absolute path to a script that will process the XML)
    std::string ;

    /// \brief a field where you can save any string you wnat.
    std::string properties;
public:

    /// \brief Constructor, sets the object type.
    CdsActiveItem();

    /// \brief Sets the action for the item.
    /// \param action absolute path to the script that will process the XML data.
    void setAction(std::string action);

    /// \brief Get the path of the action script.
    std::string getAction();

    /// \brief Set action state.
    /// \param state any string you want.
    ///
    /// This is quite useful to let the script identify what state the item is in.
    /// Think of it as a cookie (did I already mention that I hate web cookies?)
    void setState(std::string state);

    /// \brief Retrieve the item state.
    std::string getState();

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    void copyTo(std::shared_ptr<CdsObject> obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    int equals(std::shared_ptr<CdsObject> obj, bool exactly=false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate();
};

*/

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
    explicit CdsContainer(std::shared_ptr<Storage> storage);

    /// \brief Set the searchable flag.
    inline void setSearchable(bool searchable) { changeFlag(OBJECT_FLAG_SEARCHABLE, searchable); }

    /// \brief Query searchable flag.
    inline int isSearchable() const { return getFlag(OBJECT_FLAG_SEARCHABLE); }

    /// \brief Set the container update ID value.
    inline void setUpdateID(int updateID) { this->updateID = updateID; }

    /// \brief Query container update ID value.
    inline int getUpdateID() const { return updateID; }

    /// \brief Set container childCount attribute.
    inline void setChildCount(int childCount) { this->childCount = childCount; }

    /// \brief Retrieve number of children
    inline int getChildCount() const { return childCount; }

    /// \brief returns whether this container is an autoscan start point.
    inline int getAutoscanType() const { return autoscanType; }

    /// \brief sets whether this container is an autoscan start point.
    inline void setAutoscanType(int type) { autoscanType = type; }

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    int equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) override;

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() override;

    /// \brief Returns the path to the object as it appears in the database tree.
    std::string getVirtualPath() const override;
};

#endif // __CDS_OBJECTS_H__

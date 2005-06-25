/*  cds_objects.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file cds_objects.h
/// \brief Definition for the CdsObject, CdsItem, CdsActiveItem and CdsContainer classes.
#ifndef __CDS_OBJECTS_H__
#define __CDS_OBJECTS_H__

#include "common.h"
#include "dictionary.h"

#define OBJECT_TYPE_CONTAINER 1
#define OBJECT_TYPE_ITEM 2
#define OBJECT_TYPE_ACTIVE_ITEM 4
#define OBJECT_TYPE_ITEM_EXTERNAL_URL 8

#define IS_CDS_CONTAINER(type) (type & OBJECT_TYPE_CONTAINER)
#define IS_CDS_ITEM(type) (type & OBJECT_TYPE_ITEM)
#define IS_CDS_ACTIVE_ITEM(type) (type & OBJECT_TYPE_ACTIVE_ITEM)
#define IS_CDS_ITEM_EXTERNAL_URL(type) (type & OBJECT_TYPE_ITEM_EXTERNAL_URL)

int CdsObjectTitleComparator(void *arg1, void *arg2);

/// \brief Generic object in the Content Directory.
class CdsObject : public zmm::Object
{
protected:
    /// \brief ID of the object in the content directory
    zmm::String id;

    /// \brief ID of the referenced object
    zmm::String ref_id;

    /// \brief ID of the object's parent
    zmm::String parentID;

    /// \brief restricted flag
    int restricted;

    /// \brief dc:title
    zmm::String title;

    /// \brief upnp:class
    zmm::String upnp_class;

    /// \brief Physical location of the media.
    zmm::String location;

    /// \brief virtual object flag
    int virt;

    /// \brief type of the object: item, container, etc.
    int objectType;

    zmm::Ref<Dictionary> metadata;
    zmm::Ref<Dictionary> auxdata;
    zmm::Ref<zmm::Array<Dictionary> > resources;

public:
    /// \brief Constructor, currently only sets the restricted flag to 1
    CdsObject();

    /// \brief Set the object ID.
    void setID(zmm::String id);

    /// \brief Retrieve the object ID.
    zmm::String getID();

    /// \brief Set the object ID.
    void setRefID(zmm::String ref_id);

    /// \brief Retrieve the object ID.
    zmm::String getRefID();

    /// \brief Set the parent ID of the object.
    void setParentID(zmm::String parentID);

    /// \brief Retrieve the objects parent ID.
    zmm::String getParentID();

    /// \brief Set the restricted flag.
    void setRestricted(int restricted);

    /// \brief Query the restricted flag.
    int isRestricted();

    /// \brief Set the object title (dc:title)
    void setTitle(zmm::String title);

    /// \brief Retrieve the title.
    zmm::String getTitle();

    /// \brief set the upnp:class
    void setClass(zmm::String upnp_class);

    /// \brief Retrieve class
    zmm::String getClass();

    /// \brief Set the physical location of the media (usually an absolute path)
    void setLocation(zmm::String location);

    /// \brief Retrieve media location.
    zmm::String getLocation();

    /// \brief Set the virtual flag.
    void setVirtual(int virt);

    /// \brief Query the virtual flag.
    int isVirtual();

    /// \brief Query information on object type: item, container, etc.
    int getObjectType();

    /// \brief Query single metadata value.
    zmm::String getMetadata(zmm::String key);

    /// \brief Query entire metadata dictionary.
    zmm::Ref<Dictionary> getMetadata();

    /// \brief Set a single metadata value.
    void setMetadata(zmm::String key, zmm::String value);

    /// \brief Removes metadata with the given key
    void removeMetadata(zmm::String key);
    
    /// \brief Set entire metadata dictionary.
    void setMetadata(zmm::Ref<Dictionary> metadata);



    /// \brief Query single auxdata value.
    zmm::String getAuxData(zmm::String key);

    /// \brief Query entire auxdata dictionary.
    zmm::Ref<Dictionary> getAuxData();

    /// \brief Set a single auxdata value.
    void setAuxData(zmm::String key, zmm::String value);

    /// \brief Removes auxdata with the given key
    void removeAuxData(zmm::String key);
    
    /// \brief Set entire auxdata dictionary.
    void setAuxData(zmm::Ref<Dictionary> auxdata);


    /// \brief Get number of resource tags
    int getResourceCount();
    /// \brief Query resource tag with the given index
    zmm::Ref<Dictionary> getResource(int index);
    /// \brief Query single resource tag key
    zmm::String getResource(int index, zmm::String key);
    /// \brief Set resource tag with the given index
    void setResource(int index, zmm::Ref<Dictionary> resource);
    /// \brief Set resource tag key with the given index and key
    void setResource(int index, zmm::String key, zmm::String value);
    /// \brief Add resource tag
    void addResource(zmm::Ref<Dictionary> resource);
    /// \brief Add empty resource tag
    void addResource();
    

    
    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    /// \param obj object to check against
    /// \param exactly tells to check really everything or only the "public" version
    ///
    /// The difference between setting this flag to true or false is following:
    /// exactly=true checks all fields, also internal ones, exactly=false checks
    /// only the fields that will be visible in DIDL-Lite
    virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();

    static zmm::Ref<CdsObject> createObject(int objectType);

	friend int CdsObjectTitleComparator(void *arg1, void *arg2);
};

/// \brief An Item in the content directory.
class CdsItem : public CdsObject
{
protected:
    /// \brief mime-type of the media.
    zmm::String mimeType;

public:
    /// \brief Constructor, sets the object type and default upnp:class (object.item)
    CdsItem();

    /// \brief Set mime-type information of the media.
    void setMimeType(zmm::String mimeType);

    /// \brief Query mime-type information.
    zmm::String getMimeType();

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();
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
class CdsActiveItem : public CdsItem
{
protected:
    /// \brief action to be executed (an absolute path to a script that will process the XML)
    zmm::String action;

    /// \brief a field where you can save any string you wnat.
    zmm::String state;
public:

    /// \brief Constructor, sets the object type.
    CdsActiveItem();

    /// \brief Sets the action for the item.
    /// \param action absolute path to the script that will process the XML data.
    void setAction(zmm::String action);

    /// \brief Get the path of the action script.
    zmm::String getAction();

    /// \brief Set action state.
    /// \param state any string you want.
    ///
    /// This is quite useful to let the script identify what state the item is in.
    /// Think of it as a cookie (did I already mention that I hate web cookies?)
    void setState(zmm::String state);

    /// \brief Retrieve the item state.
    zmm::String getState();

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();
};

class CdsItemExternalURL : public CdsItem
{
public:

    /// \brief Constructor, sets the object type.
    CdsItemExternalURL();

    /// \brief Sets the URL for the item.
    /// \param URL full url to the item: http://somewhere.com/something.mpg
    void setURL(zmm::String URL);

    /// \brief Get the path of the action script.
    zmm::String getURL();
    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    //virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    //virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();
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
    zmm::String ;

    /// \brief a field where you can save any string you wnat.
    zmm::String properties;
public:

    /// \brief Constructor, sets the object type.
    CdsActiveItem();

    /// \brief Sets the action for the item.
    /// \param action absolute path to the script that will process the XML data.
    void setAction(zmm::String action);

    /// \brief Get the path of the action script.
    zmm::String getAction();

    /// \brief Set action state.
    /// \param state any string you want.
    ///
    /// This is quite useful to let the script identify what state the item is in.
    /// Think of it as a cookie (did I already mention that I hate web cookies?)
    void setState(zmm::String state);

    /// \brief Retrieve the item state.
    zmm::String getState();

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();
};

*/

/// \brief A container in the content directory.
class CdsContainer : public CdsObject
{
protected:
    /// \brief searchable flag.
    int searchable;

    /// \brief container update id.
    int updateID;

    /// \brief childCount attribute
    int childCount;

public:
    /// \brief Constructor, initializes default values for the flags and sets the object type.
    CdsContainer();

    /// \brief Set the searchable flag.
    void setSearchable(int searchable);

    /// \brief Query searchable flag.
    int isSearchable();

    /// \brief Set the container update ID value.
    void setUpdateID(int updateID);

    /// \brief Query container update ID value.
    int getUpdateID();

    /// \brief Set container childCount attribute.
    void setChildCount(int childCount);

    /// \briefe Retrieve number of children
    int getChildCount();

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    virtual void copyTo(zmm::Ref<CdsObject> obj);

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    virtual int equals(zmm::Ref<CdsObject> obj, bool exactly=false);

    /// \brief Checks if the minimum required parameters for the object have been set and are valid.
    virtual void validate();
};

#endif // __CDS_OBJECTS_H__


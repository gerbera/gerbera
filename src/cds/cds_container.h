/*GRB*
Gerbera - https://gerbera.io/

    cds_container.h - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file cds_container.h
/// \brief Definition for the CdsContainer class.
#ifndef __CDS_CONTAINER_H__
#define __CDS_CONTAINER_H__

#include "cds_objects.h"
#include "upnp/upnp_common.h"

/// \brief A container in the content directory.
class CdsContainer final : public CdsObject {
protected:
    /// \brief container update id.
    int updateID {};

    /// \brief childCount attribute
    int childCount { -1 };

    /// \brief whether this container is an autoscan start point.
    AutoscanType autoscanType { AutoscanType::None };

public:
    /// \brief Constructor, initializes default values for the flags and sets the object type.
    CdsContainer();
    explicit CdsContainer(const std::string& title, const std::string& upnpClass = UPNP_CLASS_CONTAINER)
    {
        this->title = title;
        this->upnpClass = upnpClass;
    }

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
    AutoscanType getAutoscanType() const { return autoscanType; }

    /// \brief sets whether this container is an autoscan start point.
    void setAutoscanType(AutoscanType type) { autoscanType = type; }

    /// \brief Copies all object properties to another object.
    /// \param obj target object (clone)
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// \brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    bool equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) const override;

    void setUpnpShortcut(const std::string& upnpShortcut) { addMetaData(MetadataFields::M_UPNP_SHORTCUT, upnpShortcut); }
    std::string getUpnpShortcut() const { return getMetaData(MetadataFields::M_UPNP_SHORTCUT); }
};

#endif // __CDS_CONTAINER_H__

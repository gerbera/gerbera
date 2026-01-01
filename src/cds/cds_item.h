/*GRB*
Gerbera - https://gerbera.io/

    cds_item.h - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file cds/cds_item.h
/// @brief Definition for the CdsItem classes.
#ifndef __CDS_ITEM_H__
#define __CDS_ITEM_H__

#include "cds_objects.h"

class ClientStatusDetail;

/// @brief An Item in the content directory.
class CdsItem : public CdsObject {
protected:
    /// @brief mime-type of the media.
    std::string mimeType { MIMETYPE_DEFAULT };

    /// @brief number of part, e.g. disk or season
    int partNumber {};

    /// @brief number of track e.g. track on disk or episode of season
    int trackNumber {};

    /// @brief unique service ID
    std::string serviceID;

    std::shared_ptr<ClientStatusDetail> playStatus;

public:
    /// @brief Constructor, sets the object type and default upnp:class (object.item)
    CdsItem();

    /// @brief Set mime-type information of the media.
    void setMimeType(const std::string& mimeType) { this->mimeType = mimeType; }

    bool isItem() const override { return true; }
    bool isPureItem() const override { return true; }

    /// @brief Query mime-type information.
    std::string getMimeType() const { return mimeType; }

    /// @brief Sets the upnp:originalTrackNumber property
    void setTrackNumber(int trackNumber)
    {
        if (trackNumber >= 0)
            this->trackNumber = trackNumber;
    }

    int getTrackNumber() const { return trackNumber; }

    /// @brief Sets the part number property
    void setPartNumber(int partNumber)
    {
        if (partNumber >= 0)
            this->partNumber = partNumber;
    }

    int getPartNumber() const { return partNumber; }

    /// @brief Copies all object properties to another object.
    /// @param obj target object (clone)`
    void copyTo(const std::shared_ptr<CdsObject>& obj) override;

    /// @brief Checks if current object is equal to obj.
    ///
    /// See description for CdsObject::equals() for details.
    bool equals(const std::shared_ptr<CdsObject>& obj, bool exactly = false) const override;

    /// @brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() const override;

    /// @brief Set the unique service ID.
    void setServiceID(const std::string& serviceID) { this->serviceID = serviceID; }

    /// @brief Retrieve the unique service ID.
    std::string getServiceID() const { return serviceID; }

    /// @brief Retrieve Play Status details
    void setPlayStatus(const std::shared_ptr<ClientStatusDetail>& playStatus) { this->playStatus = playStatus; }

    /// @brief Set Play Status details
    std::shared_ptr<ClientStatusDetail> getPlayStatus() const { return playStatus; }
};

/// @brief An item that is accessible via a URL.
class CdsItemExternalURL : public CdsItem {
public:
    /// @brief Constructor, sets the object type.
    CdsItemExternalURL();
    bool isPureItem() const override { return false; }
    bool isExternalItem() const override { return true; }

    /// @brief Sets the URL for the item.
    /// @param URL full url to the item: http://somewhere.com/something.mpg
    void setURL(const std::string& URL) { this->location = URL; }

#if 0
    /// @brief Copies all object properties to another object.
    /// @param obj target object (clone)
    void copyTo(std::shared_ptr<CdsObject> obj) override;

    /// @brief Checks if current object is equal to obj.
    ///
    /// See description for \fn CdsObject#equals() for details.
    int equals(std::shared_ptr<CdsObject> obj, bool exactly=false) override;
#endif

    /// @brief Checks if the minimum required parameters for the object have been set and are valid.
    void validate() const override;
};

#endif // __CDS_ITEM_H__

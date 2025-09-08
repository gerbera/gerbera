/*GRB*
Gerbera - https://gerbera.io/

    cds_container.cc - this file is part of Gerbera.

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

/// @file cds/cds_container.cc

#define GRB_LOG_FAC GrbLogFacility::content
#include "cds_container.h" // API

#include "util/grb_time.h"

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

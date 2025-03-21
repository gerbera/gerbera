/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/edit_save.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/edit_save.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "content/content.h"
#include "exceptions.h"
#include "util/tools.h"

const std::string_view Web::EditSave::PAGE = "edit_save";

bool Web::EditSave::processPageAction(Json::Value& element, const std::string& action)
{
    std::string objID = param("object_id");
    if (objID.empty())
        throw_std_runtime_error("invalid object id");

    int objectID = std::stoi(objID);
    auto object = content->updateObject(objectID, params);
    if (readResources(object))
        content->updateObject(object, true);

    return true;
}

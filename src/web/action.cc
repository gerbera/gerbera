/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/action.cc - this file is part of MediaTomb.

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

/// \file web/action.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "common.h"
#include "config/config.h"
#include "content/content.h"
#include "exceptions.h"
#include "util/xml_to_json.h"

const std::string Web::Action::PAGE = "action";

void Web::Action::processPageAction(pugi::xml_node& element)
{
    std::string action = param("action");
    if (action.empty())
        throw_std_runtime_error("No action given");
    log_debug("action: {}", action);
}

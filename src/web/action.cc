/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    action.cc - this file is part of MediaTomb.
    
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

/// \file action.cc

#include "pages.h" // API

#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"

web::action::action(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::action::process()
{
    log_debug("action: start");
    check_request();

    std::string action = param("action");
    if (action.empty())
        throw_std_runtime_error("No action given");
    log_debug("action: {}", action.c_str());

    log_debug("action: returning");
}

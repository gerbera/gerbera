/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    pages.cc - this file is part of MediaTomb.
    
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

/// \file pages.cc

#include "pages.h" // API

namespace web {

std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    std::shared_ptr<ContentManager> content,
    const std::string& page)
{
    if (page == "add")
        return std::make_unique<web::add>(move(content));
    if (page == "remove")
        return std::make_unique<web::remove>(move(content));
    if (page == "add_object")
        return std::make_unique<web::addObject>(move(content));
    if (page == "auth")
        return std::make_unique<web::auth>(move(content));
    if (page == "containers")
        return std::make_unique<web::containers>(move(content));
    if (page == "directories")
        return std::make_unique<web::directories>(move(content));
    if (page == "files")
        return std::make_unique<web::files>(move(content));
    if (page == "items")
        return std::make_unique<web::items>(move(content));
    if (page == "edit_load")
        return std::make_unique<web::edit_load>(move(content));
    if (page == "edit_save")
        return std::make_unique<web::edit_save>(move(content));
    if (page == "autoscan")
        return std::make_unique<web::autoscan>(move(content));
    if (page == "void")
        return std::make_unique<web::voidType>(move(content));
    if (page == "tasks")
        return std::make_unique<web::tasks>(move(content));
    if (page == "action")
        return std::make_unique<web::action>(move(content));
    if (page == "clients")
        return std::make_unique<web::clients>(move(content));
    if (page == "config_load")
        return std::make_unique<web::configLoad>(move(content));
    if (page == "config_save")
        return std::make_unique<web::configSave>(move(content));

    throw_std_runtime_error("Unknown page: {}", page.c_str());
}

} // namespace web

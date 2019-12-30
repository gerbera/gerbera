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

#include "pages.h"

namespace web {

WebRequestHandler* createWebRequestHandler(
    std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager,
    std::string page)
{
    if (page == "add")
        return new web::add(config, storage, content, sessionManager);
    if (page == "remove")
        return new web::remove(config, storage, content, sessionManager);
    if (page == "add_object")
        return new web::addObject(config, storage, content, sessionManager);
    if (page == "auth")
        return new web::auth(config, storage, content, sessionManager);
    if (page == "containers")
        return new web::containers(config, storage, content, sessionManager);
    if (page == "directories")
        return new web::directories(config, storage, content, sessionManager);
    if (page == "files")
        return new web::files(config, storage, content, sessionManager);
    if (page == "items")
        return new web::items(config, storage, content, sessionManager);
    if (page == "edit_load")
        return new web::edit_load(config, storage, content, sessionManager);
    if (page == "edit_save")
        return new web::edit_save(config, storage, content, sessionManager);
    if (page == "autoscan")
        return new web::autoscan(config, storage, content, sessionManager);
    if (page == "void")
        return new web::voidType(config, storage, content, sessionManager);
    if (page == "tasks")
        return new web::tasks(config, storage, content, sessionManager);
    if (page == "action")
        return new web::action(config, storage, content, sessionManager);

    throw _Exception("Unknown page: " + page);
}

} // namespace

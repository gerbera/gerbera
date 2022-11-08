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
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

namespace Web {

std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<ContentManager>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::string& page)
{
    if (page == "add")
        return std::make_unique<Web::Add>(content);
    if (page == "remove")
        return std::make_unique<Web::Remove>(content);
    if (page == "add_object")
        return std::make_unique<Web::AddObject>(content);
    if (page == "auth")
        return std::make_unique<Web::Auth>(content);
    if (page == "containers")
        return std::make_unique<Web::Containers>(content, xmlBuilder);
    if (page == "directories")
        return std::make_unique<Web::Directories>(content);
    if (page == "files")
        return std::make_unique<Web::Files>(content);
    if (page == "items")
        return std::make_unique<Web::Items>(content, xmlBuilder);
    if (page == "edit_load")
        return std::make_unique<Web::EditLoad>(content, xmlBuilder);
    if (page == "edit_save")
        return std::make_unique<Web::EditSave>(content);
    if (page == "autoscan")
        return std::make_unique<Web::Autoscan>(content);
    if (page == "void")
        return std::make_unique<Web::VoidType>(content);
    if (page == "tasks")
        return std::make_unique<Web::Tasks>(content);
    if (page == "action")
        return std::make_unique<Web::Action>(content);
    if (page == "clients")
        return std::make_unique<Web::Clients>(content);
    if (page == "config_load")
        return std::make_unique<Web::ConfigLoad>(content);
    if (page == "config_save")
        return std::make_unique<Web::ConfigSave>(context, content);

    throw_std_runtime_error("Unknown page: {}", page);
}

} // namespace Web

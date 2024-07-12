/*MT*

    MediaTomb - http://www.mediatomb.cc/

    pages.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/config_setup.h"
#include "exceptions.h"
#include "util/xml_to_json.h"

namespace Web {

std::unique_ptr<WebRequestHandler> createWebRequestHandler(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::string& page)
{
    if (page == "add")
        return std::make_unique<Web::Add>(content, server);
    if (page == "remove")
        return std::make_unique<Web::Remove>(content, server);
    if (page == "add_object")
        return std::make_unique<Web::AddObject>(content, server);
    if (page == "auth")
        return std::make_unique<Web::Auth>(content, server);
    if (page == "containers")
        return std::make_unique<Web::Containers>(content, xmlBuilder, server);
    if (page == "directories")
        return std::make_unique<Web::Directories>(content, server);
    if (page == "files")
        return std::make_unique<Web::Files>(content, server);
    if (page == "items")
        return std::make_unique<Web::Items>(content, xmlBuilder, server);
    if (page == "edit_load")
        return std::make_unique<Web::EditLoad>(content, xmlBuilder, server);
    if (page == "edit_save")
        return std::make_unique<Web::EditSave>(content, server);
    if (page == "autoscan")
        return std::make_unique<Web::Autoscan>(content, server);
    if (page == "void")
        return std::make_unique<Web::VoidType>(content, server);
    if (page == "tasks")
        return std::make_unique<Web::Tasks>(content, server);
    if (page == "action")
        return std::make_unique<Web::Action>(content, server);
    if (page == "clients")
        return std::make_unique<Web::Clients>(content, server);
    if (page == "config_load")
        return std::make_unique<Web::ConfigLoad>(content, server);
    if (page == "config_save")
        return std::make_unique<Web::ConfigSave>(context, content, server);

    throw_std_runtime_error("Unknown page: {}", page);
}

} // namespace Web

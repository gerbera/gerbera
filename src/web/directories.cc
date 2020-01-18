/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    directories.cc - this file is part of MediaTomb.
    
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

/// \file directories.cc

#include "common.h"
#include "util/filesystem.h"
#include "pages.h"
#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::directories::directories(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::directories::process()
{
    check_request();

    std::string path;
    std::string parentID = param("parent_id");
    if (parentID.empty() || parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(parentID);

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    containers.append_attribute("parent_id") = parentID.c_str();
    if (string_ok(param("select_it")))
        containers.append_attribute("select_it") = param("select_it").c_str();
    containers.append_attribute("type") = "filesystem";

    auto fs = std::make_unique<Filesystem>(config);
    auto arr = fs->readDirectory(path, FS_MASK_DIRECTORIES, FS_MASK_DIRECTORIES);
    for (auto it = arr.begin(); it != arr.end(); it++) {
        auto ce = containers.append_child("container");
        std::string filename = (*it)->filename;
        std::string filepath;
        if (path.c_str()[path.length() - 1] == '/')
            filepath = path + filename;
        else
            filepath = path + '/' + filename;

        /// \todo replace hex_encode with base64_encode?
        std::string id = hex_encode(filepath.c_str(), filepath.length());
        ce.append_attribute("id") = id.c_str();
        if ((*it)->hasContent)
            ce.append_attribute("child_count") = 1;
        else
            ce.append_attribute("child_count") = 0;

        auto f2i = StringConverter::f2i(config);
        ce.append_attribute("title") = f2i->convert(filename).c_str();
    }
}

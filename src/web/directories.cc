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

#include <utility>

#include "common.h"
#include "pages.h"
#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::directories::directories(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
}

void web::directories::process()
{
    check_request();

    fs::path path;
    std::string parentID = param("parent_id");
    if (parentID.empty() || parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(parentID);

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    containers.append_attribute("parent_id") = parentID.c_str();
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();
    containers.append_attribute("type") = "filesystem";

    // don't bother users with system directorties
    std::vector<fs::path> excludes_fullpath = {
        "/bin",
        "/boot",
        "/dev",
        "/etc",
        "/lib", "/lib32", "/lib64", "/libx32",
        "/media",
        "/proc",
        "/run",
        "/sbin",
        "/sys",
        "/tmp",
        "/usr",
        "/var"
    };
    // don't bother users with special or config directorties
    std::vector<std::string> excludes_dirname = {
        "lost+found",
    };
    bool exclude_config_dirs = true;

    for (auto& it : fs::directory_iterator(path)) {
        const fs::path& filepath = it.path();

        if (!it.is_directory())
            continue;
        if (std::find(excludes_fullpath.begin(), excludes_fullpath.end(), filepath) != excludes_fullpath.end())
            continue;
        if (std::find(excludes_dirname.begin(), excludes_dirname.end(), filepath.filename()) != excludes_dirname.end()
            || (exclude_config_dirs && startswith(filepath.filename(), ".")))
            continue;

        bool hasContent = false;
        for (auto& subIt : fs::directory_iterator(path)) {
            if (!subIt.is_directory() && !subIt.is_regular_file())
                continue;
            hasContent = true;
            break;
        }

        /// \todo replace hex_encode with base64_encode?
        std::string id = hex_encode(filepath.c_str(), filepath.string().length());

        auto ce = containers.append_child("container");
        ce.append_attribute("id") = id.c_str();
        if (hasContent)
            ce.append_attribute("child_count") = 1;
        else
            ce.append_attribute("child_count") = 0;

        auto f2i = StringConverter::f2i(config);
        ce.append_attribute("title") = f2i->convert(filepath.filename()).c_str();
    }
}

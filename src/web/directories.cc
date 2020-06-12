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

#include "pages.h" // API

#include <set>
#include <utility>

#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::directories::directories(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
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
    std::set<fs::path> excludes_fullpath = {
        "/bin",
        "/boot",
        "/dev",
        "/etc",
        "/lib", "/lib32", "/lib64", "/libx32",
        "/proc",
        "/run",
        "/sbin",
        "/sys",
        "/tmp",
        "/usr",
        "/var"
    };

    // don't bother users with special or config directorties
    std::set<std::string> excludes_dirname = {
        "lost+found",
    };

    auto f2i = StringConverter::f2i(config);
    // Keep name first for sorting.
    std::vector<std::pair<std::string, fs::path>> result;

    for (const auto& it : fs::directory_iterator(path)) {
        std::error_code ec;
        if (!it.is_directory(ec))
            continue;

        if (excludes_fullpath.count(it.path()))
            continue;

        auto fn = it.path().filename().string();

        if (excludes_dirname.count(fn) || fn.compare(0, 1, ".") == 0)
            continue;

        result.emplace_back(f2i->convert(fn), it.path());
    }

    std::sort(result.begin(), result.end());

    for (const auto& [name, path] : result) {
        bool hasContent = false;
        std::error_code ec;
        for (auto& subIt : fs::directory_iterator(path, ec)) {
            if (!subIt.is_directory(ec) && !isRegularFile(subIt.path(), ec))
                continue;
            hasContent = true;
            break;
        }

        auto ce = containers.append_child("container");
        /// \todo replace hex_encode with base64_encode?
        ce.append_attribute("id") = hex_encode(path.native().c_str(), path.native().length()).c_str();
        ce.append_attribute("child_count") = hasContent;
        ce.append_attribute("title") = name.c_str();
    }
}

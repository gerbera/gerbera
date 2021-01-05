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

#include <utility>

#include "database/database.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::directories::directories(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

struct dirInfo {
    std::string filename;
    bool hasContent;
};

void web::directories::process()
{
    check_request();

    fs::path path;
    std::string parentID = param("parent_id");
    if (parentID.empty() || parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hexDecodeString(parentID);

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    xml2JsonHints->setFieldType("title", "string");
    containers.append_attribute("parent_id") = parentID.c_str();
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();
    containers.append_attribute("type") = "filesystem";

    // don't bother users with system directorties
    constexpr std::array<std::string_view, 15> excludes_fullpath { {
        "/bin",
        "/boot",
        "/dev",
        "/etc",
        "/lib",
        "/lib32",
        "/lib64",
        "/libx32",
        "/proc",
        "/run",
        "/sbin",
        "/sys",
        "/tmp",
        "/usr",
        "/var",
    } };
    // don't bother users with special or config directorties
    constexpr std::array<std::string_view, 1> excludes_dirname = { {
        "lost+found",
    } };
    bool exclude_config_dirs = true;

    std::error_code ec;
    std::map<std::string, struct dirInfo> filesMap;

    for (const auto& it : fs::directory_iterator(path, ec)) {
        const fs::path& filepath = it.path();

        if (!it.is_directory(ec))
            continue;
        if (std::find(excludes_fullpath.begin(), excludes_fullpath.end(), filepath) != excludes_fullpath.end())
            continue;
        if (std::find(excludes_dirname.begin(), excludes_dirname.end(), filepath.filename()) != excludes_dirname.end()
            || (exclude_config_dirs && startswith(filepath.filename(), ".")))
            continue;

        bool hasContent = false;
        for (auto& subIt : fs::directory_iterator(filepath, ec)) {
            if (!subIt.is_directory(ec) && !isRegularFile(subIt.path(), ec))
                continue;
            hasContent = true;
            break;
        }

        /// \todo replace hexEncode with base64_encode?
        std::string id = hexEncode(filepath.c_str(), filepath.string().length());
        filesMap[id] = { filepath.filename(), hasContent };
    }

    auto f2i = StringConverter::f2i(config);
    for (const auto& [key, val] : filesMap) {
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = key.c_str();
        ce.append_attribute("child_count") = val.hasContent;

        ce.append_attribute("title") = f2i->convert(val.filename).c_str();
    }
}

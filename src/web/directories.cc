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

#include "database/database.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::directories::directories(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

using dirInfo = std::pair<fs::path, bool>;

void web::directories::process()
{
    check_request();

    std::string parentID = param("parent_id");
    auto path = fs::path { (parentID.empty() || parentID == "0") ? FS_ROOT_DIRECTORY : hexDecodeString(parentID) };

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    xml2JsonHints->setFieldType("title", "string");
    containers.append_attribute("parent_id") = parentID.c_str();
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();
    containers.append_attribute("type") = "filesystem";

    // don't bother users with system directorties
    auto&& excludes_fullpath = config->getArrayOption(CFG_IMPORT_SYSTEM_DIRECTORIES);
    // don't bother users with special or config directorties
    constexpr auto excludes_dirname = std::array {
        "lost+found",
    };
    bool exclude_config_dirs = true;

    std::error_code ec;
    std::map<std::string, dirInfo> filesMap;

    for (auto&& it : fs::directory_iterator(path, ec)) {
        const fs::path& filepath = it.path();

        if (!it.is_directory(ec))
            continue;
        if (std::find(excludes_fullpath.begin(), excludes_fullpath.end(), filepath) != excludes_fullpath.end())
            continue;
        if (std::find(excludes_dirname.begin(), excludes_dirname.end(), filepath.filename()) != excludes_dirname.end()
            || (exclude_config_dirs && startswith(filepath.filename(), ".")))
            continue;

        auto&& dir = fs::directory_iterator(filepath, ec);
        bool hasContent = std::any_of(begin(dir), end(dir), [&](auto&& sub) { return sub.is_directory(ec) || isRegularFile(sub, ec); });

        /// \todo replace hexEncode with base64_encode?
        std::string id = hexEncode(filepath.c_str(), filepath.string().length());
        filesMap[id] = { filepath.filename(), hasContent };
    }

    auto f2i = StringConverter::f2i(config);
    for (auto&& [key, val] : filesMap) {
        auto&& [file, has] = val;
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = key.c_str();
        ce.append_attribute("child_count") = has;

        ce.append_attribute("title") = f2i->convert(file).c_str();
    }
}

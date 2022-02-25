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

#include "content/content_manager.h"
#include "database/database.h"
#include "util/string_converter.h"
#include "util/tools.h"

using dirInfo = std::pair<fs::path, bool>;

void Web::Directories::process()
{
    checkRequest();

    std::string parentID = param("parent_id");
    auto path = fs::path(parentID.empty() || parentID == "0" ? FS_ROOT_DIRECTORY : hexDecodeString(parentID));

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    xml2JsonHints->setFieldType("title", "string");
    containers.append_attribute("parent_id") = parentID.c_str();
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();
    containers.append_attribute("type") = "filesystem";

    // don't bother users with system directorties
    auto&& excludesFullpath = config->getArrayOption(CFG_IMPORT_SYSTEM_DIRECTORIES);
    auto&& includesFullpath = config->getArrayOption(CFG_IMPORT_VISIBLE_DIRECTORIES);
    // don't bother users with special or config directorties
    constexpr auto excludesDirname = std::array {
        "lost+found",
    };
    bool excludeConfigDirs = true;

    std::error_code ec;
    std::map<std::string, dirInfo> filesMap;
    auto autoscanDirs = content->getAutoscanDirectories();

    for (auto&& it : fs::directory_iterator(path, ec)) {
        const fs::path& filepath = it.path();

        if (!it.is_directory(ec))
            continue;
        if (!includesFullpath.empty()
            && std::none_of(includesFullpath.begin(), includesFullpath.end(), //
                [&](auto&& sub) { return startswith(filepath.string(), sub) || startswith(sub, filepath.string()); }))
            continue; // skip unwanted dir
        if (includesFullpath.empty()) {
            if (std::find(excludesFullpath.begin(), excludesFullpath.end(), filepath) != excludesFullpath.end())
                continue; // skip excluded dir
            if (std::find(excludesDirname.begin(), excludesDirname.end(), filepath.filename()) != excludesDirname.end()
                || (excludeConfigDirs && startswith(filepath.filename().string(), ".")))
                continue; // skip dir with leading .
        }
        auto dir = fs::directory_iterator(filepath, ec);
        bool hasContent = std::any_of(begin(dir), end(dir), [&](auto&& sub) { return sub.is_directory(ec) || isRegularFile(sub, ec); });

        /// \todo replace hexEncode with base64_encode?
        std::string id = hexEncode(filepath.c_str(), filepath.string().length());
        filesMap.emplace(id, std::pair(filepath, hasContent));
    }

    auto f2i = StringConverter::f2i(config);
    for (auto&& [key, val] : filesMap) {
        auto file = val.first;
        auto&& has = val.second;
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = key.c_str();
        ce.append_attribute("child_count") = has;
        auto aDir = std::find_if(autoscanDirs.begin(), autoscanDirs.end(), [&](auto& a) { return file == a->getLocation(); });
        if (aDir != autoscanDirs.end()) {
            ce.append_attribute("autoscan_type") = (*aDir)->isPersistent() ? "isPersistent" : "ui";
            ce.append_attribute("autoscan_mode") = AutoscanDirectory::mapScanMode((*aDir)->getScanMode());
        } else {
            aDir = std::find_if(autoscanDirs.begin(), autoscanDirs.end(), [&](auto& a) { return a->isRecursive() && startswith(file.string(), a->getLocation().string()); });
            if (aDir != autoscanDirs.end()) {
                ce.append_attribute("autoscan_type") = "parent";
                ce.append_attribute("autoscan_mode") = AutoscanDirectory::mapScanMode((*aDir)->getScanMode());
            }
        }
        ce.append_attribute("title") = f2i->convert(file.filename()).c_str();
    }
}

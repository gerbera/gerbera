/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/directories.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/directories.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "config/result/directory_tweak.h"
#include "content/content.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <algorithm>
#include <array>

const std::string_view Web::Directories::PAGE = "directories";

Web::Directories::Directories(const std::shared_ptr<Content>& content,
    std::shared_ptr<ConverterManager> converterManager,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : PageRequest(content, server, xmlBuilder, quirks)
    , converterManager(std::move(converterManager))
{
}

bool Web::Directories::processPageAction(Json::Value& element, const std::string& action)
{
    static auto RootId = fmt::format("{}", CDS_ID_ROOT);
    std::string parentID = param("parent_id");
    auto path = fs::path(parentID.empty() || parentID == RootId ? FS_ROOT_DIRECTORY : hexDecodeString(parentID));

    Json::Value containers;
    Json::Value containerArray(Json::arrayValue);
    containers["parent_id"] = parentID;
    containers["type"] = "filesystem";
    if (!param("select_it").empty())
        containers["select_it"] = param("select_it");

    auto filesMap = listFiles(path);
    outputFiles(containerArray, filesMap);

    containers["container"] = containerArray;
    element["containers"] = containers;

    return true;
}

std::map<std::string, Web::Directories::DirInfo> Web::Directories::listFiles(const fs::path& path)
{
    std::error_code ec;
    std::map<std::string, Web::Directories::DirInfo> filesMap;
    // don't bother users with system directorties
    auto&& excludesFullpath = config->getArrayOption(ConfigVal::IMPORT_SYSTEM_DIRECTORIES);
    auto&& includesFullpath = config->getArrayOption(ConfigVal::IMPORT_VISIBLE_DIRECTORIES);
    // don't bother users with special or config directorties
    constexpr std::array excludesDirname {
        "lost+found",
    };
    bool excludeConfigDirs = true;

    for (auto&& it : fs::directory_iterator(path, ec)) {
        const fs::path& filepath = it.path();

        if (!it.is_directory(ec))
            continue;
        if (!includesFullpath.empty()
            && std::none_of(includesFullpath.begin(), includesFullpath.end(), //
                [&](fs::path&& sub) { return isSubDir(filepath.string(), sub) || isSubDir(sub, filepath.string()) || sub == filepath; })) {
            log_debug("skipping unwanted dir {}", filepath.string());
            continue; // skip unwanted dir
        }
        if (includesFullpath.empty()) {
            if (std::find(excludesFullpath.begin(), excludesFullpath.end(), filepath) != excludesFullpath.end()) {
                log_debug("skipping excluded dir {}", filepath.string());
                continue; // skip excluded dir
            }
            if (std::find(excludesDirname.begin(), excludesDirname.end(), filepath.filename()) != excludesDirname.end()
                || (excludeConfigDirs && startswith(filepath.filename().string(), "."))) {
                log_debug("skipping special dir {}", filepath.string());
                continue; // skip dir with leading .
            }
        }
        auto dir = fs::directory_iterator(filepath, ec);
        bool hasContent = std::any_of(begin(dir), end(dir), [&](auto&& sub) { return sub.is_directory(ec) || isRegularFile(sub, ec); });

        /// \todo replace hexEncode with base64_encode?
        std::string id = hexEncode(filepath.c_str(), filepath.string().length());
        filesMap.emplace(id, std::pair(filepath, hasContent));
    }
    return filesMap;
}

void Web::Directories::outputFiles(
    Json::Value& containerArray,
    const std::map<std::string, Web::Directories::DirInfo>& filesMap)
{
    auto autoscanDirs = content->getAutoscanDirectories();
    auto allTweaks = config->getDirectoryTweakOption(ConfigVal::IMPORT_DIRECTORIES_LIST)->getArrayCopy();

    auto f2i = converterManager->f2i();
    for (auto&& [key, val] : filesMap) {
        auto file = val.first;
        auto&& has = val.second;
        Json::Value ce;
        ce["id"] = key;
        ce["child_count"] = has;
        auto tweak = std::find_if(allTweaks.begin(), allTweaks.end(), [&](auto& d) { return file == d->getLocation(); });
        auto aDir = std::find_if(autoscanDirs.begin(), autoscanDirs.end(), [&](auto& a) { return file == a->getLocation(); });
        ce["tweak"] = tweak != allTweaks.end();
        if (aDir != autoscanDirs.end()) {
            ce["autoscan_type"] = (*aDir)->persistent() ? "persistent" : "ui";
            ce["autoscan_mode"] = AutoscanDirectory::mapScanmode((*aDir)->getScanMode());
        } else {
            aDir = std::find_if(autoscanDirs.begin(), autoscanDirs.end(), [&](auto& a) { return a->getRecursive() && isSubDir(file, a->getLocation()); });
            if (aDir != autoscanDirs.end()) {
                ce["autoscan_type"] = "parent";
                ce["autoscan_mode"] = AutoscanDirectory::mapScanmode((*aDir)->getScanMode());
            }
        }
        {
            auto [mval, err] = f2i->convert(file.filename());
            if (!err.empty()) {
                log_warning("{}: {}", file.filename().string(), err);
            }
            ce["title"] = mval;
        }
        {
            auto [mval, err] = f2i->convert(file);
            if (!err.empty()) {
                log_warning("{}: {}", file.string(), err);
            }
            ce["location"] = mval;
        }
        ce["upnp_class"] = "folder";
        containerArray.append(ce);
    }
}

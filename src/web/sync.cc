/*GRB*

    Gerbera - https://gerbera.io/

    web/sync.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with Gerbera; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// @file web/sync.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "common.h"
#include "cds/cds_objects.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"
#include "util/tools.h"

const std::string_view Web::Sync::PAGE = "sync";

bool Web::Sync::processPageAction(Json::Value& element, const std::string& action)
{
    std::string objID = param("object_id");
    auto path = fs::path((objID == "0") ? FS_ROOT_DIRECTORY : hexDecodeString(objID));
    if (path.empty())
        throw_std_runtime_error("Illegal empty path");

    std::error_code ec;
    auto dirEnt = fs::directory_entry(path, ec);
    if (ec || !dirEnt.is_directory(ec))
        throw_std_runtime_error("Failed to read {}: {}", path.string(), ec ? ec.message() : "not a directory");

    // prefer the autoscan machinery when the path is tracked
    auto adir = content->getAutoscanDirectory(path);
    if (!adir) {
        for (auto&& dir : content->getAutoscanDirectories()) {
            if (dir && dir->getRecursive() && isSubDir(path, dir->getLocation()))
                adir = dir;
        }
    }
    if (!adir)
        throw_std_runtime_error("sync called with untracked path {}", path.string());

    if (adir->getLocation() == path) {
        content->rescanDirectory(adir, adir->getObjectID(), path, true);
    } else {
        auto container = database->findObjectByPath(path, UNUSED_CLIENT_GROUP, DbFileType::Directory);
        if (container && container->isContainer()) {
            content->rescanDirectory(adir, container->getID(), path, true);
        } else {
            // tracked path not scanned yet: import just this subtree,
            // mirroring the addSubDirectory branch of a running rescan
            AutoScanSetting asSetting;
            asSetting.adir = adir;
            asSetting.recursive = adir->getRecursive();
            asSetting.followSymlinks = adir->getFollowSymlinks();
            asSetting.hidden = adir->getHidden();
            asSetting.rescanResource = false;
            asSetting.mergeOptions(config, path);
            content->addFile(dirEnt, adir->getLocation(), asSetting, true, false);
        }
    }
    return true;
}

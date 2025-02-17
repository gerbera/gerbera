/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/add.cc - this file is part of MediaTomb.

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

/// \file web/add.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "common.h"
#include "config/config_val.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "exceptions.h"
#include "util/tools.h"
#include "util/xml_to_json.h"

const std::string Web::Add::PAGE = "add";

void Web::Add::processPageAction(pugi::xml_node& element)
{
    std::string objID = param("object_id");
    auto path = fs::path((objID == "0") ? FS_ROOT_DIRECTORY : hexDecodeString(objID));
    if (path.empty())
        throw_std_runtime_error("Illegal empty path");

    AutoScanSetting asSetting;
    asSetting.recursive = true;
    asSetting.followSymlinks = config->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    asSetting.hidden = config->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);
    asSetting.mergeOptions(config, path);

    std::error_code ec;
    auto dirEnt = fs::directory_entry(path, ec);
    if (!ec) {
        content->addFile(dirEnt, asSetting);
    } else {
        log_error("Failed to read {}: {}", path.c_str(), ec.message());
    }
}

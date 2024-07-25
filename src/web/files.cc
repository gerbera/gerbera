/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/files.cc - this file is part of MediaTomb.

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

/// \file web/files.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "common.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "util/xml_to_json.h"

void Web::Files::process()
{
    checkRequest();

    std::string parentID = param("parent_id");
    std::string path = (parentID == "0") ? FS_ROOT_DIRECTORY : hexDecodeString(parentID);

    auto root = xmlDoc->document_element();

    auto files = root.append_child("files");
    xml2Json->setArrayName(files, "file");
    xml2Json->setFieldType("filename", FieldType::STRING);
    files.append_attribute("parent_id") = parentID.c_str();
    files.append_attribute("location") = path.c_str();

    bool excludeConfigFiles = true;

    std::error_code ec;
    std::map<std::string, fs::path> filesMap;
    auto&& includesFullpath = config->getArrayOption(ConfigVal::IMPORT_VISIBLE_DIRECTORIES);

    for (auto&& it : fs::directory_iterator(path, ec)) {
        const fs::path& filepath = it.path();

        if (!isRegularFile(it, ec))
            continue;
        if (excludeConfigFiles && startswith(filepath.filename().string(), "."))
            continue;
        if (!includesFullpath.empty()
            && std::none_of(includesFullpath.begin(), includesFullpath.end(), //
                [=](auto&& sub) { return startswith(filepath.string(), sub); }))
            continue; // skip unwanted file

        std::string id = hexEncode(filepath.c_str(), filepath.string().length());
        filesMap.try_emplace(id, filepath.filename());
    }

    auto f2i = StringConverter::f2i(config);
    for (auto&& [key, val] : filesMap) {
        auto fe = files.append_child("file");
        fe.append_attribute("id") = key.c_str();
        fe.append_attribute("filename") = f2i->convert(val).c_str();
    }
}

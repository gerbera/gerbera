/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    files.cc - this file is part of MediaTomb.
    
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

/// \file files.cc

#include "pages.h" // API

#include <utility>

#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

web::files::files(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
}

void web::files::process()
{
    check_request();

    std::string path;
    std::string parentID = param("parent_id");
    if (parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(parentID);

    auto root = xmlDoc->document_element();

    auto files = root.append_child("files");
    xml2JsonHints->setArrayName(files, "file");
    files.append_attribute("parent_id") = parentID.c_str();
    files.append_attribute("location") = path.c_str();

    bool exclude_config_files = true;

    for (auto& it : fs::directory_iterator(path)) {
        const fs::path& filepath = it.path();

        std::error_code ec;
        if (!isRegularFile(it.path(), ec))
            continue;
        if (exclude_config_files && startswith(filepath.filename(), "."))
            continue;

        std::string id = hex_encode(filepath.c_str(), filepath.string().length());

        auto fe = files.append_child("file");
        fe.append_attribute("id") = id.c_str();
        auto f2i = StringConverter::f2i(config);
        fe.append_attribute("filename") = f2i->convert(filepath.filename()).c_str();
    }
}

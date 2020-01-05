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

#include "common.h"
#include "util/filesystem.h"
#include "pages.h"
#include "storage/storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

web::files::files(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::files::process()
{
    check_request();

    std::string path;
    std::string parentID = param("parent_id");
    if (!string_ok(parentID) || parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(parentID);

    Ref<Element> files(new Element("files"));
    files->setArrayName("file");
    files->setAttribute("parent_id", parentID);
    files->setAttribute("location", path);
    root->appendElementChild(files);

    auto fs = std::make_unique<Filesystem>(config);
    auto arr = fs->readDirectory(path, FS_MASK_FILES);
    for (auto it = arr.begin(); it != arr.end(); it++) {
        Ref<Element> fe(new Element("file"));
        std::string filename = (*it)->filename;
        std::string filepath = path + "/" + filename;
        std::string id = hex_encode(filepath.c_str(), filepath.length());
        fe->setAttribute("id", id);

        Ref<StringConverter> f2i = StringConverter::f2i(config);
        fe->setTextKey("filename");
        fe->setText(f2i->convert(filename));
        files->appendElementChild(fe);
    }
}

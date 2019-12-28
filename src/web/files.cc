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
#include "storage.h"
#include "util/string_converter.h"
#include "util/tools.h"

using namespace zmm;
using namespace mxml;

web::files::files()
    : WebRequestHandler()
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

    Ref<Filesystem> fs(new Filesystem());
    Ref<Array<FsObject>> arr;
    arr = fs->readDirectory(path, FS_MASK_FILES);

    for (int i = 0; i < arr->size(); i++) {
        Ref<FsObject> obj = arr->get(i);

        Ref<Element> fe(new Element("file"));
        std::string filename = obj->filename;
        std::string filepath = path + "/" + filename;
        std::string id = hex_encode(filepath.c_str(), filepath.length());
        fe->setAttribute("id", id);

        Ref<StringConverter> f2i = StringConverter::f2i();
        fe->setTextKey("filename");
        fe->setText(f2i->convert(filename));
        files->appendElementChild(fe);
    }
}

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

#include "common.h"
#include "filesystem.h"
#include "pages.h"
#include "storage.h"
#include "string_converter.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

web::directories::directories()
    : WebRequestHandler()
{
}

void web::directories::process()
{
    check_request();

    std::string path;
    std::string parentID = param("parent_id");
    if (parentID.empty() || parentID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(parentID);

    Ref<Element> containers(new Element("containers"));
    containers->setArrayName("container");
    containers->setAttribute("parent_id", parentID);
    if (string_ok(param("select_it")))
        containers->setAttribute("select_it", param("select_it"));
    containers->setAttribute("type", "filesystem");
    root->appendElementChild(containers);

    Ref<Filesystem> fs(new Filesystem());

    Ref<Array<FsObject>> arr;
    arr = fs->readDirectory(path, FS_MASK_DIRECTORIES,
        FS_MASK_DIRECTORIES);

    for (int i = 0; i < arr->size(); i++) {
        Ref<FsObject> obj = arr->get(i);

        Ref<Element> ce(new Element("container"));
        std::string filename = obj->filename;
        std::string filepath;
        if (path.c_str()[path.length() - 1] == '/')
            filepath = path + filename;
        else
            filepath = path + '/' + filename;

        /// \todo replace hex_encode with base64_encode?
        std::string id = hex_encode(filepath.c_str(), filepath.length());
        ce->setAttribute("id", id);
        if (obj->hasContent)
            ce->setAttribute("child_count", std::to_string(1), mxml_int_type);
        else
            ce->setAttribute("child_count", std::to_string(0), mxml_int_type);

        Ref<StringConverter> f2i = StringConverter::f2i();
        ce->setTextKey("title");
        ce->setText(f2i->convert(filename));
        containers->appendElementChild(ce);
    }
}

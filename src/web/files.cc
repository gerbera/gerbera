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
#include "filesystem.h"
#include "pages.h"
#include "storage.h"
#include "string_converter.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

web::files::files()
    : WebRequestHandler()
{
}

void web::files::process()
{
    check_request();

    String path;
    String parentID = param(_("parent_id"));
    if (!string_ok(parentID) || parentID == "0")
        path = _(FS_ROOT_DIRECTORY);
    else
        path = hex_decode_string(parentID);

    Ref<Element> files(new Element(_("files")));
    files->setArrayName(_("file"));
    files->setAttribute(_("parent_id"), parentID);
    files->setAttribute(_("location"), path);
    root->appendElementChild(files);

    Ref<Filesystem> fs(new Filesystem());
    Ref<Array<FsObject>> arr;
    arr = fs->readDirectory(path, FS_MASK_FILES);

    for (int i = 0; i < arr->size(); i++) {
        Ref<FsObject> obj = arr->get(i);

        Ref<Element> fe(new Element(_("file")));
        String filename = obj->filename;
        String filepath = path + _("/") + filename;
        String id = hex_encode(filepath.c_str(), filepath.length());
        fe->setAttribute(_("id"), id);

        Ref<StringConverter> f2i = StringConverter::f2i();
        fe->setTextKey(_("filename"));
        fe->setText(f2i->convert(filename));
        files->appendElementChild(fe);
    }
}

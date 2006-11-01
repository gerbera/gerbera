/*  files.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "common.h"
#include "pages.h"
#include "tools.h"
#include "storage.h"
#include "filesystem.h"
#include "string_converter.h"

using namespace zmm;
using namespace mxml;

web::files::files() : WebRequestHandler()
{
}

void web::files::process()
{
    check_request();

    String path;
    String parentID = param(_("parent_id"));
    if (! string_ok(parentID))
        path = _(FS_ROOT_DIRECTORY);
    else
        path = hex_decode_string(parentID);

    try
    {
        Ref<Filesystem> fs(new Filesystem());
        Ref<Array<FsObject> > arr = fs->readDirectory(path, FS_MASK_FILES);
    
        Ref<Element> files(new Element(_("files")));
        files->addAttribute(_("ofId"), parentID);
    
        for (int i = 0; i < arr->size(); i++)
        {
            Ref<FsObject> obj = arr->get(i);
    
            Ref<Element> fe(new Element(_("file")));
            String filename = obj->filename;
            String filepath = path + _("/") + filename;
            String id = hex_encode(filepath.c_str(), filepath.length());
            fe->addAttribute(_("id"), id);
            int childCount = 1;
            if (childCount)
                fe->addAttribute(_("childCount"), String::from(childCount));

            Ref<StringConverter> f2i = StringConverter::f2i();
            fe->setText(f2i->convert(filename));
            files->appendChild(fe);
        }
        root->appendChild(files);
        }
    catch (Exception e)
    {
        Ref<Element> error(new Element(_("error")));
        error->setText(e.getMessage());
        root->appendChild(error);
    }
}


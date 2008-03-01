/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    directories.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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

web::directories::directories() : WebRequestHandler()
{
}

void web::directories::process()
{
    check_request();

    String path;
    String parentID = param(_("parent_id"));
    if (parentID == nil || parentID == "0")
        path = _(FS_ROOT_DIRECTORY);
    else
        path = hex_decode_string(parentID);
    
    Ref<Element> containers (new Element(_("containers")));
    containers->setAttribute(_("ofId"), parentID);
    containers->setAttribute(_("select_it"), param(_("select_it")));
    containers->setAttribute(_("type"), _("f"));
    root->appendElementChild(containers);
    
    Ref<Filesystem> fs(new Filesystem());
    
    Ref<Array<FsObject> > arr;
    try
    {
        arr = fs->readDirectory(path, FS_MASK_DIRECTORIES,
                                                      FS_MASK_DIRECTORIES);
    }
    catch (Exception e)
    {
        containers->setAttribute(_("success"), _("0"));
        return;
    }
    containers->setAttribute(_("success"), _("1"));
    

    for (int i = 0; i < arr->size(); i++)
    {
        Ref<FsObject> obj = arr->get(i);

        Ref<Element> ce(new Element(_("container")));
        String filename = obj->filename;
        String filepath;
        if (path.c_str()[path.length() - 1] == '/')
            filepath = path + filename;
        else
            filepath = path + '/' + filename;
        
        /// \todo replace hex_encode with base64_encode?
        String id = hex_encode(filepath.c_str(), filepath.length());
        ce->setAttribute(_("id"), id);
        if (obj->hasContent)
            ce->setAttribute(_("childCount"), String::from(1));

        Ref<StringConverter> f2i = StringConverter::f2i();
        ce->setText(f2i->convert(filename));
        containers->appendElementChild(ce);
    }
}

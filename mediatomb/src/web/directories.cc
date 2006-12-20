/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    directories.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
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
    containers->addAttribute(_("ofId"), parentID);
    containers->addAttribute(_("select_it"), param(_("select_it")));
    containers->addAttribute(_("type"), _("f"));
    root->appendChild(containers);
    
    Ref<Filesystem> fs(new Filesystem());
    
    Ref<Array<FsObject> > arr;
    try
    {
        arr = fs->readDirectory(path, FS_MASK_DIRECTORIES,
                                                      FS_MASK_DIRECTORIES);
    }
    catch (Exception e)
    {
        containers->addAttribute(_("success"), _("0"));
        return;
    }
    containers->addAttribute(_("success"), _("1"));
    

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
        ce->addAttribute(_("id"), id);
        if (obj->hasContent)
            ce->addAttribute(_("childCount"), String::from(1));

        Ref<StringConverter> f2i = StringConverter::f2i();
        ce->setText(f2i->convert(filename));
        containers->appendChild(ce);
    }
}


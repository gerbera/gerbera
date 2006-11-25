/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    web_autoscan.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id: add.cc 1123 2006-11-03 01:03:30Z leo $
*/

/// \file web_autoscan.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "pages.h"
#include "storage.h"
#include "autoscan.h"
#include "content_manager.h"

using namespace zmm;
using namespace mxml;

web::autoscan::autoscan() : WebRequestHandler()
{
}

void web::autoscan::process()
{
    
    check_request();
    
    String action = param(_("action"));
    if (! string_ok(action))
        throw _Exception(_("web:autoscan called with illegal action"));
    
    Ref<Storage> storage = Storage::getInstance();
    Ref<ContentManager> cm = ContentManager::getInstance();
        
    if (action == "add")
    {
        int objID = intParam(_("object_id"));
        bool recursive = boolParam(_("recursive"));
        bool hidden = boolParam(_("hidden"));
        bool persistent = boolParam(_("persistent"));
        int interval = intParam(_("interval"), 0);
        if (interval <= 0 )
        {
            throw _Exception(_("illegal interval given"));
        }
        else
        {
            String scan_level_str = param(_("scan_level"));
            scan_level_t scan_level = AutoscanDirectory::remapScanlevel(scan_level_str);
            scan_mode_t scan_mode = TimedScanMode;
            
            Ref<CdsObject> obj = storage->loadObject(objID);
            if (obj == nil
                || ! IS_CDS_CONTAINER(obj->getObjectType())
                || obj->isVirtual())
                throw _Exception(_("tried to add an illegal object (id) as an autoscan directory"));
            
            /// \todo make more configurable!
            Ref<AutoscanDirectory> autoscan(new AutoscanDirectory(
                obj->getLocation(),
                scan_mode,
                scan_level,
                recursive,
                persistent,
                -1,
                interval,
                hidden
                ));
            
            storage->addAutoscanDirectory(autoscan);
            cm->addAutoscanDirectory(autoscan);
        }
    }
    else if (action == "remove")
    {
        int objID = intParam(_("object_id"));
        if (! storage->isAutoscanDirectory(objID))
            throw _Exception(_("the object id ")+objID+" is not among the list of the autoscan directories");
        Ref<CdsObject> obj = storage->loadObject(objID);
        if (obj == nil
            || ! IS_CDS_CONTAINER(obj->getObjectType())
            || obj->isVirtual())
            throw _Exception(_("tried to remove an illegal object (id) from the list of the autoscan directories"));
        
        storage->removeAutoscanDirectory(objID);
        cm->removeAutoscanDirectory(obj->getLocation());
    }
    else
        throw _Exception(_("web:autoscan called with illegal action"));
}


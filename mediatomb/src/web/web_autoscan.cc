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
    
    $Id$
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
        bool fromFs = boolParam(_("fromFs"));
        //int objID = intParam(_("object_id"));
        bool recursive = boolParam(_("recursive"));
        bool hidden = boolParam(_("hidden"));
        //bool persistent = boolParam(_("persistent"));
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
            
            String location;
            if (fromFs)
            {
                location = hex_decode_string(param(_("object_id")));
                cm->ensurePathExistence(location);
            }
            else
            {
                Ref<CdsObject> obj = storage->loadObject(intParam(_("object_id")));
                if (obj == nil
                    || ! IS_CDS_CONTAINER(obj->getObjectType())
                    || obj->isVirtual())
                    throw _Exception(_("tried to add an illegal object (id) as an autoscan directory"));
                location = obj->getLocation();
            }
            
            log_debug("adding autoscan: location=%s, scan_mode=%s, scan_level=%s, recursive=%d, interval=%d, hidden=%d\n", 
                location.c_str(), AutoscanDirectory::mapScanmode(scan_mode).c_str(),
                AutoscanDirectory::mapScanlevel(scan_level).c_str(), recursive, interval, hidden);
            
            Ref<AutoscanDirectory> autoscan(new AutoscanDirectory(
                location,
                scan_mode,
                scan_level,
                recursive,
                false, // persistent
                -1, // autoscan id - used only internally by CM
                interval,
                hidden
                ));
            /* 
            
            try
            {
            */
            cm->addAutoscanDirectory(autoscan);
            //storage->addAutoscanDirectory(autoscan);
            /* why was this here??
            }
            catch (Exception e)
            {
                throw e;
            }
            */
        }
    }
    else if (action == "remove")
    {
        int objID = intParam(_("object_id"));
        if (storage->getAutoscanDirectoryType(objID) != 1)
            throw _Exception(_("the object id ")+objID+" is not among the list of the autoscan directories, or it was defined in config.xml");
        Ref<CdsObject> obj = storage->loadObject(objID);
        if (obj == nil
            || ! IS_CDS_CONTAINER(obj->getObjectType())
            || obj->isVirtual())
            throw _Exception(_("tried to remove an illegal object (id) from the list of the autoscan directories"));
        
        storage->removeAutoscanDirectory(objID);
        cm->removeAutoscanDirectory(obj->getLocation());
    }
    else if (action == "list")
    {
        /// \todo use a method of ContentManager, should give back all autoscans
        Ref<AutoscanList> autoscanList = storage->getAutoscanList(TimedScanMode);
        
        int size = autoscanList->size();
        Ref<Element> autoscansEl (new Element(_("autoscans")));
        for (int i = 0; i < size; i++)
        {
            Ref<AutoscanDirectory> autoscanDir = autoscanList->get(i);
            Ref<Element> autoscanEl (new Element(_("autoscan")));
            autoscanEl->addAttribute(_("objectID"), String::from(autoscanDir->getObjectID()));
            autoscanEl->appendTextChild(_("location"), autoscanDir->getLocation());
            //autoscanEl->appendTextChild(_("scan_level"), AutoscanDirectory::mapScanlevel(autoscanDir->getScanLevel()));
            autoscansEl->appendChild(autoscanEl);
        }
    }
    else
        throw _Exception(_("web:autoscan called with illegal action"));
}


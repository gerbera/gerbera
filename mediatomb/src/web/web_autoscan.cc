/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_autoscan.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    Ref<ContentManager> cm = ContentManager::getInstance();
    Ref<Storage> storage = Storage::getInstance();
    
    if (action == "as_edit_load")
    {
        bool fromFs = boolParam(_("from_fs"));
        Ref<Element> autoscan (new Element(_("autoscan")));
        root->appendChild(autoscan);
        if (fromFs)
        {
            autoscan->appendTextChild(_("from_fs"), _("1"));
            autoscan->appendTextChild(_("object_id"), param(_("object_id")));
            Ref<AutoscanDirectory> adir = cm->getAutoscanDirectory(hex_decode_string(param(_("object_id"))));
            autoscan2XML(autoscan, adir);
        }
        else
        {
            autoscan->appendTextChild(_("from_fs"), _("0"));
            autoscan->appendTextChild(_("object_id"), param(_("object_id")));
            Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(intParam(_("object_id")));
            autoscan2XML(autoscan, adir);
        }
    }
    else if (action == "as_edit_save")
    {
        bool fromFs = boolParam(_("from_fs"));
        
        String scan_level_str = param(_("scan_level"));
        if (scan_level_str == "none")
        {
            // remove...
            try
            {
                if (fromFs)
                    cm->removeAutoscanDirectory(hex_decode_string(param(_("object_id"))));
                else
                    cm->removeAutoscanDirectory(intParam(_("object_id")));
            }
            catch (Exception e)
            {
                // didn't work, well we don't care in this case
            }
        }
        else
        {
            // add or update
            bool recursive = boolParam(_("recursive"));
            bool hidden = boolParam(_("hidden"));
            //bool persistent = boolParam(_("persistent"));
            int interval = intParam(_("interval"), 0);
            if (interval <= 0 )
            throw _Exception(_("illegal interval given"));
            scan_level_t scan_level = AutoscanDirectory::remapScanlevel(scan_level_str);
            scan_mode_t scan_mode = TimedScanMode;
            
            String location;
            int objectID = INVALID_OBJECT_ID;
            if (fromFs)
            {
                location = hex_decode_string(param(_("object_id")));
                objectID = cm->ensurePathExistence(location);
            }
            else
            {
                objectID = intParam(_("object_id"));
            }
            
            //log_debug("adding autoscan: location=%s, scan_mode=%s, scan_level=%s, recursive=%d, interval=%d, hidden=%d\n", 
            //    location.c_str(), AutoscanDirectory::mapScanmode(scan_mode).c_str(),
            //    AutoscanDirectory::mapScanlevel(scan_level).c_str(), recursive, interval, hidden);
            
            Ref<AutoscanDirectory> autoscan(new AutoscanDirectory(
                nil, //location
                scan_mode,
                scan_level,
                recursive,
                false, // persistent
                INVALID_SCAN_ID, // autoscan id - used only internally by CM
                interval,
                hidden
                ));
            autoscan->setObjectID(objectID);
            cm->setAutoscanDirectory(autoscan);
        }
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
        root->appendChild(autoscansEl);
    }
    else
        throw _Exception(_("web:autoscan called with illegal action"));
}

void web::autoscan::autoscan2XML(Ref<Element> element, Ref<AutoscanDirectory> adir)
{
    if (adir == nil)
    {
        element->appendTextChild(_("scan_level"), _("none"));
        element->appendTextChild(_("recursive"), _("0"));
        element->appendTextChild(_("hidden"), _("0"));
        element->appendTextChild(_("interval"), _("1800"));
    }
    else
    {
        element->appendTextChild(_("scan_level"), AutoscanDirectory::mapScanlevel(adir->getScanLevel()));
        element->appendTextChild(_("recursive"), (adir->getRecursive() ? _("1") : _("0") ));
        element->appendTextChild(_("hidden"), (adir->getHidden() ? _("1") : _("0") ));
        element->appendTextChild(_("interval"), String::from(adir->getInterval()));
    }
}

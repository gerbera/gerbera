/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_autoscan.cc - this file is part of MediaTomb.
    
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

/// \file web_autoscan.cc

#include "autoscan.h"
#include "content_manager.h"
#include "filesystem.h"
#include "pages.h"
#include "storage.h"

using namespace zmm;
using namespace mxml;

int WebAutoscanProcessListComparator(void* arg1, void* arg2)
{
    auto* a1 = (AutoscanDirectory*)arg1;
    auto* a2 = (AutoscanDirectory*)arg2;
    return strcmp(a1->getLocation().c_str(), a2->getLocation().c_str());
}

web::autoscan::autoscan()
    : WebRequestHandler()
{
}

void web::autoscan::process()
{

    check_request();

    String action = param(_("action"));
    if (!string_ok(action))
        throw _Exception(_("web:autoscan called with illegal action"));

    Ref<ContentManager> cm = ContentManager::getInstance();
    Ref<Storage> storage = Storage::getInstance();

    bool fromFs = boolParam(_("from_fs"));
    String path;
    String objID = param(_("object_id"));
    if (fromFs) {
        if (!string_ok(objID) || objID == "0")
            path = _(FS_ROOT_DIRECTORY);
        else
            path = hex_decode_string(objID);
    }

    if (action == "as_edit_load") {
        Ref<Element> autoscan(new Element(_("autoscan")));
        root->appendElementChild(autoscan);
        if (fromFs) {
            autoscan->appendTextChild(_("from_fs"), _("1"), mxml_bool_type);
            autoscan->appendTextChild(_("object_id"), objID);
            Ref<AutoscanDirectory> adir = cm->getAutoscanDirectory(path);
            autoscan2XML(autoscan, adir);
        } else {
            autoscan->appendTextChild(_("from_fs"), _("0"), mxml_bool_type);
            autoscan->appendTextChild(_("object_id"), objID);
            Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(intParam(_("object_id")));
            autoscan2XML(autoscan, adir);
        }
    } else if (action == "as_edit_save") {
        String scan_mode_str = param(_("scan_mode"));
        if (scan_mode_str == "none") {
            // remove...
            try {
                if (fromFs)
                    cm->removeAutoscanDirectory(path);
                else
                    cm->removeAutoscanDirectory(intParam(_("object_id")));
            } catch (const Exception& e) {
                // didn't work, well we don't care in this case
            }
        } else {
            // add or update
            bool recursive = boolParam(_("recursive"));
            bool hidden = boolParam(_("hidden"));
            //bool persistent = boolParam(_("persistent"));

            ScanMode scan_mode = AutoscanDirectory::remapScanmode(scan_mode_str);
            ScanLevel scan_level;
            scan_level = AutoscanDirectory::remapScanlevel(param(_("scan_level")));
            int interval = intParam(_("interval"), 0);
            if (scan_mode == ScanMode::Timed && interval <= 0)
                throw _Exception(_("illegal interval given"));

            int objectID = INVALID_OBJECT_ID;
            if (fromFs)
                objectID = cm->ensurePathExistence(path);
            else
                objectID = intParam(_("object_id"));

            //log_debug("adding autoscan: location=%s, scan_mode=%s, scan_level=%s, recursive=%d, interval=%d, hidden=%d\n",
            //    location.c_str(), AutoscanDirectory::mapScanmode(scan_mode).c_str(),
            //    AutoscanDirectory::mapScanlevel(scan_level).c_str(), recursive, interval, hidden);

            Ref<AutoscanDirectory> autoscan(new AutoscanDirectory(
                nullptr, //location
                scan_mode,
                scan_level,
                recursive,
                false, // persistent
                INVALID_SCAN_ID, // autoscan id - used only internally by CM
                interval,
                hidden));
            autoscan->setObjectID(objectID);
            cm->setAutoscanDirectory(autoscan);
        }
    } else if (action == "list") {
        Ref<Array<AutoscanDirectory>> autoscanList = cm->getAutoscanDirectories();
        int size = autoscanList->size();

        // --- sorting autoscans

        quicksort((COMPARABLE*)autoscanList->getObjectArray(), autoscanList->size(),
            WebAutoscanProcessListComparator);

        // ---

        Ref<Element> autoscansEl(new Element(_("autoscans")));
        autoscansEl->setArrayName(_("autoscan"));
        for (int i = 0; i < size; i++) {
            Ref<AutoscanDirectory> autoscanDir = autoscanList->get(i);
            Ref<Element> autoscanEl(new Element(_("autoscan")));
            autoscanEl->setAttribute(_("objectID"), String::from(autoscanDir->getObjectID()));
            autoscanEl->appendTextChild(_("location"), autoscanDir->getLocation());
            autoscanEl->appendTextChild(_("scan_mode"), AutoscanDirectory::mapScanmode(autoscanDir->getScanMode()));
            autoscanEl->appendTextChild(_("from_config"), autoscanDir->persistent() ? _("1") : _("0"), mxml_bool_type);
            //autoscanEl->appendTextChild(_("scan_level"), AutoscanDirectory::mapScanlevel(autoscanDir->getScanLevel()));
            autoscansEl->appendElementChild(autoscanEl);
        }
        root->appendElementChild(autoscansEl);
    } else
        throw _Exception(_("web:autoscan called with illegal action"));
}

void web::autoscan::autoscan2XML(Ref<Element> element, Ref<AutoscanDirectory> adir)
{
    if (adir == nullptr) {
        element->appendTextChild(_("scan_mode"), _("none"));
        element->appendTextChild(_("scan_level"), _("full"));
        element->appendTextChild(_("recursive"), _("0"), mxml_bool_type);
        element->appendTextChild(_("hidden"), _("0"), mxml_bool_type);
        element->appendTextChild(_("interval"), _("1800"), mxml_int_type);
        element->appendTextChild(_("persistent"), _("0"), mxml_bool_type);
    } else {
        element->appendTextChild(_("scan_mode"), AutoscanDirectory::mapScanmode(adir->getScanMode()));
        element->appendTextChild(_("scan_level"), AutoscanDirectory::mapScanlevel(adir->getScanLevel()));
        element->appendTextChild(_("recursive"), (adir->getRecursive() ? _("1") : _("0")), mxml_bool_type);
        element->appendTextChild(_("hidden"), (adir->getHidden() ? _("1") : _("0")), mxml_bool_type);
        element->appendTextChild(_("interval"), String::from(adir->getInterval()), mxml_int_type);
        element->appendTextChild(_("persistent"), (adir->persistent() ? _("1") : _("0")), mxml_bool_type);
    }
}

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
#include "util/filesystem.h"
#include "pages.h"
#include "storage/storage.h"

using namespace zmm;
using namespace mxml;

int WebAutoscanProcessListComparator(void* arg1, void* arg2)
{
    auto* a1 = (AutoscanDirectory*)arg1;
    auto* a2 = (AutoscanDirectory*)arg2;
    return strcmp(a1->getLocation().c_str(), a2->getLocation().c_str());
}

web::autoscan::autoscan(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::autoscan::process()
{

    check_request();

    std::string action = param("action");
    if (!string_ok(action))
        throw std::runtime_error("web:autoscan called with illegal action");

    bool fromFs = boolParam("from_fs");
    std::string path;
    std::string objID = param("object_id");
    if (fromFs) {
        if (!string_ok(objID) || objID == "0")
            path = FS_ROOT_DIRECTORY;
        else
            path = hex_decode_string(objID);
    }

    if (action == "as_edit_load") {
        Ref<Element> autoscan(new Element("autoscan"));
        root->appendElementChild(autoscan);
        if (fromFs) {
            autoscan->appendTextChild("from_fs", "1", mxml_bool_type);
            autoscan->appendTextChild("object_id", objID);
            Ref<AutoscanDirectory> adir = content->getAutoscanDirectory(path);
            autoscan2XML(autoscan, adir);
        } else {
            autoscan->appendTextChild("from_fs", "0", mxml_bool_type);
            autoscan->appendTextChild("object_id", objID);
            Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(intParam("object_id"));
            autoscan2XML(autoscan, adir);
        }
    } else if (action == "as_edit_save") {
        std::string scan_mode_str = param("scan_mode");
        if (scan_mode_str == "none") {
            // remove...
            try {
                if (fromFs)
                    content->removeAutoscanDirectory(path);
                else
                    content->removeAutoscanDirectory(intParam("object_id"));
            } catch (const std::runtime_error& e) {
                // didn't work, well we don't care in this case
            }
        } else {
            // add or update
            bool recursive = boolParam("recursive");
            bool hidden = boolParam("hidden");
            //bool persistent = boolParam("persistent");

            ScanMode scan_mode = AutoscanDirectory::remapScanmode(scan_mode_str);
            ScanLevel scan_level;
            scan_level = AutoscanDirectory::remapScanlevel(param("scan_level"));
            int interval = intParam("interval", 0);
            if (scan_mode == ScanMode::Timed && interval <= 0)
                throw std::runtime_error("illegal interval given");

            int objectID = INVALID_OBJECT_ID;
            if (fromFs)
                objectID = content->ensurePathExistence(path);
            else
                objectID = intParam("object_id");

            //log_debug("adding autoscan: location={}, scan_mode={}, scan_level={}, recursive={}, interval={}, hidden={}",
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
            content->setAutoscanDirectory(autoscan);
        }
    } else if (action == "list") {
        Ref<Array<AutoscanDirectory>> autoscanList = content->getAutoscanDirectories();
        int size = autoscanList->size();

        // --- sorting autoscans

        quicksort((COMPARABLE*)autoscanList->getObjectArray(), autoscanList->size(),
            WebAutoscanProcessListComparator);

        // ---

        Ref<Element> autoscansEl(new Element("autoscans"));
        autoscansEl->setArrayName("autoscan");
        for (int i = 0; i < size; i++) {
            Ref<AutoscanDirectory> autoscanDir = autoscanList->get(i);
            Ref<Element> autoscanEl(new Element("autoscan"));
            autoscanEl->setAttribute("objectID", std::to_string(autoscanDir->getObjectID()));
            autoscanEl->appendTextChild("location", autoscanDir->getLocation());
            autoscanEl->appendTextChild("scan_mode", AutoscanDirectory::mapScanmode(autoscanDir->getScanMode()));
            autoscanEl->appendTextChild("from_config", autoscanDir->persistent() ? "1" : "0", mxml_bool_type);
            //autoscanEl->appendTextChild("scan_level", AutoscanDirectory::mapScanlevel(autoscanDir->getScanLevel()));
            autoscansEl->appendElementChild(autoscanEl);
        }
        root->appendElementChild(autoscansEl);
    } else
        throw std::runtime_error("web:autoscan called with illegal action");
}

void web::autoscan::autoscan2XML(Ref<Element> element, Ref<AutoscanDirectory> adir)
{
    if (adir == nullptr) {
        element->appendTextChild("scan_mode", "none");
        element->appendTextChild("scan_level", "full");
        element->appendTextChild("recursive", "0", mxml_bool_type);
        element->appendTextChild("hidden", "0", mxml_bool_type);
        element->appendTextChild("interval", "1800", mxml_int_type);
        element->appendTextChild("persistent", "0", mxml_bool_type);
    } else {
        element->appendTextChild("scan_mode", AutoscanDirectory::mapScanmode(adir->getScanMode()));
        element->appendTextChild("scan_level", AutoscanDirectory::mapScanlevel(adir->getScanLevel()));
        element->appendTextChild("recursive", (adir->getRecursive() ? "1" : "0"), mxml_bool_type);
        element->appendTextChild("hidden", (adir->getHidden() ? "1" : "0"), mxml_bool_type);
        element->appendTextChild("interval", std::to_string(adir->getInterval()), mxml_int_type);
        element->appendTextChild("persistent", (adir->persistent() ? "1" : "0"), mxml_bool_type);
    }
}

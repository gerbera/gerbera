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

#include "pages.h" // API

#include <utility>

#include "autoscan.h"
#include "content_manager.h"
#include "database/database.h"

static bool WebAutoscanProcessListComparator(const std::shared_ptr<AutoscanDirectory>& a1, const std::shared_ptr<AutoscanDirectory>& a2)
{
    return strcmp(a1->getLocation().c_str(), a2->getLocation().c_str()) < 0;
}

web::autoscan::autoscan(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::autoscan::process()
{
    check_request();

    std::string action = param("action");
    if (action.empty())
        throw_std_runtime_error("web:autoscan called with illegal action");

    bool fromFs = boolParam("from_fs");
    std::string path;
    std::string objID = param("object_id");
    if (fromFs) {
        if (objID == "0")
            path = FS_ROOT_DIRECTORY;
        else
            path = hexDecodeString(objID);
    }

    auto root = xmlDoc->document_element();

    if (action == "as_edit_load") {
        auto autoscan = root.append_child("autoscan");
        if (fromFs) {
            autoscan.append_child("from_fs").append_child(pugi::node_pcdata).set_value("1");
            autoscan.append_child("object_id").append_child(pugi::node_pcdata).set_value(objID.c_str());
            std::shared_ptr<AutoscanDirectory> adir = content->getAutoscanDirectory(path);
            autoscan2XML(adir, &autoscan);
        } else {
            autoscan.append_child("from_fs").append_child(pugi::node_pcdata).set_value("0");
            autoscan.append_child("object_id").append_child(pugi::node_pcdata).set_value(objID.c_str());
            std::shared_ptr<AutoscanDirectory> adir = database->getAutoscanDirectory(intParam("object_id"));
            autoscan2XML(adir, &autoscan);
        }
    } else if (action == "as_edit_save") {
        std::string scan_mode_str = param("scan_mode");
        if (scan_mode_str == "none") {
            // remove...
            try {
                std::shared_ptr<AutoscanDirectory> adir;
                if (fromFs)
                    adir = content->getAutoscanDirectory(path);
                else
                    adir = content->getAutoscanDirectory(intParam("object_id"));
                content->removeAutoscanDirectory(adir);
            } catch (const std::runtime_error& e) {
                // didn't work, well we don't care in this case
            }
        } else {
            // add or update
            bool recursive = boolParam("recursive");
            bool hidden = boolParam("hidden");
            //bool persistent = boolParam("persistent");

            ScanMode scan_mode = AutoscanDirectory::remapScanmode(scan_mode_str);
            int interval = intParam("interval", 0);
            if (scan_mode == ScanMode::Timed && interval <= 0)
                throw_std_runtime_error("illegal interval given");

            int objectID = fromFs ? content->ensurePathExistence(path) : intParam("object_id");

            //log_debug("adding autoscan: location={}, scan_mode={}, recursive={}, interval={}, hidden={}",
            //    location.c_str(), AutoscanDirectory::mapScanmode(scan_mode).c_str(),
            //    recursive, interval, hidden);

            auto autoscan = std::make_shared<AutoscanDirectory>(
                "", //location
                scan_mode,
                recursive,
                false, // persistent
                INVALID_SCAN_ID, // autoscan id - used only internally by CM
                interval,
                hidden);
            autoscan->setObjectID(objectID);
            content->setAutoscanDirectory(autoscan);
        }
    } else if (action == "list") {
        auto autoscanList = content->getAutoscanDirectories();

        // --- sorting autoscans

        std::sort(autoscanList.begin(), autoscanList.end(), WebAutoscanProcessListComparator);

        // ---

        auto autoscansEl = root.append_child("autoscans");
        xml2JsonHints->setArrayName(autoscansEl, "autoscan");
        for (const auto& autoscanDir : autoscanList) {
            auto autoscanEl = autoscansEl.append_child("autoscan");
            autoscanEl.append_attribute("objectID") = autoscanDir->getObjectID();

            autoscanEl.append_child("location").append_child(pugi::node_pcdata).set_value(autoscanDir->getLocation().c_str());
            autoscanEl.append_child("scan_mode").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::mapScanmode(autoscanDir->getScanMode()).c_str());
            autoscanEl.append_child("from_config").append_child(pugi::node_pcdata).set_value(autoscanDir->persistent() ? "1" : "0");
            //autoscanEl.append_child("scan_level").append_child(pugi::node_pcdata)
            //    .set_value(AutoscanDirectory::mapScanlevel(autoscanDir->getScanLevel()).c_str());
        }
    } else
        throw_std_runtime_error("called with illegal action");
}

void web::autoscan::autoscan2XML(const std::shared_ptr<AutoscanDirectory>& adir, pugi::xml_node* element)
{
    if (adir == nullptr) {
        element->append_child("scan_mode").append_child(pugi::node_pcdata).set_value("none");
        element->append_child("recursive").append_child(pugi::node_pcdata).set_value("0");
        element->append_child("hidden").append_child(pugi::node_pcdata).set_value("0");
        element->append_child("interval").append_child(pugi::node_pcdata).set_value("1800");
        element->append_child("persistent").append_child(pugi::node_pcdata).set_value("0");
    } else {
        element->append_child("scan_mode").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::mapScanmode(adir->getScanMode()).c_str());
        element->append_child("recursive").append_child(pugi::node_pcdata).set_value(adir->getRecursive() ? "1" : "0");
        element->append_child("hidden").append_child(pugi::node_pcdata).set_value(adir->getHidden() ? "1" : "0");
        element->append_child("interval").append_child(pugi::node_pcdata).set_value(std::to_string(adir->getInterval()).c_str());
        element->append_child("persistent").append_child(pugi::node_pcdata).set_value(adir->persistent() ? "1" : "0");
    }
}

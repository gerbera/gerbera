/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web_autoscan.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

#include "content/autoscan.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "util/xml_to_json.h"

void Web::Autoscan::process()
{
    checkRequest();

    std::string action = param("action");
    if (action.empty())
        throw_std_runtime_error("web:autoscan called with illegal action");

    bool fromFs = boolParam("from_fs");
    std::string objID = param("object_id");
    auto path = [fromFs, &objID]() -> fs::path {
        if (fromFs) {
            if (objID == "0")
                return FS_ROOT_DIRECTORY;
            return hexDecodeString(objID);
        }
        return {};
    }();

    auto root = xmlDoc->document_element();
    if (action == "as_edit_load") {
        auto autoscan = root.append_child("autoscan");
        if (fromFs) {
            autoscan.append_child("from_fs").append_child(pugi::node_pcdata).set_value("1");
            autoscan.append_child("object_id").append_child(pugi::node_pcdata).set_value(objID.c_str());
            auto adir = content->getAutoscanDirectory(path);
            autoscan2XML(adir, autoscan);
        } else {
            autoscan.append_child("from_fs").append_child(pugi::node_pcdata).set_value("0");
            autoscan.append_child("object_id").append_child(pugi::node_pcdata).set_value(objID.c_str());
            auto adir = database->getAutoscanDirectory(intParam("object_id"));
            autoscan2XML(adir, autoscan);
        }
    } else if (action == "as_edit_save") {
        std::string scanModeStr = param("scan_mode");
        if (scanModeStr == "none") {
            // remove...
            try {
                auto adir = fromFs ? content->getAutoscanDirectory(path) : content->getAutoscanDirectory(intParam("object_id"));
                content->removeAutoscanDirectory(adir);
            } catch (const std::runtime_error&) {
                // didn't work, well we don't care in this case
            }
        } else {
            // add or update
            bool recursive = boolParam("recursive");
            bool hidden = boolParam("hidden");
            bool followSymlinks = boolParam("followSymlinks");

            std::vector<std::string> mediaType;
            if (boolParam("audio"))
                mediaType.emplace_back("Audio");
            if (boolParam("audioMusic"))
                mediaType.emplace_back("Music");
            if (boolParam("audioBook"))
                mediaType.emplace_back("AudioBook");
            if (boolParam("audioBroadcast"))
                mediaType.emplace_back("AudioBroadcast");
            if (boolParam("image"))
                mediaType.emplace_back("Image");
            if (boolParam("imagePhoto"))
                mediaType.emplace_back("Photo");
            if (boolParam("video"))
                mediaType.emplace_back("Video");
            if (boolParam("videoMovie"))
                mediaType.emplace_back("Movie");
            if (boolParam("videoTV"))
                mediaType.emplace_back("TV");
            if (boolParam("videoMusicVideo"))
                mediaType.emplace_back("MusicVideo");
            int mt = AutoscanDirectory::makeMediaType(fmt::format("{}", fmt::join(mediaType, "|")));
            // bool persistent = boolParam("persistent");

            AutoscanScanMode scanMode = AutoscanDirectory::remapScanmode(scanModeStr);
            int interval;
            auto intervalParam = param("interval");
            parseTime(interval, intervalParam);
            if (scanMode == AutoscanScanMode::Timed && interval <= 0)
                throw_std_runtime_error("illegal interval given");

            int objectID = fromFs ? content->ensurePathExistence(path) : intParam("object_id");

            // log_info("adding autoscan {}: location={}, scan_mode={}, recursive={}, audio={}, image={}, video={}, interval={}, hidden={}",
            //     objectID, "", AutoscanDirectory::mapScanmode(scanMode),
            //     recursive, audio, image, video, interval, hidden);

            auto containerMap = AutoscanDirectory::ContainerTypesDefaults;
            containerMap[AutoscanMediaMode::Audio] = param("ctAudio");
            containerMap[AutoscanMediaMode::Image] = param("ctImage");
            containerMap[AutoscanMediaMode::Video] = param("ctVideo");
            auto autoscan = std::make_shared<AutoscanDirectory>(
                "", // location
                scanMode,
                recursive,
                false, // persistent
                interval,
                hidden,
                followSymlinks,
                mt);
            autoscan->setObjectID(objectID);
            content->setAutoscanDirectory(autoscan);
        }
    } else if (action == "list") {
        auto autoscanList = content->getAutoscanDirectories();

        // --- sorting autoscans

        std::sort(autoscanList.begin(), autoscanList.end(), [](auto&& a1, auto&& a2) { return a1->getLocation() < a2->getLocation(); });

        // ---

        auto autoscansEl = root.append_child("autoscans");
        xml2Json->setArrayName(autoscansEl, "autoscan");
        for (auto&& autoscanDir : autoscanList) {
            auto autoscanEl = autoscansEl.append_child("autoscan");
            autoscanEl.append_attribute("objectID") = autoscanDir->getObjectID();

            autoscanEl.append_child("location").append_child(pugi::node_pcdata).set_value(autoscanDir->getLocation().c_str());
            autoscanEl.append_child("scan_mode").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::mapScanmode(autoscanDir->getScanMode()));
            autoscanEl.append_child("from_config").append_child(pugi::node_pcdata).set_value(autoscanDir->persistent() ? "1" : "0");
            // autoscanEl.append_child("scan_level").append_child(pugi::node_pcdata)
            //     .set_value(AutoscanDirectory::mapScanlevel(autoscanDir->getScanLevel()).c_str());
        }
    } else
        throw_std_runtime_error("called with illegal action");
}

void Web::Autoscan::autoscan2XML(const std::shared_ptr<AutoscanDirectory>& adir, pugi::xml_node& element)
{
    if (!adir) {
        element.append_child("scan_mode").append_child(pugi::node_pcdata).set_value("none");
        element.append_child("recursive").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("hidden").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("followSymlinks").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("interval").append_child(pugi::node_pcdata).set_value("1800");
        element.append_child("persistent").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("mediaType").append_child(pugi::node_pcdata).set_value("-1");
        element.append_child("audio").append_child(pugi::node_pcdata).set_value("1");
        element.append_child("audioMusic").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("audioBook").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("audioBroadcast").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("image").append_child(pugi::node_pcdata).set_value("1");
        element.append_child("imagePhoto").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("video").append_child(pugi::node_pcdata).set_value("1");
        element.append_child("videoMovie").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("videoTV").append_child(pugi::node_pcdata).set_value("0");
        element.append_child("videoMusicVideo").append_child(pugi::node_pcdata).set_value("0");

        element.append_child("ctAudio").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio).c_str());
        element.append_child("ctImage").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Image).c_str());
        element.append_child("ctVideo").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Video).c_str());
    } else {
        element.append_child("scan_mode").append_child(pugi::node_pcdata).set_value(AutoscanDirectory::mapScanmode(adir->getScanMode()));
        element.append_child("recursive").append_child(pugi::node_pcdata).set_value(adir->getRecursive() ? "1" : "0");
        element.append_child("hidden").append_child(pugi::node_pcdata).set_value(adir->getHidden() ? "1" : "0");
        element.append_child("followSymlinks").append_child(pugi::node_pcdata).set_value(adir->getFollowSymlinks() ? "1" : "0");
        element.append_child("interval").append_child(pugi::node_pcdata).set_value(fmt::to_string(adir->getInterval().count()).c_str());
        element.append_child("persistent").append_child(pugi::node_pcdata).set_value(adir->persistent() ? "1" : "0");
        element.append_child("mediaType").append_child(pugi::node_pcdata).set_value(fmt::to_string(adir->getMediaType()).c_str());
        element.append_child("audio").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_AUDIO_ITEM) ? "1" : "0");
        element.append_child("audioMusic").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_MUSIC_TRACK) ? "1" : "0");
        element.append_child("audioBook").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_AUDIO_BOOK) ? "1" : "0");
        element.append_child("audioBroadcast").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_AUDIO_BROADCAST) ? "1" : "0");
        element.append_child("image").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_IMAGE_ITEM) ? "1" : "0");
        element.append_child("imagePhoto").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_IMAGE_PHOTO) ? "1" : "0");
        element.append_child("video").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_VIDEO_ITEM) ? "1" : "0");
        element.append_child("videoMovie").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_VIDEO_MOVIE) ? "1" : "0");
        element.append_child("videoTV").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_VIDEO_BROADCAST) ? "1" : "0");
        element.append_child("videoMusicVideo").append_child(pugi::node_pcdata).set_value(adir->hasContent(UPNP_CLASS_VIDEO_MUSICVIDEOCLIP) ? "1" : "0");

        element.append_child("ctAudio").append_child(pugi::node_pcdata).set_value(adir->getContainerTypes().at(AutoscanMediaMode::Audio).c_str());
        element.append_child("ctImage").append_child(pugi::node_pcdata).set_value(adir->getContainerTypes().at(AutoscanMediaMode::Image).c_str());
        element.append_child("ctVideo").append_child(pugi::node_pcdata).set_value(adir->getContainerTypes().at(AutoscanMediaMode::Video).c_str());
    }
}

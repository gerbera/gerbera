/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/web_autoscan.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/web_autoscan.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_objects.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"

const std::string_view Web::Autoscan::PAGE = "autoscan";

bool Web::Autoscan::processPageAction(Json::Value& element, const std::string& action)
{
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

    if (action == "as_edit_load") {
        editLoad(fromFs, element, objID, path);
    } else if (action == "as_edit_save") {
        editSave(fromFs, path);
    } else if (action == "as_run") {
        runScan(fromFs, path);
    } else if (action == "list") {
        list(element);
    } else
        throw_std_runtime_error("autoscan called with illegal action {}", action);
    return true;
}

void Web::Autoscan::editLoad(
    bool fromFs,
    Json::Value& element,
    const std::string& objID,
    const fs::path& path)
{
    Json::Value autoscan;
    autoscan["object_id"] = objID;
    std::shared_ptr<AutoscanDirectory> adir;
    if (fromFs) {
        autoscan["from_fs"] = 1;
        adir = content->getAutoscanDirectory(path);
    } else {
        autoscan["from_fs"] = 0;
        auto object = database->loadObject(intParam("object_id"));
        adir = object ? content->getAutoscanDirectory(object->getLocation()) : database->getAutoscanDirectory(intParam("object_id"));
    }
    autoscan2XML(adir, autoscan);
    element["autoscan"] = autoscan;
}

void Web::Autoscan::runScan(
    bool fromFs,
    const fs::path& path)
{
    std::shared_ptr<AutoscanDirectory> adir;
    if (fromFs) {
        adir = content->getAutoscanDirectory(path);
    } else {
        auto object = database->loadObject(intParam("object_id"));
        adir = object ? content->getAutoscanDirectory(object->getLocation()) : database->getAutoscanDirectory(intParam("object_id"));
    }
    if (adir)
        content->rescanDirectory(adir, adir->getObjectID(), path, true);
    else
        throw_std_runtime_error("runScan called with illegal parameters {}", fromFs ? path.string() : param("object_id"));
}

void Web::Autoscan::editSave(
    bool fromFs,
    const fs::path& path)
{
    std::string scanModeStr = param("scan_mode");
    if (scanModeStr == "none") {
        // remove...
        try {
            log_debug("Removing Autoscan {} ()", path.string(), intParam("object_id"));
            auto adir = fromFs ? content->getAutoscanDirectory(path) : content->getAutoscanDirectory(intParam("object_id"));
            content->removeAutoscanDirectory(adir);
        } catch (const std::runtime_error&) {
            log_warning("Remove Autoscan {} did not work", intParam("object_id"));
            // didn't work, well we don't care in this case
        }
    } else {
        // add or update
        bool recursive = boolParam("recursive");
        bool hidden = boolParam("hidden");
        bool followSymlinks = boolParam("followSymlinks");
        int retryCount = intParam("retryCount");
        bool dirTypes = boolParam("dirTypes");
        bool forceRescan = boolParam("forceRescan");

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
#if 0
        bool persistent = boolParam("persistent");
#endif

        AutoscanScanMode scanMode = AutoscanDirectory::remapScanmode(scanModeStr);
        int interval;
        auto intervalParam = param("interval");
        parseTime(interval, intervalParam);
        if (scanMode == AutoscanScanMode::Timed && interval <= 0)
            throw_std_runtime_error("illegal interval given");

        int objectID = fromFs ? content->ensurePathExistence(path) : intParam("object_id");

        log_debug("adding autoscan {}: location={}, recursive={}, mediaType={}, interval={}, hidden={}",
            objectID, path.string(), recursive, fmt::join(mediaType, "|"), interval, hidden);

        auto containerMap = AutoscanDirectory::ContainerTypesDefaults;
        containerMap[AutoscanMediaMode::Audio] = param("ctAudio");
        containerMap[AutoscanMediaMode::Image] = param("ctImage");
        containerMap[AutoscanMediaMode::Video] = param("ctVideo");
        auto adir = fromFs ? content->getAutoscanDirectory(path) : content->getAutoscanDirectory(intParam("object_id"));
        auto autoscan = std::make_shared<AutoscanDirectory>(
            path, // location
            scanMode,
            recursive,
            false, // persistent
            interval,
            hidden,
            followSymlinks,
            mt);
        if (adir) {
            autoscan->setScanID(adir->getScanID());
        }
        autoscan->setObjectID(objectID);
        autoscan->setRetryCount(retryCount);
        autoscan->setDirTypes(dirTypes);
        autoscan->setForceRescan(forceRescan);
        content->setAutoscanDirectory(autoscan);
    }
}

void Web::Autoscan::autoscan2XML(
    const std::shared_ptr<AutoscanDirectory>& adir,
    Json::Value& element)
{
    if (!adir) {
        element["scan_mode"] = "none";
        element["recursive"] = 0;
        element["hidden"] = 0;
        element["followSymlinks"] = 0;
        element["interval"] = 1800;
        element["persistent"] = 0;
        element["mediaType"] = -1;
        element["retryCount"] = 0;
        element["dirTypes"] = 0;
        element["forceRescan"] = 0;
        element["audio"] = 1;
        element["audioMusic"] = 0;
        element["audioBook"] = 0;
        element["audioBroadcast"] = 0;
        element["image"] = 1;
        element["imagePhoto"] = 0;
        element["video"] = 1;
        element["videoMovie"] = 0;
        element["videoTV"] = 0;
        element["videoMusicVideo"] = 0;

        element["ctAudio"] = AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio);
        element["ctImage"] = AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Image);
        element["ctVideo"] = AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Video);
    } else {
        element["scan_mode"] = AutoscanDirectory::mapScanmode(adir->getScanMode());
        element["recursive"] = adir->getRecursive() ? 1 : 0;
        element["hidden"] = adir->getHidden() ? 1 : 0;
        element["followSymlinks"] = adir->getFollowSymlinks() ? 1 : 0;
        element["interval"] = fmt::to_string(adir->getInterval().count());
        element["persistent"] = adir->persistent() ? 1 : 0;
        element["mediaType"] = fmt::to_string(adir->getMediaType());
        element["retryCount"] = fmt::to_string(adir->getRetryCount());
        element["dirTypes"] = adir->hasDirTypes() ? 1 : 0;
        element["forceRescan"] = adir->getForceRescan() ? 1 : 0;
        element["audio"] = adir->hasContent(UPNP_CLASS_AUDIO_ITEM) ? 1 : 0;
        element["audioMusic"] = adir->hasContent(UPNP_CLASS_MUSIC_TRACK) ? 1 : 0;
        element["audioBook"] = adir->hasContent(UPNP_CLASS_AUDIO_BOOK) ? 1 : 0;
        element["audioBroadcast"] = adir->hasContent(UPNP_CLASS_AUDIO_BROADCAST) ? 1 : 0;
        element["image"] = adir->hasContent(UPNP_CLASS_IMAGE_ITEM) ? 1 : 0;
        element["imagePhoto"] = adir->hasContent(UPNP_CLASS_IMAGE_PHOTO) ? 1 : 0;
        element["video"] = adir->hasContent(UPNP_CLASS_VIDEO_ITEM) ? 1 : 0;
        element["videoMovie"] = adir->hasContent(UPNP_CLASS_VIDEO_MOVIE) ? 1 : 0;
        element["videoTV"] = adir->hasContent(UPNP_CLASS_VIDEO_BROADCAST) ? 1 : 0;
        element["videoMusicVideo"] = adir->hasContent(UPNP_CLASS_VIDEO_MUSICVIDEOCLIP) ? 1 : 0;

        element["ctAudio"] = adir->getContainerTypes().at(AutoscanMediaMode::Audio);
        element["ctImage"] = adir->getContainerTypes().at(AutoscanMediaMode::Image);
        element["ctVideo"] = adir->getContainerTypes().at(AutoscanMediaMode::Video);
    }
}

void Web::Autoscan::list(Json::Value& element)
{
    auto autoscanList = content->getAutoscanDirectories();

    // --- sorting autoscans

    std::sort(autoscanList.begin(), autoscanList.end(), [](auto&& a1, auto&& a2) { return a1->getLocation() < a2->getLocation(); });

    // --- create list

    Json::Value autoscanArray(Json::arrayValue);
    for (auto&& autoscanDir : autoscanList) {
        Json::Value autoscanEl;
        autoscanEl["objectID"] = autoscanDir->getObjectID();
        autoscanEl["location"] = autoscanDir->getLocation().string();
        autoscanEl["scan_mode"] = AutoscanDirectory::mapScanmode(autoscanDir->getScanMode());
        autoscanEl["from_config"] = autoscanDir->persistent() ? 1 : 0;
        autoscanArray.append(autoscanEl);
    }
    element["autoscans"] = autoscanArray;
}

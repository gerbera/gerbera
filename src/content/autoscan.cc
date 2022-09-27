/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan.cc - this file is part of MediaTomb.

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

/// \file autoscan.cc

#include "autoscan.h" // API

#include "content_manager.h"
#include "database/database.h"

#include <numeric>

const std::map<AutoscanDirectory::MediaMode, std::string> AutoscanDirectory::ContainerTypesDefaults = {
    { AutoscanDirectory::MediaMode::Audio, UPNP_CLASS_MUSIC_ALBUM },
    { AutoscanDirectory::MediaMode::Image, UPNP_CLASS_PHOTO_ALBUM },
    { AutoscanDirectory::MediaMode::Video, UPNP_CLASS_CONTAINER },
};

AutoscanDirectory::AutoscanDirectory(fs::path location, ScanMode mode, bool recursive, bool persistent, unsigned int interval, bool hidden, int mediaType, const std::map<MediaMode, std::string>& containerMap)
    : location(std::move(location))
    , mode(mode)
    , recursive(recursive)
    , hidden(hidden)
    , persistent_flag(persistent)
    , interval(interval)
    , scanID(INVALID_SCAN_ID)
    , containerMap(containerMap)
{
    setMediaType(mediaType);
}

using MediaTypeIterator = EnumIterator<AutoscanDirectory::MediaType, AutoscanDirectory::MediaType::Audio, AutoscanDirectory::MediaType::MAX>;

std::map<AutoscanDirectory::MediaType, std::string_view> mediaTypes = {
    { AutoscanDirectory::MediaType::Audio, "Audio" },
    { AutoscanDirectory::MediaType::Music, "Music" },
    { AutoscanDirectory::MediaType::AudioBook, "AudioBook" },
    { AutoscanDirectory::MediaType::AudioBroadcast, "AudioBroadcast" },

    { AutoscanDirectory::MediaType::Image, "Image" },
    { AutoscanDirectory::MediaType::Photo, "Photo" },

    { AutoscanDirectory::MediaType::Video, "Video" },
    { AutoscanDirectory::MediaType::Movie, "Movie" },
    { AutoscanDirectory::MediaType::TV, "TV" },
    { AutoscanDirectory::MediaType::MusicVideo, "MusicVideo" },

    { AutoscanDirectory::MediaType::Any, "Any" },
    { AutoscanDirectory::MediaType::MAX, "MAX" },
};

std::map<AutoscanDirectory::MediaType, std::string> upnpClasses = {
    { AutoscanDirectory::MediaType::Audio, UPNP_CLASS_AUDIO_ITEM },
    { AutoscanDirectory::MediaType::Music, UPNP_CLASS_MUSIC_TRACK },
    { AutoscanDirectory::MediaType::AudioBook, UPNP_CLASS_AUDIO_BOOK },
    { AutoscanDirectory::MediaType::AudioBroadcast, UPNP_CLASS_AUDIO_BROADCAST },

    { AutoscanDirectory::MediaType::Image, UPNP_CLASS_IMAGE_ITEM },
    { AutoscanDirectory::MediaType::Photo, UPNP_CLASS_IMAGE_PHOTO },

    { AutoscanDirectory::MediaType::Video, UPNP_CLASS_VIDEO_ITEM },
    { AutoscanDirectory::MediaType::Movie, UPNP_CLASS_VIDEO_MOVIE },
    { AutoscanDirectory::MediaType::TV, UPNP_CLASS_VIDEO_BROADCAST },
    { AutoscanDirectory::MediaType::MusicVideo, UPNP_CLASS_VIDEO_MUSICVIDEOCLIP },

    { AutoscanDirectory::MediaType::Any, UPNP_CLASS_ITEM },
    { AutoscanDirectory::MediaType::MAX, UPNP_CLASS_ITEM },
};

int AutoscanDirectory::makeMediaType(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | AutoscanDirectory::remapMediaType(trimString(i)); });
}

std::string_view AutoscanDirectory::mapMediaType(AutoscanDirectory::MediaType mt)
{
    return mediaTypes[mt];
}

std::string AutoscanDirectory::mapMediaType(int mt)
{
    if (mt < 0)
        return mediaTypes.at(AutoscanDirectory::MediaType::Any).data();

    std::vector<std::string> myFlags;

    for (auto [mFlag, mLabel] : mediaTypes) {
        if (mt & (1 << to_underlying(mFlag))) {
            myFlags.emplace_back(mLabel);
            mt &= ~(1 << to_underlying(mFlag));
        }
    }

    if (mt) {
        myFlags.push_back(fmt::format("{:#04x}", mt));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

int AutoscanDirectory::remapMediaType(const std::string& flag)
{
    for (auto&& bit : MediaTypeIterator()) {
        if (toLower(mediaTypes[bit].data()) == toLower(flag)) {
            return 1 << to_underlying(bit);
        }
    }

    if (toLower(mediaTypes[MediaType::Any].data()) == toLower(flag)) {
        return -1;
    }
    return stoiString(flag, 0, 0);
}

void AutoscanDirectory::setMediaType(int mt)
{
    mediaType = mt;
    if (mt == -1)
        mt = ~0;
    for (auto&& mFlag : MediaTypeIterator()) {
        scanContent[upnpClasses[mFlag]] = (mt & (1 << to_underlying(mFlag)));
    }
}

void AutoscanDirectory::setCurrentLMT(const fs::path& loc, std::chrono::seconds lmt)
{
    bool firstScan = false;
    bool activeScan = false;
    auto current = currentTime();
    if (lmt > current)
        lmt = current;
    if (!loc.empty()) {
        auto lmDir = lastModified.find(loc);
        if (lmDir == lastModified.end() || lastModified.at(loc) > std::chrono::seconds::zero()) {
            firstScan = true;
        }
        if (lmDir == lastModified.end() || lastModified.at(loc) == std::chrono::seconds::zero()) {
            activeScan = true;
        }
        lastModified[loc] = lmt;
    }
    if (lmt == std::chrono::seconds::zero()) {
        if (firstScan) {
            activeScanCount++;
        }
    } else {
        if (activeScan && activeScanCount > 0) {
            activeScanCount--;
        }
        if (lmt > last_mod_current_scan) {
            last_mod_current_scan = lmt;
        }
    }
}

bool AutoscanDirectory::updateLMT()
{
    bool result = taskCount <= 0 && activeScanCount == 0;
    if (result) {
        result = last_mod_previous_scan < last_mod_current_scan;
        last_mod_previous_scan = last_mod_current_scan;
        log_debug("set autoscan lmt location: {}; last_modified: {}", location.c_str(), last_mod_current_scan.count());
    }
    return result;
}

std::chrono::seconds AutoscanDirectory::getPreviousLMT(const fs::path& loc, const std::shared_ptr<CdsContainer>& parent) const
{
    auto lmDir = lastModified.find(loc);
    if (!loc.empty() && lmDir != lastModified.end() && lastModified.at(loc) > std::chrono::seconds::zero()) {
        return lastModified.at(loc);
    }
    if (parent && parent->getMTime() > std::chrono::seconds::zero()) {
        return parent->getMTime();
    }
    return last_mod_previous_scan;
}

std::chrono::seconds AutoscanDirectory::getPreviousLMT() const
{
    return last_mod_previous_scan;
}

void AutoscanDirectory::setLocation(const fs::path& location)
{
    this->location = location;
}

void AutoscanDirectory::setScanID(int id)
{
    scanID = id;
    timer_parameter->setID(id);
}

bool AutoscanDirectory::hasContent(const std::string& upnpClass) const
{
    bool result = true;

    for (auto&& [cls, active] : scanContent) {
        if (cls == upnpClass)
            return active;
        if (startswith(upnpClass, cls))
            result = active;
        else if (startswith(cls, upnpClass)) // looking for base class with sub class
            result = false;
    }
    return result;
}

const char* AutoscanDirectory::mapScanmode(ScanMode scanmode)
{
    switch (scanmode) {
    case ScanMode::Timed:
        return "timed";
    case ScanMode::INotify:
        return "inotify";
    }
    throw_std_runtime_error("Illegal scanmode ({}) given to mapScanmode()", scanmode);
}

AutoscanDirectory::ScanMode AutoscanDirectory::remapScanmode(const std::string& scanmode)
{
    if (scanmode == "timed")
        return ScanMode::Timed;
    if (scanmode == "inotify")
        return ScanMode::INotify;

    throw_std_runtime_error("Illegal scanmode ({}) given to remapScanmode()", scanmode);
}

void AutoscanDirectory::copyTo(const std::shared_ptr<AutoscanDirectory>& copy) const
{
    copy->location = location;
    copy->mode = mode;
    copy->recursive = recursive;
    copy->hidden = hidden;
    copy->persistent_flag = persistent_flag;
    copy->interval = interval;
    copy->taskCount = taskCount;
    copy->scanID = scanID;
    copy->objectID = objectID;
    copy->databaseID = databaseID;
    copy->last_mod_previous_scan = last_mod_previous_scan;
    copy->last_mod_current_scan = last_mod_current_scan;
    copy->timer_parameter = timer_parameter;
    copy->scanContent = scanContent;
    copy->mediaType = mediaType;
    copy->containerMap = containerMap;
}

std::shared_ptr<Timer::Parameter> AutoscanDirectory::getTimerParameter() const
{
    return timer_parameter;
}

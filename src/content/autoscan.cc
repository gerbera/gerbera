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

AutoscanDirectory::AutoscanDirectory()
    : timer_parameter(std::make_shared<Timer::Parameter>(Timer::Parameter::IDAutoscan, INVALID_SCAN_ID))
{
}

AutoscanDirectory::AutoscanDirectory(fs::path location, ScanMode mode, bool recursive, bool persistent,
    int id, unsigned int interval, bool hidden)
    : location(std::move(location))
    , mode(mode)
    , recursive(recursive)
    , hidden(hidden)
    , persistent_flag(persistent)
    , interval(interval)
    , scanID(id)
    , timer_parameter(std::make_shared<Timer::Parameter>(Timer::Parameter::IDAutoscan, INVALID_SCAN_ID))
{
}

void AutoscanDirectory::setCurrentLMT(const std::string& loc, time_t lmt)
{
    bool firstScan = false;
    bool activeScan = false;
    if (!loc.empty()) {
        auto lmDir = lastModified.find(loc);
        if (lmDir == lastModified.end() || lastModified.at(loc) > 0) {
            firstScan = true;
        }
        if (lmDir == lastModified.end() || lastModified.at(loc) == 0) {
            activeScan = true;
        }
        lastModified[loc] = lmt;
    }
    if (lmt == 0) {
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
    bool result = taskCount <= 0 && activeScanCount <= 0;
    if (result) {
        result = last_mod_previous_scan < last_mod_current_scan;
        last_mod_previous_scan = last_mod_current_scan;
        log_debug("set autoscan lmt location: {}; last_modified: {}", location.c_str(), last_mod_current_scan);
    }
    return result;
}

time_t AutoscanDirectory::getPreviousLMT(const std::string& loc) const
{
    auto lmDir = lastModified.find(loc);
    if (!loc.empty() && lmDir != lastModified.end() && lastModified.at(loc) > 0) {
        return lastModified.at(loc);
    }
    return last_mod_previous_scan;
}

void AutoscanDirectory::setLocation(fs::path location)
{
    this->location = std::move(location);
}

void AutoscanDirectory::setScanID(int id)
{
    scanID = id;
    timer_parameter->setID(id);
}

std::string AutoscanDirectory::mapScanmode(ScanMode scanmode)
{
    std::string scanmode_str;
    switch (scanmode) {
    case ScanMode::Timed:
        scanmode_str = "timed";
        break;
    case ScanMode::INotify:
        scanmode_str = "inotify";
        break;
    default:
        throw_std_runtime_error("Illegal scanmode ({}) given to mapScanmode()", scanmode);
    }
    return scanmode_str;
}

ScanMode AutoscanDirectory::remapScanmode(const std::string& scanmode)
{
    if (scanmode == "timed")
        return ScanMode::Timed;
    if (scanmode == "inotify")
        return ScanMode::INotify;

    throw_std_runtime_error("Illegal scanmode ({}) given to remapScanmode()", scanmode.c_str());
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
}

std::shared_ptr<Timer::Parameter> AutoscanDirectory::getTimerParameter()
{
    return timer_parameter;
}

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

#include <utility>

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

void AutoscanDirectory::updateLMT()
{
    if (activeScanCount == 0) {
        last_mod_previous_scan = last_mod_current_scan;
        log_debug("set autoscan lmt location: {}; last_modified: {}", location.c_str(), last_mod_current_scan);
    }
}

time_t AutoscanDirectory::getPreviousLMT(const std::string& loc) const
{
    auto lmDir = lastModified.find(loc);
    if (!loc.empty() && lmDir != lastModified.end() && lastModified.at(loc) > 0) {
        return lastModified.at(loc);
    }
    return last_mod_previous_scan;
}

AutoscanList::AutoscanList(std::shared_ptr<Database> database)
    : database(std::move(database))
{
}

void AutoscanList::updateLMinDB()
{
    AutoLock lock(mutex);
    for (size_t i = 0; i < list.size(); i++) {
        log_debug("i: {}", i);
        auto ad = list[i];
        database->updateAutoscanDirectory(ad);
    }
}

int AutoscanList::add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index)
{
    AutoLock lock(mutex);
    return _add(dir, index);
}

int AutoscanList::_add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index)
{
    if (std::any_of(list.begin(), list.end(), [loc = dir->getLocation()](const auto& item) { return loc == item->getLocation(); })) {
        throw_std_runtime_error("Attempted to add same autoscan path twice");
    }
    if (index == SIZE_MAX) {
        index = getEditSize();
        origSize = list.size() + 1;
        dir->setOrig(true);
    } else {
        dir->setPersistent(true);
    }
    dir->setScanID(index);
    list.push_back(dir);
    indexMap[dir->getScanID()] = dir;

    return dir->getScanID();
}

void AutoscanList::addList(const std::shared_ptr<AutoscanList>& list)
{
    AutoLock lock(mutex);

    for (const auto& dir : list->list) {
        _add(dir, SIZE_MAX);
    }
}

size_t AutoscanList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}

std::vector<std::shared_ptr<AutoscanDirectory>> AutoscanList::getArrayCopy()
{
    AutoLock lock(mutex);

    return list;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(size_t id, bool edit)
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap[id];
    }
    return nullptr;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::getByObjectID(int objectID)
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](const auto& item) { return objectID == item->getObjectID(); });
    return it != list.end() ? *it : nullptr;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(const fs::path& location)
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](const auto& item) { return location == item->getLocation(); });
    return it != list.end() ? *it : nullptr;
}

void AutoscanList::remove(size_t id, bool edit)
{
    AutoLock lock(mutex);

    if (!edit) {
        if (id >= list.size()) {
            log_debug("No such ID {}!", id);
            return;
        }
        auto dir = list[id];
        dir->setScanID(INVALID_SCAN_ID);

        list.erase(list.begin() + id);
        log_debug("ID {} removed!", id);
    } else {
        if (indexMap.find(id) == indexMap.end()) {
            log_debug("No such index ID {}!", id);
            return;
        }
        const auto& dir = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [=](const auto& item) { return dir->getScanID() == item->getScanID(); });
        dir->setScanID(INVALID_SCAN_ID);
        list.erase(entry);

        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}

std::shared_ptr<AutoscanList> AutoscanList::removeIfSubdir(const fs::path& parent, bool persistent)
{
    AutoLock lock(mutex);

    auto rm_id_list = std::make_shared<AutoscanList>(database);

    for (auto it = list.begin(); it != list.end(); /*++it*/) {
        auto& dir = *it;

        if (startswith(dir->getLocation(), parent)) {
            if (dir->persistent() && !persistent) {
                ++it;
                continue;
            }
            indexMap[dir->getScanID()] = nullptr;
            auto copy = std::make_shared<AutoscanDirectory>();
            dir->copyTo(copy);
            copy->setScanID(dir->getScanID());
            rm_id_list->add(copy);

            dir->setScanID(INVALID_SCAN_ID);
            it = list.erase(it);
        } else
            ++it;
    }

    return rm_id_list;
}

void AutoscanList::notifyAll(Timer::Subscriber* sub)
{
    if (sub == nullptr)
        return;
    AutoLock lock(mutex);

    for (const auto& i : list) {
        sub->timerNotify(i->getTimerParameter());
    }
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
        throw_std_runtime_error("illegal scanmode given to mapScanmode()");
    }
    return scanmode_str;
}

ScanMode AutoscanDirectory::remapScanmode(const std::string& scanmode)
{
    if (scanmode == "timed")
        return ScanMode::Timed;
    if (scanmode == "inotify")
        return ScanMode::INotify;

    throw_std_runtime_error("illegal scanmode (" + scanmode + ") given to remapScanmode()");
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

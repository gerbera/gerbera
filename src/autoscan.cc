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

#include "autoscan.h"
#include "content_manager.h"
#include "storage/storage.h"

using namespace std;

AutoscanDirectory::AutoscanDirectory()
{
    taskCount = 0;
    objectID = INVALID_OBJECT_ID;
    storageID = INVALID_OBJECT_ID;
    last_mod_previous_scan = 0;
    last_mod_current_scan = 0;
    timer_parameter = std::make_shared<Timer::Parameter>(Timer::Parameter::IDAutoscan, INVALID_SCAN_ID);
}

AutoscanDirectory::AutoscanDirectory(std::string location, ScanMode mode,
    ScanLevel level, bool recursive, bool persistent,
    int id, unsigned int interval, bool hidden)
    : location(location)
    , mode(mode)
    , level(level)
    , recursive(recursive)
    , hidden(hidden)
    , persistent_flag(persistent)
    , interval(interval)
    , taskCount(0)
    , scanID(id)
    , objectID(INVALID_OBJECT_ID)
    , storageID(INVALID_OBJECT_ID)
    , last_mod_previous_scan(0)
    , last_mod_current_scan(0)
{
    timer_parameter = std::make_shared<Timer::Parameter>(Timer::Parameter::IDAutoscan, INVALID_SCAN_ID);
}

void AutoscanDirectory::setCurrentLMT(time_t lmt)
{
    if (lmt > last_mod_current_scan)
        last_mod_current_scan = lmt;
}

AutoscanList::AutoscanList(std::shared_ptr<Storage> storage)
    : storage(storage)
{
}

void AutoscanList::updateLMinDB()
{
    AutoLock lock(mutex);
    for (size_t i = 0; i < list.size(); i++) {
        log_debug("i: {}", i);
        auto ad = list[i];
        storage->autoscanUpdateLM(ad);
    }
}

int AutoscanList::add(std::shared_ptr<AutoscanDirectory> dir)
{
    AutoLock lock(mutex);
    return _add(dir);
}

int AutoscanList::_add(std::shared_ptr<AutoscanDirectory> dir)
{
    std::string loc = dir->getLocation();

    for (size_t i = 0; i < list.size(); i++) {
        if (loc == list[i]->getLocation()) {
            throw std::runtime_error("Attempted to add same autoscan path twice");
        }
    }

    dir->setScanID(list.size());
    list.push_back(dir);

    return dir->getScanID();
}

void AutoscanList::addList(std::shared_ptr<AutoscanList> list)
{
    AutoLock lock(mutex);

    for (size_t i = 0; i < list->list.size(); i++) {
        _add(list->list[i]);
    }
}

std::vector<std::shared_ptr<AutoscanDirectory>> AutoscanList::getArrayCopy()
{
    AutoLock lock(mutex);

    return list;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(size_t id)
{
    AutoLock lock(mutex);

    if ((id < 0) || (id >= list.size()))
        return nullptr;

    return list[id];
}

std::shared_ptr<AutoscanDirectory> AutoscanList::getByObjectID(int objectID)
{
    AutoLock lock(mutex);

    for (size_t i = 0; i < list.size(); i++) {
        if (objectID == list[i]->getObjectID())
            return list[i];
    }
    return nullptr;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(std::string location)
{
    AutoLock lock(mutex);
    for (size_t i = 0; i < list.size(); i++) {
        if (location == list[i]->getLocation())
            return list[i];
    }
    return nullptr;
}

void AutoscanList::remove(size_t id)
{
    AutoLock lock(mutex);

    if ((id < 0) || (id >= list.size())) {
        log_debug("No such ID {}!", id);
        return;
    }

    auto dir = list[id];
    dir->setScanID(INVALID_SCAN_ID);

    list.erase(list.begin() + id);
    log_debug("ID {} removed!", id);
}

int AutoscanList::removeByObjectID(int objectID)
{
    AutoLock lock(mutex);

    for (size_t i = 0; i < list.size(); i++) {
        if (objectID == list[i]->getObjectID()) {
            auto dir = list[i];
            dir->setScanID(INVALID_SCAN_ID);
            list.erase(list.begin() + i);
            return i;
        }
    }
    return INVALID_SCAN_ID;
}

int AutoscanList::remove(std::string location)
{
    AutoLock lock(mutex);

    for (size_t i = 0; i < list.size(); i++) {
        if (location == list[i]->getLocation()) {
            auto dir = list[i];
            dir->setScanID(INVALID_SCAN_ID);
            list.erase(list.begin() + i);
            return i;
        }
    }
    return INVALID_SCAN_ID;
}

std::shared_ptr<AutoscanList> AutoscanList::removeIfSubdir(std::string parent, bool persistent)
{
    AutoLock lock(mutex);

    auto rm_id_list = std::make_shared<AutoscanList>(storage);

    for (auto it = list.begin(); it != list.end(); /*++it*/) {
        auto& dir = *it;

        if (startswith(dir->getLocation(), parent)) {
            if (dir->persistent() && !persistent) {
                 ++it;
                continue;
            }
            auto copy = std::make_shared<AutoscanDirectory>();
            dir->copyTo(copy);
            copy->setScanID(dir->getScanID());
            rm_id_list->add(copy);

            dir->setScanID(INVALID_SCAN_ID);
            it = list.erase(it);
        }
        else
            ++it;

    }

    return rm_id_list;
}

void AutoscanList::notifyAll(Timer::Subscriber* sub)
{
    if (sub == nullptr)
        return;
    AutoLock lock(mutex);

    for (size_t i = 0; i < list.size(); i++) {
        sub->timerNotify(list[i]->getTimerParameter());
    }
}

void AutoscanDirectory::setLocation(std::string location)
{
    this->location = location;
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
        throw std::runtime_error("illegal scanmode given to mapScanmode()");
    }
    return scanmode_str;
}

ScanMode AutoscanDirectory::remapScanmode(std::string scanmode)
{
    if (scanmode == "timed")
        return ScanMode::Timed;
    if (scanmode == "inotify")
        return ScanMode::INotify;
    else
        throw std::runtime_error("illegal scanmode (" + scanmode + ") given to remapScanmode()");
}

std::string AutoscanDirectory::mapScanlevel(ScanLevel scanlevel)
{
    std::string scanlevel_str;
    switch (scanlevel) {
    case ScanLevel::Basic:
        scanlevel_str = "basic";
        break;
    case ScanLevel::Full:
        scanlevel_str = "full";
        break;
    default:
        throw std::runtime_error("illegal scanlevel given to mapScanlevel()");
    }
    return scanlevel_str;
}

ScanLevel AutoscanDirectory::remapScanlevel(std::string scanlevel)
{
    if (scanlevel == "basic")
        return ScanLevel::Basic;
    else if (scanlevel == "full")
        return ScanLevel::Full;
    else
        throw std::runtime_error("illegal scanlevel (" + scanlevel + ") given to remapScanlevel()");
}

void AutoscanDirectory::copyTo(std::shared_ptr<AutoscanDirectory> copy)
{
    copy->location = location;
    copy->mode = mode;
    copy->level = level;
    copy->recursive = recursive;
    copy->hidden = hidden;
    copy->persistent_flag = persistent_flag;
    copy->interval = interval;
    copy->taskCount = taskCount;
    copy->scanID = scanID;
    copy->objectID = objectID;
    copy->storageID = storageID;
    copy->last_mod_previous_scan = last_mod_previous_scan;
    copy->last_mod_current_scan = last_mod_current_scan;
    copy->timer_parameter = timer_parameter;
}

std::shared_ptr<Timer::Parameter> AutoscanDirectory::getTimerParameter()
{
    return timer_parameter;
}

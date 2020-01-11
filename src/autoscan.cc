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

using namespace zmm;
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
    list = Ref<Array<AutoscanDirectory>>(new Array<AutoscanDirectory>());
}

void AutoscanList::updateLMinDB()
{
    AutoLock lock(mutex);
    for (int i = 0; i < list->size(); i++) {
        log_debug("i: {}", i);
        Ref<AutoscanDirectory> ad = list->get(i);
        if (ad != nullptr)
            storage->autoscanUpdateLM(ad);
    }
}

int AutoscanList::add(Ref<AutoscanDirectory> dir)
{
    AutoLock lock(mutex);
    return _add(dir);
}

int AutoscanList::_add(Ref<AutoscanDirectory> dir)
{

    std::string loc = dir->getLocation();
    int nil_index = -1;

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) == nullptr) {
            nil_index = i;
            continue;
        }

        if (loc == list->get(i)->getLocation()) {
            throw std::runtime_error("Attempted to add same autoscan path twice");
        }
    }

    if (nil_index != -1) {
        dir->setScanID(nil_index);
        list->set(dir, nil_index);
    } else {
        dir->setScanID(list->size());
        list->append(dir);
    }

    return dir->getScanID();
}

void AutoscanList::addList(zmm::Ref<AutoscanList> list)
{
    AutoLock lock(mutex);

    for (int i = 0; i < list->list->size(); i++) {
        if (list->list->get(i) == nullptr)
            continue;

        _add(list->list->get(i));
    }
}

Ref<Array<AutoscanDirectory>> AutoscanList::getArrayCopy()
{
    AutoLock lock(mutex);
    Ref<Array<AutoscanDirectory>> copy(new Array<AutoscanDirectory>(list->size()));
    for (int i = 0; i < list->size(); i++)
        copy->append(list->get(i));

    return copy;
}

Ref<AutoscanDirectory> AutoscanList::get(int id)
{
    AutoLock lock(mutex);

    if ((id < 0) || (id >= list->size()))
        return nullptr;

    return list->get(id);
}

Ref<AutoscanDirectory> AutoscanList::getByObjectID(int objectID)
{
    AutoLock lock(mutex);

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) != nullptr && objectID == list->get(i)->getObjectID())
            return list->get(i);
    }
    return nullptr;
}

Ref<AutoscanDirectory> AutoscanList::get(std::string location)
{
    AutoLock lock(mutex);
    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) != nullptr && (location == list->get(i)->getLocation()))
            return list->get(i);
    }
    return nullptr;
}

void AutoscanList::remove(int id)
{
    AutoLock lock(mutex);

    if ((id < 0) || (id >= list->size())) {
        log_debug("No such ID {}!", id);
        return;
    }

    Ref<AutoscanDirectory> dir = list->get(id);
    dir->setScanID(INVALID_SCAN_ID);

    if (id == list->size() - 1) {
        list->removeUnordered(id);
    } else {
        list->set(nullptr, id);
    }

    log_debug("ID {} removed!", id);
}

int AutoscanList::removeByObjectID(int objectID)
{
    AutoLock lock(mutex);

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) != nullptr && objectID == list->get(i)->getObjectID()) {
            Ref<AutoscanDirectory> dir = list->get(i);
            dir->setScanID(INVALID_SCAN_ID);
            if (i == list->size() - 1) {
                list->removeUnordered(i);
            } else {
                list->set(nullptr, i);
            }
            return i;
        }
    }
    return INVALID_SCAN_ID;
}

int AutoscanList::remove(std::string location)
{
    AutoLock lock(mutex);

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) != nullptr && location == list->get(i)->getLocation()) {
            Ref<AutoscanDirectory> dir = list->get(i);
            dir->setScanID(INVALID_SCAN_ID);
            if (i == list->size() - 1) {
                list->removeUnordered(i);
            } else {
                list->set(nullptr, i);
            }
            return i;
        }
    }
    return INVALID_SCAN_ID;
}

Ref<AutoscanList> AutoscanList::removeIfSubdir(std::string parent, bool persistent)
{
    AutoLock lock(mutex);

    Ref<AutoscanList> rm_id_list(new AutoscanList(storage));

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) != nullptr && startswith(list->get(i)->getLocation(), parent)) {
            Ref<AutoscanDirectory> dir = list->get(i);
            if (dir == nullptr)
                continue;
            if (dir->persistent() && !persistent) {
                continue;
            }
            Ref<AutoscanDirectory> copy(new AutoscanDirectory());
            dir->copyTo(copy);
            rm_id_list->add(copy);
            copy->setScanID(dir->getScanID());
            dir->setScanID(INVALID_SCAN_ID);
            if (i == list->size() - 1) {
                list->removeUnordered(i);
            } else {
                list->set(nullptr, i);
            }
        }
    }

    return rm_id_list;
}

void AutoscanList::notifyAll(Timer::Subscriber* sub)
{
    if (sub == nullptr)
        return;
    AutoLock lock(mutex);

    for (int i = 0; i < list->size(); i++) {
        if (list->get(i) == nullptr)
            continue;
        sub->timerNotify(list->get(i)->getTimerParameter());
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

void AutoscanDirectory::copyTo(Ref<AutoscanDirectory> copy)
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

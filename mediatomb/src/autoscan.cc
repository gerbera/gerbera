/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    autoscan.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file autoscan.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "autoscan.h"
#include "storage.h"

using namespace zmm;

AutoscanDirectory::AutoscanDirectory(String location, scan_mode_t mode,
        scan_level_t level, bool recursive, bool persistent,
        int id, unsigned int interval, bool hidden)
{
    this->location = location;
    this->mode = mode;
    this->level = level;
    this->recursive = recursive;
    this->hidden = hidden;
    this->interval = interval;
    this->persistent_flag = persistent;
    scanID = id;
    taskCount = 0;
    objectID = INVALID_OBJECT_ID;
    storageID = INVALID_OBJECT_ID;
    last_mod_previous_scan = 0;
    last_mod_current_scan = 0;
}

void AutoscanDirectory::setCurrentLMT(time_t lmt) 
{
    if (lmt > last_mod_current_scan)
        last_mod_current_scan = lmt;
}

AutoscanList::AutoscanList()
{
    mutex = Ref<Mutex>(new Mutex(true));
    list = Ref<Array<AutoscanDirectory> > (new Array<AutoscanDirectory>());
}

void AutoscanList::updateLMinDB()
{
    AUTOLOCK(mutex);
    for (int i = 0; i < list->size(); i++)
        Storage::getInstance()->autoscanUpdateLM(list->get(i));
}

int AutoscanList::add(Ref<AutoscanDirectory> dir)
{
    AUTOLOCK(mutex);
    return _add(dir);
}

int AutoscanList::_add(Ref<AutoscanDirectory> dir)
{

    String loc = dir->getLocation();
    int nil_index = -1;
    
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) == nil)
        {
            nil_index = i;
            continue;
        }
        
        if (loc == list->get(i)->getLocation())
        {
            throw _Exception(_("Attempted to add same autoscan path twice"));
        }
    }
    
    if (nil_index != -1)
    {
        dir->setScanID(nil_index);
        list->set(dir, nil_index);
    }
    else
    {
        dir->setScanID(list->size());
        list->append(dir);
    }

    return dir->getScanID();
}

void AutoscanList::addList(zmm::Ref<AutoscanList> list)
{
    AUTOLOCK(mutex);
    
    for (int i = 0; i < list->list->size(); i++)
    {
        if (list->list->get(i) == nil)
            continue;

        _add(list->list->get(i));
    }
}

Ref<AutoscanDirectory> AutoscanList::get(int id)
{
    AUTOLOCK(mutex);

    if ((id < 0) || (id >= list->size()))
        return nil;

    return list->get(id);
}

void AutoscanList::remove(int id)
{
    AUTOLOCK(mutex);
    
    if ((id < 0) || (id >= list->size()))
                return;
   
    Ref<AutoscanDirectory> dir = list->get(id);
    dir->setScanID(INVALID_SCAN_ID);

    if (id == list->size()-1)
    {
        list->removeUnordered(id);
    }
    else
    {
        list->set(nil, id);
    }
}

int AutoscanList::remove(String location)
{
    AUTOLOCK(mutex);
    
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) != nil && location == list->get(i)->getLocation())
        {
            Ref<AutoscanDirectory> dir = list->get(i);
            dir->setScanID(INVALID_SCAN_ID);
            if (i == list->size()-1)
            {
                list->removeUnordered(i);
            }
            else
            {
                list->set(nil, i);
            }
            return i;
        }
    }
    return INVALID_SCAN_ID;
}

void AutoscanList::removeIfSubdir(String parent, bool persistent)
{
    AUTOLOCK(mutex);
    
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) != nil && (parent.startsWith(list->get(i)->getLocation())))
        {
            Ref<AutoscanDirectory> dir = list->get(i);
            if (dir->persistent() && (persistent == false))
                continue;

            dir->setScanID(INVALID_SCAN_ID);
            if (i == list->size()-1)
            {
                list->removeUnordered(i);
            }
            else
            {
                list->set(nil, i);
            }
        }
    }
}


/*
void AutoscanList::subscribeAll(Ref<TimerSubscriber> obj)
{
    AUTOLOCK(mutex);
    
    Ref<Timer> timer = Timer::getInstance();
    for (int i = 0; i < list->size(); i++)
    {
        Ref<AutoscanDirectory> dir = list->get(i);
        if (dir == nil)
            continue;
        timer->addTimerSubscriber(obj, dir->getInterval(), dir->getScanID(), true);
    }
}
*/

void AutoscanList::notifyAll(Ref<TimerSubscriberSingleton<Object> > cm)
{
    AUTOLOCK(mutex);
    
    Ref<Timer> timer = Timer::getInstance();
    for (int i = 0; i < list->size(); i++)
    {
        if (list->get(i) == nil)
            continue;
        
       cm->timerNotify(i);
    }
}

/*
void AutoscanList::subscribeDir(zmm::Ref<TimerSubscriber> obj, int id, bool once)
{
    AUTOLOCK(mutex);

    if ((id < 0) || (id >= list->size()))
        return;
  
    Ref<Timer> timer = Timer::getInstance();
    Ref<AutoscanDirectory> dir = list->get(id);
    timer->addTimerSubscriber(obj, dir->getInterval(), dir->getScanID(), once);
}
*/

String AutoscanDirectory::mapScanmode(scan_mode_t scanmode)
{
    String scanmode_str = nil;
    switch (scanmode)
    {
        case TimedScanMode: scanmode_str = _("timed"); break;
    }
    if (scanmode_str == nil)
        throw Exception(_("illegal scanmode given to mapScanmode(): ") + scanmode);
    return scanmode_str;
}

scan_mode_t AutoscanDirectory::remapScanmode(String scanmode)
{
    if (scanmode == "timed")
        return TimedScanMode;
    else
        throw _Exception(_("illegal scanmode (") + scanmode + ") given to remapScanmode()");
}

String AutoscanDirectory::mapScanlevel(scan_level_t scanlevel)
{
    String scanlevel_str = nil;
    switch (scanlevel)
    {
        case BasicScanLevel: scanlevel_str = _("basic"); break;
        case FullScanLevel: scanlevel_str = _("full"); break;
    }
    if (scanlevel_str == nil)
        throw Exception(_("illegal scanlevel given to mapScanlevel(): ") + scanlevel);
    return scanlevel_str;
}

scan_level_t AutoscanDirectory::remapScanlevel(String scanlevel)
{
    if (scanlevel == "basic")
        return BasicScanLevel;
    else if (scanlevel == "full")
        return FullScanLevel;
    else
        throw _Exception(_("illegal scanlevel (") + scanlevel + ") given to remapScanlevel()");
}


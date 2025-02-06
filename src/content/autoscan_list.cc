/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_list.cc - this file is part of Gerbera.

    Copyright (C) 2021-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file autoscan_list.cc
#define GRB_LOG_FAC GrbLogFacility::autoscan

#include "autoscan_list.h"

#include "config/result/autoscan.h"
#include "content.h"
#include "database/database.h"
#include "exceptions.h"
#include "util/timer.h"

#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#include "content/inotify/mt_inotify.h"
#endif

#include <algorithm>

void AutoscanList::updateLMinDB(Database& database)
{
    AutoLock lock(mutex);
    for (std::size_t i = 0; i < list.size(); i++) {
        log_debug("i: {}", i);
        auto ad = list[i];
        database.updateAutoscanDirectory(ad);
    }
}

std::shared_ptr<AutoscanDirectory> AutoscanList::getByObjectID(int objectID) const
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](auto&& item) { return objectID == item->getObjectID(); });
    return it != list.end() ? *it : nullptr;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::getKey(const fs::path& location) const
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](auto&& item) { return location == item->getLocation(); });
    return it != list.end() ? *it : nullptr;
}

std::shared_ptr<AutoscanList> AutoscanList::removeIfSubdir(const fs::path& parent, bool persistent)
{
    AutoLock lock(mutex);

    auto rmIdList = std::make_shared<AutoscanList>();

    for (auto it = list.begin(); it != list.end(); /*++it*/) {
        auto dir = *it;

        if (isSubDir(dir->getLocation(), parent)) {
            if (dir->persistent() && !persistent) {
                ++it;
                continue;
            }
            indexMap[dir->getScanID()] = nullptr;
            auto copy = std::make_shared<AutoscanDirectory>();
            dir->copyTo(copy);
            copy->setScanID(dir->getScanID());
            rmIdList->add(copy);

            dir->setInvalid();
            it = list.erase(it);
            continue;
        }
        ++it;
    }

    return rmIdList;
}

void AutoscanList::notifyAll(Timer::Subscriber* sub)
{
    if (!sub)
        return;
    AutoLock lock(mutex);

    for (auto&& i : list) {
        if (i->getScanMode() == AutoscanScanMode::Timed) {
            sub->timerNotify(i->getTimerParameter());
        }
    }
}

void AutoscanList::initTimer(
    std::shared_ptr<Content>& content,
    std::shared_ptr<Timer>& timer
#ifdef HAVE_INOTIFY
    ,
    bool doInotify, std::unique_ptr<AutoscanInotify>& inotify
#endif
)
{
    AutoLock lock(mutex);
    for (auto&& autoScanDir : list) {
#ifdef HAVE_INOTIFY
        if (autoScanDir->getScanMode() == AutoscanScanMode::INotify) {
            if (doInotify && inotify)
                inotify->monitor(autoScanDir);
            auto param = std::make_shared<Timer::Parameter>(Timer::TimerParamType::IDAutoscan, autoScanDir->getScanID());
            log_debug("Adding one-shot inotify scan");
            timer->addTimerSubscriber(content.get(), std::chrono::minutes(1), std::move(param), true);
        }
#endif
        if (autoScanDir->getScanMode() == AutoscanScanMode::Timed) {
            auto param = std::make_shared<Timer::Parameter>(Timer::TimerParamType::IDAutoscan, autoScanDir->getScanID());
            log_debug("Adding timed scan with interval {}", autoScanDir->getInterval().count());
            timer->addTimerSubscriber(content.get(), autoScanDir->getInterval(), std::move(param), false);
        }
    }
}

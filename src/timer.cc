/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    timer.cc - this file is part of MediaTomb.
    
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

/// \file timer.cc

#include "timer.h"
#include "singleton.h"

using namespace zmm;
using namespace std;

void Timer::triggerWait()
{
    log_debug("triggerWait. - %d subscriber(s)\n", subscribers.size());

    unique_lock<decltype(mutex)> lock(mutex);
    if (!subscribers.empty()) {
        struct timespec* timeout = getNextNotifyTime();
        struct timespec now;
        getTimespecNow(&now);
        if (isBefore(timeout, &now)) {
            log_debug("sleeping...\n");
            cv_status ret = cond.wait_for(lock,
                chrono::milliseconds(getDeltaMillis(timeout, &now)));
            if (ret == cv_status::timeout) {
                notify();
            }
        } else {
            notify();
        }
    } else {
        log_debug("nothing to do, sleeping...\n");
        cond.wait(lock);
    }
}

struct timespec* Timer::getNextNotifyTime()
{
    struct timespec* nextTime = nullptr;
    AutoLock lock(mutex);
    for (auto& subscriber : subscribers) {
        struct timespec* nextNotify = subscriber.getNextNotify();
        if (nextTime == nullptr || isBefore(nextNotify, nextTime) > 0) {
            nextTime = nextNotify;
        }
    }
    log_debug("next time: %d\n", nextTime->tv_sec);
    return nextTime;
}

void Timer::shutdown()
{
    log_debug("finished.\n");
}

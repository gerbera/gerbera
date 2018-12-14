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
#include <cassert>

using namespace zmm;
using namespace std;

void Timer::init()
{
    log_debug("Starting Timer thread...\n");
    int ret = pthread_create(
        &thread,
        nullptr,
        Timer::staticThreadProc,
        this);

    if (ret)
        throw _Exception(_("failed to start timer thread: ") + ret);
}

void* Timer::staticThreadProc(void* arg)
{
    log_debug("Started Timer thread.\n");
    auto* inst = (Timer*)arg;
    inst->threadProc();
    log_debug("Exiting Timer thread...\n");
    return nullptr;
}

void Timer::threadProc()
{
    triggerWait();
}

void Timer::addTimerSubscriber(Subscriber* timerSubscriber, unsigned int notifyInterval, zmm::Ref<Parameter> parameter, bool once)
{
    log_debug("Adding subscriber... interval: %d once: %d \n", notifyInterval, once);
    if (notifyInterval == 0)
        throw zmm::Exception(_("Tried to add timer with illegal notifyInterval: ") + notifyInterval);

    AutoLock lock(mutex);
    TimerSubscriberElement element(timerSubscriber, notifyInterval, parameter, once);
    for (auto& subscriber : subscribers) {
        if (subscriber == element) {
            throw zmm::Exception(_("Tried to add same timer twice"));
        }
    }
    subscribers.push_back(element);
    signal();
}

void Timer::removeTimerSubscriber(Subscriber* timerSubscriber, zmm::Ref<Parameter> parameter, bool dontFail)
{
    log_debug("Removing subscriber...\n");
    AutoLock lock(mutex);
    TimerSubscriberElement element(timerSubscriber, 0, parameter);
    auto it = std::find(subscribers.cbegin(), subscribers.cend(), element);
    if (it != subscribers.cend()) {
        subscribers.erase(it);
        signal();
    } else if (!dontFail) {
        throw zmm::Exception(_("Tried to remove nonexistent timer"));
    }
}

void Timer::triggerWait()
{
    unique_lock<std::mutex> lock(waitMutex);

    while (!shutdownFlag) {
        log_debug("triggerWait. - %d subscriber(s)\n", subscribers.size());

        if (subscribers.empty()) {
            log_debug("Nothing to do, sleeping...\n");
            cond.wait(lock);
            continue;
        }

        struct timespec* timeout = getNextNotifyTime();
        struct timespec now;
        getTimespecNow(&now);

        long wait = getDeltaMillis(&now, timeout);
        if (wait > 0) {
            cv_status ret = cond.wait_for(lock, chrono::milliseconds(wait));
            if (ret != cv_status::timeout) {
                /*
                 * Some rude thread woke us!
                 * Now we have to wait all over again...
                 */
                continue;
            }
        }
        notify();
    }
}

void Timer::notify()
{
    unique_lock<std::mutex> lock(mutex);
    assert(lock.owns_lock());

    std::list<TimerSubscriberElement> toNotify;

    for (auto it = subscribers.begin(); it != subscribers.end();) {
        TimerSubscriberElement& element = *it;

        struct timespec now;
        getTimespecNow(&now);
        long wait = getDeltaMillis(&now, element.getNextNotify());

        if (wait <= 0) {
            toNotify.push_back(element);
            if (element.isOnce()) {
                it = subscribers.erase(it);
            } else {
                element.updateNextNotify();
                ++it;
            }
        } else {
            ++it;
        }
    }

    // Unlock before we notify so that other threads can modify the subscribers
    lock.unlock();
    for (auto& element : toNotify) {
        element.notify();
    }
}

struct timespec* Timer::getNextNotifyTime()
{
    AutoLock lock(mutex);
    struct timespec* nextTime = nullptr;
    for (auto& subscriber : subscribers) {
        struct timespec* nextNotify = subscriber.getNextNotify();
        if (nextTime == nullptr || getDeltaMillis(nextTime, nextNotify) < 0) {
            nextTime = nextNotify;
        }
    }
    return nextTime;
}

void Timer::shutdown()
{
    shutdownFlag = true;
    cond.notify_all();
    pthread_join(thread, nullptr);
}

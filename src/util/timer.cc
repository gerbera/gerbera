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

#include "timer.h" // API

#include <cassert>

Timer::Timer(std::shared_ptr<Config> config)
    : config(std::move(config))
{
}

void Timer::run()
{
    log_debug("Starting Timer thread...");
    threadRunner = std::make_unique<StdThreadRunner>("TimerThread", Timer::staticThreadProc, this, config);

    // wait for TimerThread to become ready
    threadRunner->waitForReady();

    if (!threadRunner->isAlive())
        throw_std_runtime_error("Failed to start timer thread");
}

void* Timer::staticThreadProc(void* arg)
{
    log_debug("Started Timer thread.");
    auto inst = static_cast<Timer*>(arg);
    inst->threadProc();
    log_debug("Exiting Timer thread...");
    return nullptr;
}

void Timer::threadProc()
{
    triggerWait();
}

void Timer::addTimerSubscriber(Subscriber* timerSubscriber, std::chrono::seconds notifyInterval, std::shared_ptr<Parameter> parameter, bool once)
{
    log_debug("Adding subscriber... interval: {} once: {} ", notifyInterval.count(), once);
    if (notifyInterval == std::chrono::seconds::zero())
        throw_std_runtime_error("Tried to add timer with illegal notifyInterval: {}", notifyInterval.count());

    auto lock = threadRunner->lockGuard();
    TimerSubscriberElement element(timerSubscriber, notifyInterval, std::move(parameter), once);

    if (!subscribers.empty()) {
        bool err = std::find(subscribers.begin(), subscribers.end(), element) != subscribers.end();
        if (err) {
            throw_std_runtime_error("Tried to add same timer twice");
        }
    }

    subscribers.push_back(element);
    threadRunner->notify();
}

void Timer::removeTimerSubscriber(Subscriber* timerSubscriber, std::shared_ptr<Parameter> parameter, bool dontFail)
{
    log_debug("Removing subscriber...");
    auto lock = threadRunner->lockGuard();
    if (!subscribers.empty()) {
        TimerSubscriberElement element(timerSubscriber, std::chrono::seconds::zero(), std::move(parameter));
        auto it = std::find(subscribers.begin(), subscribers.end(), element);
        if (it != subscribers.end()) {
            subscribers.erase(it);
            threadRunner->notify();
            log_debug("Removed subscriber...");
            return;
        }
    }
    if (!dontFail) {
        throw_std_runtime_error("Tried to remove nonexistent timer");
    }
}

void Timer::triggerWait()
{
    StdThreadRunner::waitFor("Timer", [this] { return threadRunner != nullptr; });
    std::unique_lock<std::mutex> lock(waitMutex);

    // tell run() that we are ready
    threadRunner->setReady();

    while (!shutdownFlag) {
        log_debug("triggerWait. - {} subscriber(s)", subscribers.size());

        if (subscribers.empty()) {
            log_debug("Nothing to do, sleeping...");
            threadRunner->wait(lock);
            continue;
        }

        auto timeout = getNextNotifyTime();
        auto now = currentTimeMS();

        std::chrono::milliseconds wait = getDeltaMillis(now, timeout);
        if (wait > std::chrono::milliseconds::zero()) {
            auto ret = threadRunner->waitFor(lock, wait);
            if (ret != std::cv_status::timeout) {
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
    auto lock = threadRunner->uniqueLock();
    assert(lock.owns_lock());

    std::list<TimerSubscriberElement> toNotify;

    if (!subscribers.empty()) {
        for (auto it = subscribers.begin(); it != subscribers.end(); /*++it*/) {
            TimerSubscriberElement& element = *it;

            auto now = currentTimeMS();
            auto wait = getDeltaMillis(now, element.getNextNotify());

            if (wait <= std::chrono::milliseconds::zero()) {
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
    }

    // Unlock before we notify so that other threads can modify the subscribers
    lock.unlock();
    for (auto&& element : toNotify) {
        element.notify();
    }
}

std::chrono::milliseconds Timer::getNextNotifyTime()
{
    auto lock = threadRunner->lockGuard();
    auto nextTime = std::chrono::milliseconds::zero();
    if (!subscribers.empty()) {
        for (auto&& subscriber : subscribers) {
            auto nextNotify = subscriber.getNextNotify();
            if (nextTime == std::chrono::milliseconds::zero() || getDeltaMillis(nextTime, nextNotify) < std::chrono::milliseconds::zero()) {
                nextTime = nextNotify;
            }
        }
    }
    return nextTime;
}

void Timer::shutdown()
{
    shutdownFlag = true;
    threadRunner->notifyAll();
    threadRunner->join();
}

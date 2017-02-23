/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    timer.h - this file is part of MediaTomb.
    
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

/// \file timer.h

#ifndef __TIMER_H__
#define __TIMER_H__

#include "singleton.h"
#include "tools.h"
#include "zmm/ref.h"
#include "zmm/zmm.h"
#include "zmm/zmmf.h"
#include <algorithm>
#include <condition_variable>
#include <vector>

class TimerSubscriber {
public:
    virtual ~TimerSubscriber() { log_debug("TimerSubscriber destroyed\n"); }
    virtual void timerNotify(zmm::Ref<zmm::Object> parameter) = 0;
};

class Timer : public Singleton<Timer, std::recursive_mutex> {
public:
    virtual ~Timer() { log_debug("Timer destroyed!\n"); }

    virtual void shutdown();

    /**
     * @param timerSubscriber Caller must ensure that before this pointer is
     * freed the subscriber is removed by calling removeTimerSubscriber() with
     * the same parameter argument, unless the subscription is for a one-shot
     * timer and the subscriber has already been notified (and removed from the
     * subscribers list).
     */
    void addTimerSubscriber(TimerSubscriber* timerSubscriber, unsigned int notifyInterval, zmm::Ref<zmm::Object> parameter = nullptr, bool once = false)
    {
        log_debug("adding subscriber...\n");
        if (notifyInterval <= 0)
            throw zmm::Exception(_("tried to add timer with illegal notifyInterval: ") + notifyInterval);
        AutoLock lock(mutex);
        TimerSubscriberElement element(timerSubscriber, notifyInterval, parameter, once);
        for (auto& subscriber : subscribers) {
            if (subscriber == element) {
                throw zmm::Exception(_("tried to add same timer twice"));
            }
        }
        subscribers.push_back(element);
        signal();
    }

    void removeTimerSubscriber(TimerSubscriber* timerSubscriber, zmm::Ref<zmm::Object> parameter = nullptr, bool dontFail = false)
    {
        log_debug("removing subscriber...\n");
        AutoLock lock(mutex);
        TimerSubscriberElement element(timerSubscriber, 0, parameter);
        std::vector<TimerSubscriberElement>::const_iterator it = std::find(subscribers.cbegin(), subscribers.cend(), element);
        if (it != subscribers.cend()) {
            subscribers.erase(it);
            signal();
        } else if (!dontFail) {
            throw zmm::Exception(_("tried to remove nonexistent timer"));
        }
    }

    void triggerWait();

    inline void signal() { cond.notify_one(); }

protected:
    class TimerSubscriberElement : public zmm::Object {
    public:
        TimerSubscriberElement(TimerSubscriber* subscriber, unsigned int notifyInterval, zmm::Ref<zmm::Object> parameter, bool once = false)
            : disabled(false)
            , subscriber(subscriber)
            , notifyInterval(notifyInterval)
            , parameter(parameter)
            , once(once)
        {
            notified();
        }
        void notify()
        {
            try {
                subscriber->timerNotify(parameter);
            } catch (const zmm::Exception& e) {
                log_debug("timer caught exception!\n");
                e.printStackTrace();
            }
        }
        inline unsigned int getNotifyInterval() const { return notifyInterval; }
        inline TimerSubscriber* getSubscriber() { return subscriber; }
        inline void notified()
        {
            log_debug("notify interval: %d\n", notifyInterval);
            getTimespecAfterMillis(notifyInterval * 1000, &nextNotify);
        }
        inline struct timespec* getNextNotify() { return &nextNotify; }
        inline zmm::Ref<zmm::Object> getParameter() { return parameter; }
        bool operator==(const TimerSubscriberElement& other) const
        {
            return subscriber == other.subscriber && parameter == other.parameter;
        }
        bool isOnce() const { return once; }
        bool disabled = false;

    protected:
        TimerSubscriber* subscriber;
        unsigned int notifyInterval;
        zmm::Ref<zmm::Object> parameter;
        struct timespec nextNotify;
        bool once = false;
    };

    std::condition_variable_any cond;

    std::vector<TimerSubscriberElement> subscribers;

    void notify()
    {
        log_debug("notifying. - %d subscribers\n", subscribers.size());
        log_debug("waiting for lock\n");
        AutoLock lock(mutex);
        log_debug("got lock\n");
        int i = 0;
        struct timespec now;
        getTimespecNow(&now);
        for (auto& element : subscribers) {
            log_debug("in loop!\n");
            //log_debug("compare: %d\n", compareTimespecs(element.getNextNotify(), &now));
            if (!isBefore(element.getNextNotify(), &now)) {
                log_debug("notifying %d\n", i++);
                element.notify();
                log_debug("is once? %d\n", element.isOnce());
                if (element.isOnce()) {
                    element.disabled = true;
                } else {
                    element.notified();
                }
            }
        }
        //subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
        //            [](const auto & e){ return e.disabled; }));
    }

    struct timespec* getNextNotifyTime();
};

#endif // __TIMER_H__

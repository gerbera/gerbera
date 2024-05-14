/*MT*

    MediaTomb - http://www.mediatomb.cc/

    timer.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include <algorithm>
#include <atomic>
#include <memory>

#include "common.h"
#include "grb_time.h"
#include "thread_runner.h"

class Timer {
public:
    /// \brief This is the parameter class for timerNotify
    class Parameter {
    public:
        enum timer_param_t {
            IDAutoscan,
#ifdef ONLINE_SERVICES
            IDOnlineContent,
#endif
        };

        Parameter(timer_param_t param, int id)
            : param(param)
            , id(id)
        {
        }

        timer_param_t whoami() const { return param; }
        void setID(int id) { this->id = id; }
        int getID() const { return id; }

    protected:
        timer_param_t param;
        int id;
    };

    class Subscriber {
    public:
        Subscriber() = default;
        virtual ~Subscriber() = default;
        virtual void timerNotify(const std::shared_ptr<Parameter>& parameter) = 0;
    };

    Timer() = default;

    void run();
    void shutdown();

    /// \brief Add a subscriber
    ///
    /// @param timerSubscriber Caller must ensure that before this pointer is
    /// freed the subscriber is removed by calling removeTimerSubscriber() with
    /// the same parameter argument, unless the subscription is for a one-shot
    /// timer and the subscriber has already been notified (and removed from the
    /// subscribers list).
    void addTimerSubscriber(Subscriber* timerSubscriber, std::chrono::seconds notifyInterval, std::shared_ptr<Parameter> parameter, bool once = false);
    void removeTimerSubscriber(Subscriber* timerSubscriber, std::shared_ptr<Parameter> parameter, bool dontFail = false);
    void triggerWait();

protected:
    class TimerSubscriberElement {
    public:
        TimerSubscriberElement(Subscriber* subscriber, std::chrono::seconds notifyInterval, std::shared_ptr<Parameter> parameter, bool once = false)
            : subscriber(subscriber)
            , notifyInterval(notifyInterval)
            , parameter(std::move(parameter))
            , once(once)
        {
            updateNextNotify();
        }
        void notify()
        {
            try {
                if (subscriber)
                    subscriber->timerNotify(parameter);
                else
                    log_error("subscriber null");
            } catch (const std::runtime_error& e) {
                log_error("timer caught exception!");
            }
        }
        void updateNextNotify()
        {
            auto start = currentTimeMS();
            nextNotify = start + notifyInterval;
        }
        std::chrono::milliseconds getNextNotify() const { return nextNotify; }

        std::shared_ptr<Parameter> getParameter() const { return parameter; }

        bool operator==(const TimerSubscriberElement& other) const
        {
            return subscriber == other.subscriber && parameter == other.parameter;
        }
        bool isOnce() const { return once; }

    protected:
        Subscriber* subscriber;
        std::chrono::milliseconds notifyInterval;
        std::shared_ptr<Parameter> parameter;
        std::chrono::milliseconds nextNotify {};
        bool once;
    };

    std::mutex waitMutex;
    std::vector<TimerSubscriberElement> subscribers;
    std::atomic_bool shutdownFlag {};

    void notify();
    std::chrono::milliseconds getNextNotifyTime() const;

private:
    void threadProc();
    std::shared_ptr<Config> config;
    std::unique_ptr<StdThreadRunner> threadRunner;
};

#endif // __TIMER_H__

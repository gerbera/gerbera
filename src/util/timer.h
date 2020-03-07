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

#include <memory>

#include "tools.h"

#include <algorithm>
#include <condition_variable>
#include <list>

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
        {
            this->param = param;
            this->id = id;
        }

        timer_param_t whoami() { return param; }
        void setID(int id) { this->id = id; }
        int getID() { return id; }

    protected:
        timer_param_t param;
        int id;
    };

    class Subscriber {
    public:
        virtual ~Subscriber() { log_debug("Subscriber destroyed"); }
        virtual void timerNotify(std::shared_ptr<Parameter> parameter) = 0;
    };

    Timer();
    void run();

    virtual ~Timer() { log_debug("Timer destroyed"); }
    void shutdown();

    /// \brief Add a subscriber
    ///
    /// @param timerSubscriber Caller must ensure that before this pointer is
    /// freed the subscriber is removed by calling removeTimerSubscriber() with
    /// the same parameter argument, unless the subscription is for a one-shot
    /// timer and the subscriber has already been notified (and removed from the
    /// subscribers list).
    void addTimerSubscriber(Subscriber* timerSubscriber, unsigned int notifyInterval, std::shared_ptr<Parameter> parameter = nullptr, bool once = false);
    void removeTimerSubscriber(Subscriber* timerSubscriber, std::shared_ptr<Parameter> parameter = nullptr, bool dontFail = false);
    void triggerWait();
    inline void signal() { cond.notify_one(); }

protected:
    class TimerSubscriberElement {
    public:
        TimerSubscriberElement(Subscriber* subscriber, unsigned int notifyInterval, std::shared_ptr<Parameter> parameter, bool once = false)
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
                subscriber->timerNotify(parameter);
            } catch (const std::runtime_error& e) {
                log_error("timer caught exception!\n");
            }
        }
        inline void updateNextNotify()
        {
            getTimespecAfterMillis(notifyInterval * 1000, &nextNotify);
        }
        inline struct timespec* getNextNotify() { return &nextNotify; }

        inline std::shared_ptr<Parameter> getParameter() { return parameter; }

        bool operator==(const TimerSubscriberElement& other) const
        {
            return subscriber == other.subscriber && parameter == other.parameter;
        }
        bool isOnce() const { return once; }

    protected:
        Subscriber* subscriber;
        unsigned int notifyInterval;
        std::shared_ptr<Parameter> parameter;
        struct timespec nextNotify;
        bool once;
    };

    std::mutex waitMutex;
    std::condition_variable cond;
    std::list<TimerSubscriberElement> subscribers;
    std::atomic_bool shutdownFlag;

    void notify();
    struct timespec* getNextNotifyTime();

private:
    static void* staticThreadProc(void* arg);
    void threadProc();
    pthread_t thread;

    std::mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
};

#endif // __TIMER_H__

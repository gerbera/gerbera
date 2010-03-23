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

#include "zmm/zmm.h"
#include "zmmf/zmmf.h"
#include "singleton.h"
#include "sync.h"
#include "tools.h"

//#define AS_TIMER_SUBSCRIBER(klass, object) RefCast(Ref<TimerSubscriber<Singleton<klass> > >(object), TimerSubscriber<Object>)
#define AS_TIMER_SUBSCRIBER_SINGLETON(obj) zmm::Ref<TimerSubscriberSingleton<Object> >((TimerSubscriberSingleton<Object>*)obj)
#define AS_TIMER_SUBSCRIBER_SINGLETON_FROM_REF(obj) RefCast(obj, TimerSubscriberSingleton<Object>)

/// \todo Leo: please add doxygen documentation!

class TimerSubscriber
{
public:
    virtual ~TimerSubscriber() { log_debug("TS destroyed\n"); }
    virtual void timerNotify(zmm::Ref<zmm::Object> parameter) = 0;
//    void addTimerSubscriber(unsigned int notifyInterval, int id = 0, bool once = false);
//    void removeTimerSubscriber(int id = 0, bool dontFail = false);
};


template <class T>
class TimerSubscriberSingleton : public TimerSubscriber, public Singleton<T>
{
};

class TimerSubscriberObject : public TimerSubscriber, public zmm::Object
{
};


class Timer : public Singleton<Timer>
{
public:
    Timer();
    virtual ~Timer() { log_debug("Timer destroyed!\n"); }
    //static zmm::Ref<Timer> getInstance();
    
    virtual void shutdown();
    
    template <class T>
    void addTimerSubscriber(zmm::Ref<T> timerSubscriber, unsigned int notifyInterval, zmm::Ref<zmm::Object> parameter = nil, bool once = false)
    {
        log_debug("adding subscriber...\n");
        if (notifyInterval <= 0)
            throw zmm::Exception(_("tried to add timer with illegal notifyInterval: ") + notifyInterval);
        AUTOLOCK(mutex);
        //timerSubscriber->timerNotify(id);
        zmm::Ref<TimerSubscriberElement<T> > element(new TimerSubscriberElement<T>(timerSubscriber, notifyInterval, parameter, once));
        for(int i = 0; i < getAppropriateSubscribers<T>()->size(); i++)
        {
            if (getAppropriateSubscribers<T>()->get(i)->equals(element))
            {
                throw zmm::Exception(_("tried to add same timer twice"));
            }
        }
        getAppropriateSubscribers<T>()->append(element);
        signal();
    }
    
    template <class T>
    void removeTimerSubscriber(zmm::Ref<T> timerSubscriber, zmm::Ref<zmm::Object> parameter = nil, bool dontFail = false)
    {
        log_debug("removing subscriber...\n");
        AUTOLOCK(mutex);
        zmm::Ref<TimerSubscriberElement<T> > element(new TimerSubscriberElement<T>(timerSubscriber, 0, parameter));
        bool removed = false;
        for(int i = 0; i < getAppropriateSubscribers<T>()->size(); i++)
        {
            if (getAppropriateSubscribers<T>()->get(i)->equals(element))
            {
                getAppropriateSubscribers<T>()->removeUnordered(i);
                removed = true;
                break;
            }
        }
        if (! removed && ! dontFail)
        {
            throw zmm::Exception(_("tried to remove nonexistent timer"));
        }
        signal();
    }
    
    void triggerWait();
    
    inline void signal() { cond->signal(); }
    
protected:
    template <class T>
    class TimerSubscriberElement : public zmm::Object
    {
    public:
        TimerSubscriberElement(zmm::Ref<T> subscriber, unsigned int notifyInterval, zmm::Ref<zmm::Object> parameter, bool once = false)
        {
            this->subscriber = subscriber;
            this->notifyInterval = notifyInterval;
            this->parameter = parameter;
            this->once = once;
            notified();
        }
        inline unsigned int getNotifyInterval() { return notifyInterval; }
        inline zmm::Ref<T> getSubscriber() { return subscriber; }
        inline void notified() { getTimespecAfterMillis(notifyInterval * 1000, &nextNotify); }
        inline struct timespec *getNextNotify() { return &nextNotify; }
        inline zmm::Ref<zmm::Object> getParameter() { return parameter; }
        bool equals(zmm::Ref<TimerSubscriberElement> other) { return (subscriber == other->subscriber && parameter == other->parameter); }
        bool isOnce() { return once; }
    protected:
        zmm::Ref<T> subscriber;
        unsigned int notifyInterval;
        zmm::Ref<zmm::Object> parameter;
        struct timespec nextNotify;
        bool once;
    };
    
    
    //static zmm::Ref<Timer> instance;
    //static zmm::Ref<Mutex> mutex;
    zmm::Ref<Cond> cond;
    
    zmm::Ref<zmm::Array<TimerSubscriberElement<TimerSubscriberSingleton<Object> > > > subscribersSingleton;
    zmm::Ref<zmm::Array<TimerSubscriberElement<TimerSubscriberObject> > > subscribersObject;
    
    template <class T>
    zmm::Ref<zmm::Array<TimerSubscriberElement<T> > > getAppropriateSubscribers();
    
    template <class T>
    void notify()
    {
        struct timespec now;
        getTimespecNow(&now);
        log_debug("notifying. - %d subscribers\n", getAppropriateSubscribers<T>()->size());
        for(int i = 0; i < getAppropriateSubscribers<T>()->size(); i++)
        {
            zmm::Ref<TimerSubscriberElement<T> > element = getAppropriateSubscribers<T>()->get(i);
            if (compareTimespecs(element->getNextNotify(), &now) >= 0)
            {
                log_debug("notifying %d\n", i);
                zmm::Ref<T> subscriber = element->getSubscriber();
                try
                {
                    subscriber->timerNotify(element->getParameter());
                }
                catch (zmm::Exception e)
                {
                    log_debug("timer caught exception!\n");
                    e.printStackTrace();
                }
                element->notified();
                if (element->isOnce())
                {
                    getAppropriateSubscribers<T>()->removeUnordered(i--);
                }
            }
        }
    }
    
    struct timespec *getNextNotifyTime();
};

#endif // __TIMER_H__
